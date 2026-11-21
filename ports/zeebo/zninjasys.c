/* zninjasys.c — o njSystem do Ninja sem vídeo de Dreamcast.
 *
 * sdk/ninja/njSystem.c configura o PowerVR, espera vsync em registrador e
 * instala o tratador de blank; no Zeebo o vídeo é o GLES1 do applet. Aqui ficam
 * só as chamadas que o jogo faz de fato: iniciar/sair, esperar o fim de frame
 * (que devolve o controle ao timer do BREW) e o gancho de vsync. Os ajustes de
 * modo (blend, cull, filtro...) são stubs gerados (zstubs.c). */

#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

void native_vsync(void);
/* do njSystem.c do jogo (só existe com o jogo linkado): contextos, luzes, clip */
extern void njInitGlobal(void) __attribute__((weak));
void zlog(const char *what, const char *name, int value);
void pdExecPeripheralServer(void);
Uint32 syTmrGetCount(void);

static void (*vsync_function)(void);

/* chamado a cada vsync por native_vsync (zthreads.c) */
void zvsync_run(void)
{
    static int dbg;

    if (vsync_function != 0 && dbg < 3) {
        zlog("vsyncfn", "ptr", (int)(unsigned long)vsync_function);
        dbg++;
    }
    if (vsync_function) {
        vsync_function();
    }
}

void njInitSystem(Int mode, Int frame, Int count)
{
    (void)mode;
    _nj_frame_mode_ = frame;
    _nj_waitvsync_count = count;
    /* a saída é sempre 640x480 (o sink GLES1 fixa o ortho) */
    _nj_screen_.w = 640.0F;
    _nj_screen_.h = 480.0F;
    _nj_frame_size_w = 640;
    _nj_frame_size_h = 480;
    _nj_vsync_count_ = 0;
    _nj_eor_flag_ = 0;
    if (njInitGlobal != 0) {
        njInitGlobal();
    }
}

void njExitSystem(void)
{
}

void njSetVSyncFunction(void (*func)(void))
{
    zlog("vsyncset", "func", (int)(unsigned long)func);
    vsync_function = func;
}

void njSetEORflag()
{
    _nj_eor_flag_ = 0; /* o render do GLES1 é imediato: nunca fica pendente */
}

/* fim de frame do jogo: o resto é do timer do BREW, que chama zgame_step */
void njWaitVSync(void)
{
    _nj_wait_vsync_in_ = syTmrGetCount();
    _nj_eor_flag_ = 0;
    if (_nj_eor_function_) {
        _nj_eor_function_();
    }
    _nj_calc_vertex_ = 0;
    _nj_calc_polygon_ = 0;
    _nj_draw_polygon_ = 0;
    _nj_vsync_count_++;
    pdExecPeripheralServer();
    _nj_wait_vsync_out_ = syTmrGetCount();
    if (_nj_vsync_count_ <= 3) {
        zlog("njWaitVSync frame", 0, (int)_nj_vsync_count_);
    }
    native_vsync();
}
