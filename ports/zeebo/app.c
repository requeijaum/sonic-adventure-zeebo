/* app.c — applet de bring-up do port Zeebo (Z0).
 *
 * Ciclo de vida igual ao sa1hello/conftest da Zeebo SDK:
 * AEEClsCreateInstance -> AEEApplet_New -> EVT_APP_START; timer de 35 fps.
 * Aqui só há EGL + o sink GLES1 de teste (zstrip/zscene). O jogo ainda não
 * entra: o objetivo é provar o caminho ARM .mod -> IGLES11 -> zeebx com a
 * convenção de vértice do SA1. */

#include "AEEStdLib.h"
#include "AEEAppGen.h"
#include "sa1z.bid"
#include "platform/brew_egl.h"
#include "zstrip.h"
#include "zthreads.h"
void syTmrInit(void);
void pdInitPeripheral(int logic, void *recvbuf, void *sendbuf);
void zsys_key(int down, unsigned short key);
#include "zhid.h"

/* o jogo (game.mk) define main; sem ele (applet puro) o símbolo é nulo */
extern void main(void) __attribute__((weak));
#define ZGAME_PRESENT (main != 0)
/* tabelas de dados do asm do SH-4 em C (asm.c): preencher antes de qualquer chamada */
extern void native_asm_init(void) __attribute__((weak));

#define SA1Z_FPS      35
#define SA1Z_MS_FRAME (1000 / SA1Z_FPS)

typedef struct _TSa1Z {
    AEEApplet a;
    IDisplay *tela;
    TSa1Egl egl;
    boolean tem_gl;
    boolean rodando;
    uint32_t tick;
} TSa1Z;

static boolean sa1z_HandleEvent(IApplet *pi, AEEEvent eCode, uint16 wParam,
                                uint32 dwParam);
static void sa1z_Timer(IApplet *pi);

static void sa1z_Parar(TSa1Z *me)
{
    me->rodando = FALSE;
    zaudio_shutdown();
    zhid_close();
    zfile_close();
    ISHELL_CancelTimer(me->a.m_pIShell, NULL, (void *)me);
    if (me->tem_gl) {
        zstrip_tex_release_all();
        sa1_egl_encerrar(&me->egl);
        me->tem_gl = FALSE;
    }
}

static void sa1z_Iniciar(TSa1Z *me)
{
    if (me->rodando) {
        return;
    }
    me->tem_gl = sa1_egl_iniciar(&me->egl, me->a.m_pIShell, me->tela);
    if (!me->tem_gl) {
        sa1z_Parar(me);
        return;
    }
    {
        uint32 total = 0;
        uint32 maior = 0;
        uint32 livre = GETRAMFREE(&total, &maior);

        /* orçamento do heap do app: no console, o número que falta medir */
        DBGPRINTF("sa1z: ram livre=%u total=%u maior bloco=%u", livre, total,
                  maior);
    }
    /* o som depende de flag de compilacao (AUDIO=1); o log diz qual dos dois este .mod tem */
#ifdef ZAUDIO_OFF
    DBGPRINTF("sa1z: audio DESLIGADO (ZAUDIO_OFF; AUDIO=1 liga)");
#else
    DBGPRINTF("sa1z: audio ligado");
#endif
    syTmrInit();
    pdInitPeripheral(0, 0, 0);
    zfile_open(me->a.m_pIShell);
    if (!zdc_selftest() || !zthreads_selftest()) {
        sa1z_Parar(me);
        return;
    }
    if (ZGAME_PRESENT) {
        /* o jogo: só o estado GL; sem a cena e o áudio de teste do applet */
        zscene_gl_init();
    } else {
        zscene_init();
    }
    if (ZGAME_PRESENT) {
        if (native_asm_init != 0) {
            native_asm_init();
        }
        zgame_start(main);
    }
    zaudio_set_shell(me->a.m_pIShell);
    zhid_open(me->a.m_pIShell);
    if (!ZGAME_PRESENT) {
        zaudio_start(me->a.m_pIShell);
    }
    me->tick = 0;
    me->rodando = TRUE;
    ISHELL_SetTimer(me->a.m_pIShell, SA1Z_MS_FRAME, (PFNNOTIFY)sa1z_Timer, me);
}

int AEEClsCreateInstance(AEECLSID ClsId, IShell *pIShell, IModule *po,
                         void **ppObj)
{
    *ppObj = NULL;

    if (ClsId != AEECLSID_SA1Z) {
        return EFAILED;
    }
    if (AEEApplet_New(sizeof(TSa1Z), ClsId, pIShell, po, (IApplet **)ppObj,
                      (AEEHANDLER)sa1z_HandleEvent, NULL) != TRUE) {
        return EFAILED;
    }
    {
        TSa1Z *me = (TSa1Z *)*ppObj;

        me->tela = me->a.m_pIDisplay;
        me->tem_gl = FALSE;
        me->rodando = FALSE;
        me->tick = 0;
    }
    return AEE_SUCCESS;
}

