/* njmodel_arm.c — os kernels do caminho genérico do Ninja em C.
 *
 * sdk/ninja/njModel.c guarda com #ifdef DREAMCAST os corpos de asm SH-4 que
 * projetam e iluminam os pontos de um modelo (CalcModelMatrix, loop1..A) e os
 * dois que escrevem o pacote na store queue (submit_work_vertex,
 * submit_packet). No build ARM eles ficam indefinidos e build/zeebo/game/
 * ztraps.c os trocava por no-ops: todo modelo saía com vértice lixo (o Sonic
 * desmontado), sem luz e sem bits de clip.
 *
 * Aqui cada kernel faz a mesma conta em C, na ordem do asm, e os dois de
 * envio entregam cada vértice ao zstrip (a store queue do port). A quebra de
 * strip (o pacote KM_VERTEXPARAM_ENDOFSTRIP) vira zstrip_end/zstrip_begin. */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include <sh4zam/shz_vector.h>
#include <sh4zam/shz_xmtrx.h>

#include "zstrip.h"

/* o que o njkm_zeebo.c guarda do strip corrente */
int znj_strip_textured(void);
void znj_break_strip(void);

extern Float _nj_vertex_offset_;

/* os layouts são os do njModel.c (o comentário lá descreve cada campo) */
struct model_vertex {
    union {
        Uint32 cmd;
        Uint32 clip;
    } control;
    Float sx, sy;
    Float oow; /* 1/z */
    NJS_POINT3 point; /* no espaço da vista */
    Uint32 reserve;
    union {
        struct {
            Float uv[2];
            union {
                Float component;
                Uint32 packed;
            } colors[2]; /* difusa empacotada, depois especular */
        } tex;
        NJS_ARGB col;
    } payload;
    NJS_VECTOR vector; /* normal no espaço da vista */
    Uint32 lit_key;
};

struct calc_work {
    Sint32 count;
    struct model_vertex *vertex;
    NJS_POINT3 *points;
    NJS_VECTOR *normals;
    Float scale_x, scale_y;
    Uint32 clip_and, clip_or;
    Float left, right, top, bottom;
};

struct clip_vertex {
    Uint32 cmd;
    Float sx, sy;
    Float oow;
    Float u, v;
    Uint32 color, offset;
    NJS_ARGB lit[2]; /* difusa, depois especular */
};

struct color_vertex {
    Uint32 cmd;
    Float sx, sy;
    Float oow;
    NJS_ARGB col;
};

/* os cantos dos clippers, 4 de cada (o bs do njModelGlobal.src) */
struct clip_vertex vtx2[4];
struct color_vertex vtx4[4];
NJS_ARGB _njm_col[3][2];
NJS_ARGB _njm_col2[2];

/* o cache do SH-4 não existe: a memória já é coerente. (restore_queue_cursor
 * fica com o stub do zshinobi.c: o port não usa a store queue como cursor) */
void writeback_line(void *address)
{
    (void)address;
}

/* dst = XMTRX * (src, 1) */
void CalcModelMatrix(NJS_POINT3 *dst, const NJS_POINT3 *src)
{
    shz_vec4_t out =
        shz_xmtrx_transform_vec4(shz_vec4_init(src->x, src->y, src->z, 1.0F));

    dst->x = out.x;
    dst->y = out.y;
    dst->z = out.z;
}

/* um ponto, como os loops 1 a A o escrevem.
 * has_normal: lê work->normals; normalize: divide a normal pelo módulo;
 * has_offset: soma a normal (já na vista) vezes _nj_vertex_offset_;
 * clip: calcula os bits de clip e acumula em clip_and/clip_or */
static void calc_point(struct calc_work *work, Sint32 i, int has_normal,
                       int normalize, int has_offset, int clip)
{
    struct model_vertex *v = &work->vertex[i];
    NJS_POINT3 p = work->points[i];
    shz_vec4_t t;
    Uint32 flags = 0;

    if (has_normal) {
        const NJS_VECTOR *n = &work->normals[i];
        shz_vec4_t nv =
            shz_xmtrx_transform_vec4(shz_vec4_init(n->x, n->y, n->z, 0.0F));
        float nx = nv.x, ny = nv.y, nz = nv.z;

        if (normalize) {
            float len2 = nx * nx + ny * ny + nz * nz;
            float inv = len2 > 0.0F ? 1.0F / __builtin_sqrtf(len2) : 0.0F;

            v->vector.x = nx * inv;
            v->vector.y = ny * inv;
            v->vector.z = nz * inv;
        } else {
            v->vector.x = nx;
            v->vector.y = ny;
            v->vector.z = nz;
        }
        if (has_offset) {
            p.x += nx * _nj_vertex_offset_;
            p.y += ny * _nj_vertex_offset_;
            p.z += nz * _nj_vertex_offset_;
        }
    }

    t = shz_xmtrx_transform_vec4(shz_vec4_init(p.x, p.y, p.z, 1.0F));
    {
        float z = t.z;
        float inv = 1.0F / z;

        v->point.x = t.x;
        v->point.y = t.y;
        v->point.z = z;
        v->oow = inv;
        v->sx = t.x * work->scale_x * inv + _nj_screen_.cx;
        v->sy = t.y * work->scale_y * inv + _nj_screen_.cy;
        if (clip) {
            if (!(_nj_near_clip_ > z)) {
                flags = (Uint32)(v->sy > work->bottom) << 5 |
                        (Uint32)(work->top > v->sy) << 4 |
                        (Uint32)(v->sx > work->right) << 3 |
                        (Uint32)(work->left > v->sx) << 2;
            }
            flags |= (z > _nj_far_clip_ ? 2u : 0u) |
                     (_nj_near_clip_ > z ? 1u : 0u);
            work->clip_and &= flags;
            work->clip_or |= flags;
        }
    }
    v->control.clip = flags;
    v->lit_key = 0xffffffffu;
}

