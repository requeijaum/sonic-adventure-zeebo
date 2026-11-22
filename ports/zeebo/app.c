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
    zaudio_stop();
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
        /* o jogo roda até o próximo native_vsync (fim de frame dele) */
        zscene_game_frame();
        zgame_step();
        me->tick++;
    } else {
        zscene_frame(me->tick++);
    }
    zstrip_flush();
    sa1_egl_swap(&me->egl);
    ISHELL_SetTimer(me->a.m_pIShell, SA1Z_MS_FRAME, (PFNNOTIFY)sa1z_Timer, me);
}
