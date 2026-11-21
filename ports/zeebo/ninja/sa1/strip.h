/* sa1/strip.h — shim Zeebo do envio de vértices do decomp.
 *
 * Substitui engine/dc/sa1/strip.h. A API é a mesma que engine/dc/sa1/
 * polygon.h espera (sa1_live_sq e sa1_send_vertex), mas em vez de escrever
 * na store queue do PowerVR o vértice entra no sink GLES1 (zstrip).
 *
 * A convenção é preservada: x, y em pixels de tela; z é 1/profundidade. */
#ifndef SA1_STRIP_H
#define SA1_STRIP_H

#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "zstrip.h"

/* o decomp mantém este ponteiro num registrador entre abrir e fechar o
 * strip; aqui ele não carrega endereço nenhum. */
static Uint32 *sa1_live_sq __attribute__((unused));

ALWAYS_INLINE Uint32 *sa1_send_vertex(Uint32 *sq, Uint32 control, Float x,
                                      Float y, Float z, Int textured, Float u,
                                      Float v, Uint32 color)
{
    (void)control;
    if (textured) {
        zstrip_vertex(x, y, z, u, v, color);
    } else {
        zstrip_vertex(x, y, z, 0.0F, 0.0F, color);
    }
    return sq;
}

/* o resto de engine/dc/sa1/strip.h não é usado por njDraw.c/polygon.h, que
 * são o recorte do Z1. */
struct sa1_end {
    Float x, y, u, v;
    Uint32 color;
};

#endif /* SA1_STRIP_H */