#define LOOP(fn, has_normal, normalize, has_offset, clip)                     \
    void fn(struct calc_work *work)                                           \
    {                                                                         \
        Sint32 i;                                                             \
                                                                              \
        for (i = 0; i < work->count; i++) {                                   \
            calc_point(work, i, has_normal, normalize, has_offset, clip);     \
        }                                                                     \
    }

LOOP(loop1_body, 1, 1, 1, 0)
LOOP(loop2_body, 1, 0, 1, 0)
LOOP(loop3_body, 1, 1, 0, 0)
LOOP(loop4_body, 1, 0, 0, 0)
LOOP(loop5_body, 0, 0, 0, 0)
LOOP(loop6_body, 1, 1, 1, 1)
LOOP(loop7_body, 1, 0, 1, 1)
LOOP(loop8_body, 1, 1, 0, 1)
LOOP(loop9_body, 1, 0, 0, 1)
LOOP(loopa_body, 0, 0, 0, 1)

/* --- cores empacotadas (ARGB, como o zstrip quer) --- */

static float clamp01(float v)
{
    return v < 0.0F ? 0.0F : v > 1.0F ? 1.0F : v;
}

static Uint32 pack_argb(Float a, Float r, Float g, Float b)
{
    return ((Uint32)(clamp01(a) * 255.0F) << 24) |
           ((Uint32)(clamp01(r) * 255.0F) << 16) |
           ((Uint32)(clamp01(g) * 255.0F) << 8) |
           (Uint32)(clamp01(b) * 255.0F);
}

Uint32 pack_argb_body(Float a, Float r, Float g, Float b)
{
    return pack_argb(a, r, g, b);
}

void unpack_argb_body(Uint32 color, NJS_ARGB *dst)
{
    const Float scale = 1.0F / 255.0F;

    dst->a = (Float)((color >> 24) & 0xffu) * scale;
    dst->r = (Float)((color >> 16) & 0xffu) * scale;
    dst->g = (Float)((color >> 8) & 0xffu) * scale;
    dst->b = (Float)(color & 0xffu) * scale;
}

/* ponto . normal, como o FIPR FV4,FV0 do original */
Float view_point_normal_dot(NJS_POINT3 *point, NJS_VECTOR *normal)
{
    return point->x * normal->x + point->y * normal->y +
           point->z * normal->z;
}

/* c . (a x b); o sinal é a orientação do triângulo visto do olho */
Float view_triangle_facing(NJS_POINT3 *b, NJS_POINT3 *a, NJS_POINT3 *c)
{
    float cx = a->y * b->z - a->z * b->y;
    float cy = a->z * b->x - a->x * b->z;
    float cz = a->x * b->y - a->y * b->x;

    return c->y * cx + c->z * cy + c->x * cz;
}

/* --- envio: cada pacote é um vértice do zstrip --- */

static void emit_work(const struct model_vertex *v)
{
    if (znj_strip_textured()) {
        zstrip_vertex(v->sx, v->sy, v->oow, v->payload.tex.uv[0],
                      v->payload.tex.uv[1], v->payload.tex.colors[0].packed);
    } else {
        const NJS_ARGB *c = &v->payload.col;

        zstrip_vertex(v->sx, v->sy, v->oow, 0.0F, 0.0F,
                      pack_argb(c->a, c->r, c->g, c->b));
    }
}

void submit_work_vertex(const struct model_vertex *vertex)
{
    emit_work(vertex);
    if (vertex->control.cmd == KM_VERTEXPARAM_ENDOFSTRIP) {
        znj_break_strip();
    }
}

void submit_packet(void *block)
{
    if (znj_strip_textured()) {
        const struct clip_vertex *c = block;

        zstrip_vertex(c->sx, c->sy, c->oow, c->u, c->v, c->color);
        if (c->cmd == KM_VERTEXPARAM_ENDOFSTRIP) {
            znj_break_strip();
        }
    } else {
        const struct color_vertex *c = block;

        zstrip_vertex(c->sx, c->sy, c->oow, 0.0F, 0.0F,
                      pack_argb(c->col.a, c->col.r, c->col.g, c->col.b));
        if (c->cmd == KM_VERTEXPARAM_ENDOFSTRIP) {
            znj_break_strip();
        }
    }
}
