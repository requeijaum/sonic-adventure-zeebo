/* sa1hello — EGL setup + frame loop + swap (o "shim" de plataforma).
 *
 * O motor (w3dEngine) so recebe estado e geometria; criar janela/contexto e
 * trocar buffers e trabalho nosso, contra os wrappers EGL_1x da SDK40.
 * Resize chega ao motor via Sa1Hello_Resize (viewport + PantallaSetTam),
 * definido em w3d_triangle.cpp — este .c nao toca em GL direto.
 */

#include "AEEStdLib.h"
#include "EGL_1x.h"
#include "GLES_1x.h"
#include "brew_egl.h"

/* Definido no lado C++ (w3d_triangle.cpp): informa viewport + orientacao. */
void Sa1Hello_Resize(int w, int h);

boolean sa1_egl_iniciar(TSa1Egl* g, IShell* shell, IDisplay* tela)
{
    EGLConfig  eglConfig = {0};
    EGLint     eglAttrib[] = {
        EGL_RED_SIZE,     5,
        EGL_GREEN_SIZE,   6,
        EGL_BLUE_SIZE,    5,
        EGL_DEPTH_SIZE,   16,
        EGL_STENCIL_SIZE, 4,
        EGL_NONE
    };
    EGLint     numConfigs = 0;
    IBitmap*   bitmapDDB = NULL;
    IDIB*      dib = NULL;
    AEEDeviceInfo di;

    if (!g || !shell || !tela)
        return FALSE;

    g->display  = 0;
    g->surface  = 0;
    g->contexto = 0;
    g->largura  = 0;
    g->altura   = 0;

    /* Nesta ordem (o guia e explicito); falha no segundo desfaz o primeiro. */
    if (EGL_Init(shell) != SUCCESS)
        return FALSE;
    if (GLES_Init(shell) != SUCCESS)
    {
        EGL_Release();
        return FALSE;
    }

    g->display = eglGetDisplay((NativeDisplayType)tela);
    if (g->display == EGL_NO_DISPLAY)
        goto falhou;

    if (!eglInitialize(g->display, NULL, NULL))
        goto falhou;

    if (!eglChooseConfig(g->display, eglAttrib, &eglConfig, 1, &numConfigs)
        || numConfigs < 1)
        goto falhou;

    /* A janela e o DIB do bitmap do painel: o Zeebo nao expoe framebuffer
     * proprio; o EGL quer um NativeWindowType, que e o IDIB. */
    if (IDISPLAY_GetDeviceBitmap(tela, &bitmapDDB) != SUCCESS)
        goto falhou;

    if (IBITMAP_QueryInterface(bitmapDDB, AEECLSID_DIB, (void**)&dib) != SUCCESS)
    {
        IBITMAP_Release(bitmapDDB);
        goto falhou;
    }

    g->surface = eglCreateWindowSurface(g->display, eglConfig,
                                        (NativeWindowType)dib, 0);
    IDIB_Release(dib);
    IBITMAP_Release(bitmapDDB);
    dib = NULL;
    bitmapDDB = NULL;

    if (g->surface == EGL_NO_SURFACE)
        goto falhou;

    /* Contexto unico, sem compartilhar: o driver nao suporta multiplos. */
    g->contexto = eglCreateContext(g->display, eglConfig, 0, 0);
    if (g->contexto == EGL_NO_CONTEXT)
        goto falhou;

    if (!eglMakeCurrent(g->display, g->surface, g->surface, g->contexto))
        goto falhou;

    /* Tamanho medido do device (ISHELL_GetDeviceInfo retorna void neste
     * SDK: confere o que ele preencheu, com fallback 640x480). */
    ISHELL_GetDeviceInfo(shell, &di);
    g->largura = (int)di.cxScreen;
    g->altura  = (int)di.cyScreen;
    if (g->largura <= 0 || g->altura <= 0)
    {
        g->largura = 640;
        g->altura  = 480;
    }

    /* Informa o motor uma vez (sem resize dinamico nesta fase minima). */
    Sa1Hello_Resize(g->largura, g->altura);
    return TRUE;

falhou:
    sa1_egl_encerrar(g);
    return FALSE;
}

void sa1_egl_encerrar(TSa1Egl* g)
{
    if (!g)
        return;

    if (g->contexto)
    {
        eglMakeCurrent(g->display, 0, 0, 0);
        eglDestroyContext(g->display, g->contexto);
    }
    if (g->surface)
        eglDestroySurface(g->display, g->surface);
    if (g->display)
        eglTerminate(g->display);

    g->contexto = 0;
    g->surface  = 0;
    g->display  = 0;

    /* Inverso da criacao: GLES_Release antes de EGL_Release. */
    GLES_Release();
    EGL_Release();
}

void sa1_egl_swap(TSa1Egl* g)
{
    eglSwapBuffers(g->display, g->surface);
}

/* Sa1Log — destino do printf do core (diagnostico AuditarEstado/traca).
 * Sem libc nao ha vsnprintf: formatador minimo (%d %u %x %s %c %%,
 * largura numerica opcional como em "%6d"), resto sai literal. */
#include <stdarg.h>

static char *pon_num(char *o, char *fim, unsigned int v, int base,
                     int largura, int negativo)
{
    char tmp[32];
    int n = 0, i;
    const char *dig = "0123456789abcdef";

    if (negativo)
        v = (unsigned int)(-(int)v);
    do
    {
        tmp[n++] = dig[v % (unsigned int)base];
        v /= (unsigned int)base;
    } while (v && n < (int)sizeof(tmp));
    if (negativo)
        tmp[n++] = '-';
    while (n < largura && o < fim)
    {
        *o++ = ' ';
        largura--;
    }
    for (i = n - 1; i >= 0 && o < fim; i--)
        *o++ = tmp[i];
    return o;
}

void Sa1Log(const char *fmt, ...)
{
    static char buf[512];
    char *o = buf, *fim = buf + sizeof(buf) - 1;
    va_list ap;
    int largura;

    va_start(ap, fmt);
    while (*fmt && o < fim)
    {
        if (*fmt != '%')
        {
            *o++ = *fmt++;
            continue;
        }
        fmt++;
        largura = 0;
        while (*fmt >= '0' && *fmt <= '9')
            largura = largura * 10 + (*fmt++ - '0');
        switch (*fmt)
        {
        case 'd':
            o = pon_num(o, fim, (unsigned int)va_arg(ap, int), 10,
                        largura, 1);
            break;
        case 'u':
        case 'x':
            o = pon_num(o, fim, va_arg(ap, unsigned int),
                        *fmt == 'x' ? 16 : 10, largura, 0);
            break;
        case 's':
        {
            const char *s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            while (*s && o < fim)
                *o++ = *s++;
            break;
        }
        case 'c':
            if (o < fim)
                *o++ = (char)va_arg(ap, int);
            break;
        case '%':
            if (o < fim)
                *o++ = '%';
            break;
        default:
            if (o < fim)
                *o++ = '%';
            if (o < fim && *fmt)
                *o++ = *fmt;
            break;
        }
        if (*fmt)
            fmt++;
    }
    *o = 0;
    va_end(ap);
    DBGPRINTF("%s", buf);
}
