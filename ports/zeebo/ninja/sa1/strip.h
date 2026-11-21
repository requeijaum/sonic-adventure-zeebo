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

/* NJS_COLOR's tex.u e tex.v contam 256 por lado da textura */
#define SA1_UV_SCALE (1.0F / 256.0F)
/* meia largura de uma linha 2D, em pixels, como as do Ninja */
#define SA1_HALF_WIDTH 0.525F

/* o decomp mantém este ponteiro num registrador entre abrir e fechar o
 * strip; aqui ele não carrega endereço nenhum. */
static Uint32 *sa1_live_sq __attribute__((unused));

ALWAYS_INLINE Uint32 *sa1_send_vertex(Uint32 *sq, Uint32 control, Float x,
                                      Float y, Float z, Int textured, Float u,
                                      Float v, Uint32 color)
{
    if (textured) {
        zstrip_vertex(x, y, z, u, v, color);
    } else {
        zstrip_vertex(x, y, z, 0.0F, 0.0F, color);
    }
    /* o PVR fecha o strip no vértice com ENDOFSTRIP; o seguinte abre outro */
    if (control == KM_VERTEXPARAM_ENDOFSTRIP) {
        zstrip_end();
        zstrip_begin();
    }
    return sq;
}

/* o resto de engine/dc/sa1/strip.h, igual ao decomp (só chama
 * sa1_send_vertex acima) */
struct sa1_end {
    Float x, y, u, v;
    Uint32 color;
};

ALWAYS_INLINE void sa1_end_at(struct sa1_end *end, const NJS_POINT2COL *p,
                              Int i, Int textured)
{
    end->x = p->p[i].x;
    end->y = p->p[i].y;
    end->u = textured ? p->tex[i].tex.u * SA1_UV_SCALE : 0.0F;
    end->v = textured ? p->tex[i].tex.v * SA1_UV_SCALE : 0.0F;
    end->color = p->col[i].color;
}

ALWAYS_INLINE Uint32 *sa1_send_edge(Uint32 *sq, const struct sa1_end *a,
                                    const struct sa1_end *b, Float z,
                                    Int textured)
{
    Float dx = b->x - a->x;
    Float dy = b->y - a->y;
    Float length2 = (dx * dx) + (dy * dy);
    Float inv_length;
    Float side_x;
    Float side_y;

    if (!(length2 > 0.0F)) {
        return sq;
    }
    inv_length = 1.0F / _builtin_sqrtf(length2);
    side_x = dy * inv_length * SA1_HALF_WIDTH;
    side_y = -(dx * inv_length * SA1_HALF_WIDTH);
    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, a->x + side_x,
                         a->y + side_y, z, textured, a->u, a->v, a->color);
    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, a->x - side_x,
                         a->y - side_y, z, textured, a->u, a->v, a->color);
    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, b->x + side_x,
                         b->y + side_y, z, textured, b->u, b->v, b->color);
    return sa1_send_vertex(sq, KM_VERTEXPARAM_ENDOFSTRIP, b->x - side_x,
                           b->y - side_y, z, textured, b->u, b->v, b->color);
}

/* o 1/z de um draw 2D na prioridade pri */
ALWAYS_INLINE Float sa1_rhw_2d(Float pri)
{
    return pri < -1.0F ? -1.0F / pri : 1.0F;
}

#endif /* SA1_STRIP_H */
