/* zninja.c — cena de teste desenhada pelo njDrawTexture REAL do decomp.
 *
 * Compilado com DREAMCAST+NONMATCHING, o sdk/ninja/njDraw.c usa
 * sa1/polygon.h -> sa1/strip.h (nosso shim) e as entradas nj*km* (nosso
 * njkm_zeebo.c). Ou seja: este arquivo não desenha nada à mão; ele só monta
 * os vértices e chama Ninja, e o caminho inteiro do decomp termina no sink
 * GLES1. */

#include <ninja.h>
#include <sg_xpt.h>

#include "zstrip.h"

void zninja_frame(uint32_t tick)
{
    float drift = (float)(tick % 240) * 0.5f;
    NJS_TEXTURE_VTX opaque[4];
    NJS_TEXTURE_VTX trans[4];
    float x0 = 60.0f + drift;
    float y0 = 60.0f;
    float x1 = 300.0f + drift;
    float y1 = 240.0f;

    /* z é 1/profundidade: quanto maior, mais perto. */
    opaque[0].x = x0;
    opaque[0].y = y0;
    opaque[0].z = 0.30f;
    opaque[0].u = 0.0f;
    opaque[0].v = 0.0f;
    opaque[0].col = 0xffffffffu;
    opaque[1] = opaque[0];
    opaque[1].x = x1;
    opaque[1].u = 1.0f;
    opaque[2] = opaque[0];
    opaque[2].y = y1;
    opaque[2].v = 1.0f;
    opaque[3] = opaque[0];
    opaque[3].x = x1;
    opaque[3].y = y1;
    opaque[3].u = 1.0f;
    opaque[3].v = 1.0f;

    trans[0] = opaque[0];
    trans[0].x = 220.0f;
    trans[0].y = 160.0f;
    trans[0].z = 0.60f;
    trans[0].col = 0xc0ffffffu;
    trans[1] = trans[0];
    trans[1].x = 520.0f;
    trans[1].u = 1.0f;
    trans[2] = trans[0];
    trans[2].y = 400.0f;
    trans[2].v = 1.0f;
    trans[3] = trans[0];
    trans[3].x = 520.0f;
    trans[3].y = 400.0f;
    trans[3].u = 1.0f;
    trans[3].v = 1.0f;

    njDrawTexture(opaque, 4, 0, 0); /* global 0: 1a do PVM */
    njDrawTexture(trans, 4, 1, 1);  /* global 1: 2a do PVM */
}
