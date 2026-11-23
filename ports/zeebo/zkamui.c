/* zkamui.c — a superfície de Kamui que o jogo chama, sem a biblioteca Kamui.
 *
 * O jogo linka a Kamui real (sdk/ninja/*_kamui_lib.c) no Dreamcast: ela
 * inicializa o PowerVR, instala interrupções e escreve listas de vértices.
 * No Zeebo o desenho vai direto ao GLES1 (zstrip) e o fim de frame é o
 * eglSwapBuffers do timer do applet, então essas chamadas ficam sem efeito.
 * Cada stub devolve 0 (KMSTATUS_SUCCESS / ponteiro nulo). Onde o efeito
 * existe de verdade no GLES1 (fog, clipping de usuário, cull), o stub está
 * marcado TODO e é implementado quando o jogo chegar nele.
 *
 * Sem protótipos de propósito: o valor de retorno é 0 e os argumentos são
 * ignorados, então a assinatura exata não importa para a ABI do ARM. */

void zlog(const char *what, const char *name, int value);
#define STUB(name) int name() { zlog("km", #name, (int)__builtin_return_address(0)); return 0; }

/* fim de frame e estado de render */
STUB(kmRender)              /* o desenho já foi ao GLES1 */
/* o fim de quadro do jogo (frame_wait chama logo após o desenho): marca o
 * quadro fechado, e o timer do applet apresenta. o swap é dele */
extern int zframe_end;

int kmFlipFrameBuffer(void)
{
    zframe_end = 1;
    return 0;
}
STUB(kmResetPointer)
STUB(kmRenderTexture)       /* TODO: render-para-textura (reflexos) */
STUB(kmDiscardVertexBuffer)
STUB(kmSetEndOfVertexCallback)
STUB(kmSetEORCallback)
STUB(kmSetWaitVsyncCount)
STUB(kmQueryFinishLastTextureDMA)
STUB(kmProcessVertexRenderState)
STUB(kmSetVertexRenderState)

/* inicialização e tela */
STUB(kmInitDevice)
STUB(kmSetSystemConfiguration)
STUB(kmSetDisplayMode)
STUB(kmStopDisplayFrameBuffer)
STUB(kmAdjustDisplayCenter)
STUB(kmSetStrideWidth)
STUB(kmSetBorderColor)
STUB(kmSetBackGroundPlane)
STUB(kmSetBackGroundRenderState)
STUB(kmfGetInternalSystemMetrics)
STUB(kmfSetFatalErrorCallback)
STUB(kmfSetTimeOutCount)
STUB(kmiDeviceExtension)
STUB(kmiIntStatisticData)
STUB(kmiInitInterrupt)
STUB(kmiPurgeCache)

/* estados que o GLES1 faz de outro jeito */
STUB(kmSetCheapShadowMode)
STUB(kmSetColorClampValue)
STUB(kmSetCullingRegister)
STUB(kmSetPaletteMode)
STUB(kmSetPixelClipping)
STUB(kmSetUserClipping)     /* TODO: glScissor */
STUB(kmSetFogDensity)       /* TODO: GL_FOG */
STUB(kmSetFogTable)
STUB(kmSetFogTableColor)

/* texturas: kmCreateTextureSurface/kmLoadTexture/kmFreeTexture em ztexture.c */
STUB(kmGarbageCollectTexture)
STUB(kmGetFreeTextureMem)
STUB(kmGetTexture)
STUB(kmLoadTexturePart)

/* cópia de memória de vídeo: a "VRAM" é memória comum */
void *kmiMemCopy4(void *dst, void *src, unsigned int size)
{
    unsigned int *d = (unsigned int *)dst;
    unsigned int *s = (unsigned int *)src;
    unsigned int n = size / 4;

    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

void *kmiMemCopy8(void *dst, void *src, unsigned int size)
{
    return kmiMemCopy4(dst, src, size);
}

/* kmiGetProfile*: sem arquivo de perfil, vale o valor padrão (3o argumento) */
unsigned int kmi_profile_unsigned_fallback(const char *section, const char *key,
                                           unsigned int fallback)
{
    (void)section;
    (void)key;
    return fallback;
}