static boolean sa1z_HandleEvent(IApplet *pi, AEEEvent eCode, uint16 wParam,
                                uint32 dwParam)
{
    TSa1Z *me = (TSa1Z *)pi;

    (void)dwParam;

    switch (eCode) {
    case EVT_APP_START:
    case EVT_APP_RESUME:
        sa1z_Iniciar(me);
        return TRUE;
    case EVT_KEY:
        zsys_key(1, wParam);
        return TRUE;
    case EVT_KEY_RELEASE:
        zsys_key(0, wParam);
        return TRUE;
    case EVT_APP_NO_SLEEP:
        return TRUE;
    case EVT_APP_SUSPEND:
    case EVT_APP_STOP:
        sa1z_Parar(me);
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

/* njWaitVSync (zninjasys.c) põe 1: o jogo fechou um quadro */
int zframe_end;

/* perfil: ms por segundo em cada parte, para saber onde o tempo vai */
static uint32 vprof_game, vprof_flush, vprof_swap, vprof_since, vprof_frames;
void zlog(const char *what, const char *name, int value);

extern unsigned int zlive_us __attribute__((weak));
extern unsigned zs_draws __attribute__((weak));
extern unsigned zs_redundant __attribute__((weak));
extern unsigned zs_hash __attribute__((weak));
extern unsigned zs_tstrips __attribute__((weak));
extern unsigned zs_tex16 __attribute__((weak));
extern unsigned zs_tshift __attribute__((weak));

static void vprof_report(void)
{
    uint32 now = GETUPTIMEMS();

    vprof_frames++;
    {
        static unsigned hash_frames;

        if (&zs_hash != 0 && (++hash_frames & 63) == 0) {
            zlog("estado", "hash", (int)zs_hash);
        }
    }
    if (vprof_since == 0) {
        vprof_since = now;
        return;
    }
    if (now - vprof_since < 2000) {
        return;
    }
    zaudio_report();
    zhid_report();
    zlog("vprof", "jogo", (int)vprof_game);
    if (&zlive_us != 0) { /* só no .mod (zlive.c), não no applet */
        zlog("vprof", "zlive", (int)zlive_us);
        zlive_us = 0;
    }
    if (&zs_draws != 0) { /* só no .mod (zstrip.c) */
        zlog("vprof", "desenhos", (int)zs_draws);
        zlog("vprof", "estado repetido", (int)zs_redundant);
        zs_draws = zs_redundant = 0;
        zlog("vprof", "texturas 16bpp", (int)zs_tex16);
        zlog("vprof", "transl strips", (int)zs_tstrips);
        zlog("vprof", "transl deslocamentos", (int)zs_tshift);
        zs_tstrips = zs_tshift = 0;
    }
    zlog("vprof", "gl", (int)vprof_flush);
    zlog("vprof", "swap", (int)vprof_swap);
    zlog("vprof", "ms jogo/quadro", vprof_frames ? (int)(vprof_game / vprof_frames) : -1);
    zlog("vprof", "quadros", (int)vprof_frames);
    zlog("vprof", "janela ms", (int)(now - vprof_since));
    {
        /* marca d'agua da RAM livre: o orcamento do console e 30-32 MB */
        static uint32 livre_min = 0xffffffffu;
        uint32 total = 0;
        uint32 maior = 0;
        uint32 livre = GETRAMFREE(&total, &maior);

        if (livre < livre_min) {
            livre_min = livre;
        }
        zlog("mem", "livre KB", (int)(livre / 1024));
        zlog("mem", "minimo livre KB", (int)(livre_min / 1024));
        zlog("mem", "maior bloco KB", (int)(maior / 1024));
    }
    vprof_game = vprof_flush = vprof_swap = 0;
    vprof_since = now;
    vprof_frames = 0;
}

static void sa1z_Timer(IApplet *pi)
{
#ifdef ZBENCH
    {
        void zbench_run(void);

        zbench_run(); /* a cada tique: o custo aparece em quadros perdidos */
    }
#endif
    TSa1Z *me = (TSa1Z *)pi;

    if (!me->rodando) {
        return;
    }
    if (ZGAME_PRESENT) {
        static int need_clear = 1;
        static int idle;

        /* o jogo roda até o próximo native_vsync. Só apresenta quando ele
         * fechou um quadro (njWaitVSync); até lá o que ele desenha se acumula
         * no mesmo buffer. idle: rede de segurança se nada fechar quadro */
        if (need_clear) {
            zscene_game_frame();
            need_clear = 0;
        }
        {
            uint32 t0 = GETUPTIMEMS();
            zgame_step();
            vprof_game += GETUPTIMEMS() - t0;
        }
        me->tick++;
        if (!zframe_end && ++idle < 4) {
            ISHELL_SetTimer(me->a.m_pIShell, SA1Z_MS_FRAME, (PFNNOTIFY)sa1z_Timer, me);
            return;
        }
        zframe_end = 0;
        idle = 0;
        need_clear = 1;
    } else {
        zscene_frame(me->tick++);
    }
    {
        uint32 t0 = GETUPTIMEMS();
#if Z_FPSHUD
        {
            extern void zhud_frame(unsigned, unsigned, unsigned);
            uint32 hud_total = 0, hud_max = 0;

            zhud_frame(t0, (unsigned)vprof_game,
                       (unsigned)(GETRAMFREE(&hud_total, &hud_max) >> 10));
        }
#endif
        zstrip_flush();
        vprof_flush += GETUPTIMEMS() - t0;
        t0 = GETUPTIMEMS();
        sa1_egl_swap(&me->egl);
        vprof_swap += GETUPTIMEMS() - t0;
    }
    vprof_report();
    ISHELL_SetTimer(me->a.m_pIShell, SA1Z_MS_FRAME, (PFNNOTIFY)sa1z_Timer, me);
}
