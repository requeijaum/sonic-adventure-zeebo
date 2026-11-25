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

#define SA1Z_FPS      60
#define SA1Z_MS_FRAME (1000 / SA1Z_FPS)
/* O timer do BREW so aceita ms inteiros, e 1000/60 truncado da 16: sessenta tiques somavam
 * 960 ms, nao 1000, e o passo do jogo andava ~4% adiantado em relacao ao relogio. A fracao vai
 * num acumulador e vira um ms extra a cada 1,5 tique (16,17,17,16,...), media exata 16,667.
 * p_frame_ms e calculado UMA vez por tique: as duas funcoes de espera o consultam. */
static uint32 p_frame_ms = SA1Z_MS_FRAME;
static uint32 p_frame_frac;
static uint32 p_frame_sum; /* soma dos periodos pedidos, para conferir a media por janela */
static uint32 p_ticks;

static uint32 sa1z_frame_ms(void)
{
    uint32 ms = 1000 / SA1Z_FPS;

    p_frame_frac += 1000 % SA1Z_FPS;
    if (p_frame_frac >= SA1Z_FPS) {
        p_frame_frac -= SA1Z_FPS;
        ms++;
    }
    return ms;
}

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
void zlogd(const char *what, const char *name, int value);

extern unsigned int zlive_us __attribute__((weak));
extern unsigned zs_draws __attribute__((weak));
extern void zstrip_text_janela(unsigned *, unsigned *, unsigned *, unsigned *, unsigned *) __attribute__((weak));
extern unsigned zs_redundant __attribute__((weak));
extern unsigned zs_hash __attribute__((weak));
extern unsigned zs_tstrips __attribute__((weak));
extern unsigned zs_tex16 __attribute__((weak));
extern unsigned zs_tshift __attribute__((weak));

