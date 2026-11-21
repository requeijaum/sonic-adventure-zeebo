/* sa1hello — contexto EGL + superficie de janela no BREW.
 *
 * Bring-up pelo caminho do Developer Guide sec. 7.3 (wrappers EGL_1x/GLES_1x,
 * nao o IGL legado): EGL_Init, depois GLES_Init; display do EGL e o IDisplay
 * do console; janela e o DIB do bitmap do painel; um unico contexto para a
 * vida do applet (o driver nao suporta multiplos).
 */

#ifndef SA1_BREW_EGL_H
#define SA1_BREW_EGL_H

#include "AEE.h"
#include "AEEShell.h"
#include "AEEDisp.h"
#include "gles/egl.h"

typedef struct _TSa1Egl
{
    EGLDisplay display;
    EGLSurface surface;
    EGLContext contexto;
    int        largura;   /* medido do device, nao assumido */
    int        altura;
} TSa1Egl;

boolean sa1_egl_iniciar(TSa1Egl* g, IShell* shell, IDisplay* tela);
void    sa1_egl_encerrar(TSa1Egl* g);
void    sa1_egl_swap(TSa1Egl* g);

#endif /* SA1_BREW_EGL_H */