static void vprof_report(void)
{
    uint32 now = GETUPTIMEMS();

    vprof_frames++;
    zinst_frame();
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
    zinst_perf_report(vprof_game, vprof_flush, vprof_swap, now - vprof_since);
    /* tempo do guest (GETUPTIMEMS) no fim de cada janela de 2000 ms do relogio do guest:
     * e o numero que se compara com o tempo de parede do host para saber se o tique anda 1:1 */
    zlogd("tfun", "uptime ms", (int)now);
    zlogd("tfun", "janela ms", (int)(now - vprof_since));
    /* periodo pedido ao ISHELL_SetTimer, em us: se nao for ~16667 o tique nao esta em 60 Hz */
    zlogd("tfun", "periodo medio pedido us",
          (int)(p_ticks ? p_frame_sum * 1000 / p_ticks : 0));
    zlogd("tfun", "tiques na janela", (int)p_ticks);
    p_frame_sum = 0;
    p_ticks = 0;
    zinst_tex_report();
    zinst_prs_report();
    zfog_report();
    zaudio_report();
    zhid_report();
    zlog("vprof", "jogo", (int)vprof_game);
    if (&zlive_us != 0) { /* só no .mod (zlive.c), não no applet */
        zlog("vprof", "zlive", (int)zlive_us);
        zlive_us = 0;
    }
    if (&zs_draws != 0) { /* só no .mod (zstrip.c) */
        zlog("vprof", "desenhos", (int)zs_draws);
        if (&zstrip_text_janela != 0) {
            unsigned b = 0, d = 0, r = 0, tv = 0, uj = 0;

            zstrip_text_janela(&b, &d, &r, &tv, &uj);
            zlog("vprof", "binds de textura", (int)b);
            zlog("vprof", "texturas distintas", (int)d);
            zlog("vprof", "runs translucidos", (int)r);
            zlog("vprof", "vertices translucidos", (int)tv);
            if (uj) { /* sempre 0 com Z_TJOIN=0: so aparece quando a uniao esta ligada */
                zlog("vprof", "strips unidos", (int)uj);
            }
        }
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

/* o periodo e SA1Z_MS_FRAME entre o INICIO de um tique e o do seguinte, nao entre o fim e o
 * inicio: o tempo que o quadro gastou ja conta (antes o timer era armado depois do trabalho e
 * cada tique pesado levava mais 28 ms de espera por cima) */
static uint32 sa1z_next_delay(uint32 t_cb)
{
    uint32 used = GETUPTIMEMS() - t_cb;

    return used >= p_frame_ms ? 1 : p_frame_ms - used;
}

/* Ritmo do quadro do jogo: frame_interval vsyncs (2 = 30 fps), um native_vsync por vsync. No
 * console o relogio de vsync corre durante a conta do quadro; aqui cada native_vsync que nao
 * apresenta virava um tique de 16 ms ANTES da conta pesada (a que fecha o quadro): quadro =
 * 16 + conta + espera do swap pelo proximo vsync = 50 ms com 26 ms de CPU. Agora o tique que
 * nao apresenta espera so o que falta para a conta pesada (p_step_ms, media dos passos que
 * apresentaram) terminar perto do fim do periodo do quadro, contado do ultimo swap: o swap
 * cai no vsync do periodo e o quadro volta a ser 33 ms (30 fps) quando a conta cabe */
extern int frame_interval __attribute__((weak));
static uint32 t_pres_end;       /* fim do ultimo tique que apresentou */
static uint32 step_last;        /* passo do jogo do tique em andamento */
static uint32 p_step_ms = 20;   /* passo do jogo no tique que apresenta, media movel */

static uint32 sa1z_idle_delay(uint32 t_cb)
{
    uint32 now = GETUPTIMEMS();
    uint32 period = (uint32)((&frame_interval != 0 && frame_interval > 0 ? frame_interval : 2) * 50 / 3);
    uint32 target = t_pres_end + period - 3; /* 3 ms de folga antes do vsync */
    uint32 ready = now + p_step_ms + p_step_ms / 6; /* +17%: a conta varia de quadro a quadro */

    (void)t_cb;
    if (t_pres_end == 0 || ready >= target) {
        return 1;
    }
    if (target - ready > p_frame_ms) {
        return p_frame_ms;
    }
    return target - ready;
}

#ifndef Z_MENU30
#define Z_MENU30 1
#endif
#if Z_MENU30
extern int game_mode __attribute__((weak));

/* Menus (game_mode 10 = MENU, 11 = MENU_RUN, 16 = TUTORIAL) pedem frame_interval 1: um quadro
 * por vsync, 60 fps. Com MENUFPS30=1 (padrao) so um quadro fechado em dois e apresentado; a
 * logica do jogo segue a 60 Hz, entao a velocidade das animacoes nao muda. Devolve 1 quando
 * este quadro fechado nao deve ser apresentado. Os demais modos (30 fps pelo proprio jogo,
 * filmes, minijogos) nao passam por aqui: exige frame_interval == 1 */
static int sa1z_menu_skip(void)
{
    static unsigned phase;

    if (&game_mode == 0 || &frame_interval == 0 || frame_interval != 1) {
        phase = 0;
        return 0;
    }
    if (game_mode != 10 && game_mode != 11 && game_mode != 16) {
        phase = 0;
        return 0;
    }
    phase ^= 1u;
    return (int)phase; /* 1o quadro do par: pula; 2o: apresenta */
}
#endif

static void sa1z_Timer(IApplet *pi)
{
#ifdef ZBENCH
    {
        void zbench_run(void);

        zbench_run(); /* a cada tique: o custo aparece em quadros perdidos */
    }
#endif
    TSa1Z *me = (TSa1Z *)pi;
    uint32 t_cb = GETUPTIMEMS();
    int skip_present = 0;

    p_frame_ms = sa1z_frame_ms();
    p_frame_sum += p_frame_ms;
    p_ticks++;
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
            uint32 step;

            zgame_step();
            step = GETUPTIMEMS() - t0;
            vprof_game += step;
            step_last = step;
        }
        me->tick++;
        if (!zframe_end && ++idle < 4) {
            ISHELL_SetTimer(me->a.m_pIShell, sa1z_idle_delay(t_cb), (PFNNOTIFY)sa1z_Timer, me);
            return;
        }
        zframe_end = 0;
        idle = 0;
        need_clear = 1;
#if Z_MENU30
        skip_present = sa1z_menu_skip();
#endif
    } else {
        zscene_frame(me->tick++);
    }
    if (skip_present) {
        zstrip_frame_drop();
    } else {
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
        /* o tique que apresenta tambem gasta o flush dos translucidos antes do swap */
        p_step_ms = (p_step_ms * 3 + step_last + (GETUPTIMEMS() - t0)) / 4;
        t0 = GETUPTIMEMS();
        sa1_egl_swap(&me->egl);
        vprof_swap += GETUPTIMEMS() - t0;
    }
    vprof_report();
    t_pres_end = GETUPTIMEMS();
    ISHELL_SetTimer(me->a.m_pIShell, sa1z_next_delay(t_cb), (PFNNOTIFY)sa1z_Timer, me);
}
