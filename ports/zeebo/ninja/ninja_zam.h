/* ninja_zam.h — port ARM dos helpers do non-matching do decomp.
 *
 * O original (sdk/include/ninja_zam.h) implementa os mesmos helpers com
 * asm/intrínsecos SH-4 (FIPR/FSCA/FSRRA e XMTRX). Aqui a semântica vem do
 * backend **software** do SH4ZAM (MIT): mesmas operações, mesmas convenções,
 * sem asm. Os corpos abaixo são os do original, trocando só o que era
 * específico do SH-4:
 *
 *   zam_fipr / zam_fipr_self  -> produto interno em C
 *   _builtin_fsrra            -> shz_inv_sqrtf_fsrra (SW)
 *   zam_fsca                  -> shz_sincosu16 (SW)
 *   *_transform_vec4_sh4      -> shz_mat4x4_transform_vec4 (SW; mesma conta)
 *
 * Diferença de fidelidade: o FIPR do SH-4 arredonda de um jeito próprio; o
 * SW soma em C. Isso muda bits, não a geometria. Ver docs/zeebo-port-plan.md.
 */
#ifndef NINJA_ZAM_H
#define NINJA_ZAM_H

#include <assert.h>
#include <math.h>
#include <stdint.h>

#include <sh4zam/shz_xmtrx.h>
#include <sh4zam/shz_matrix.h>
#include <sh4zam/shz_scalar.h>
#include <sh4zam/shz_trig.h>
#include <sh4zam/shz_vector.h>

/* o original usa isto para checar tipos/contratos; no port é só documentação */
#define ZAM_ASSERT_COMPATIBLE(fn) ((void)0)

/* estado da pilha de matrizes do Ninja, definido pelo jogo (njSystem) */
extern Sint32 _max_stack_size_;
extern Sint32 _stack_size_;
extern NJS_MATRIX *_nj_current_matrix_ptr_;

ALWAYS_INLINE shz_vec3_t zam_load_vec3(const NJS_POINT3 *p)
{
    return shz_vec3_init(p->x, p->y, p->z);
}

ALWAYS_INLINE shz_vec4_t zam_load_vec4(const NJS_POINT3 *p, Float w)
{
    return shz_vec4_init(p->x, p->y, p->z, w);
}

ALWAYS_INLINE void zam_store_vec3(NJS_POINT3 *p, shz_vec3_t v)
{
    shz_vec3_deref(p) = v;
}

ALWAYS_INLINE Float zam_fipr(shz_vec4_t a, shz_vec4_t b)
{
    return shz_vec4_dot(a, b);
}

ALWAYS_INLINE Float zam_fipr_self(Float x, Float y, Float z, Float w)
{
    return x * x + y * y + z * z + w * w;
}

ALWAYS_INLINE Float zam_squared_length(shz_vec3_t v)
{
    return zam_fipr_self(v.x, v.y, v.z, 0.0F);
}

ALWAYS_INLINE Float _builtin_fsrra(Float n)
{
    return shz_inv_sqrtf_fsrra(n);
}

ALWAYS_INLINE shz_sincos_t zam_fsca(Angle ang)
{
    return shz_sincosu16((uint16_t)ang);
}

ALWAYS_INLINE void _builtin_fsca(Angle ang, Float *sin, Float *cos)
{
    shz_sincos_t pair = zam_fsca(ang);

    *sin = pair.sin;
    *cos = pair.cos;
}

ALWAYS_INLINE Bool zam_njPushMatrix(NJS_MATRIX *m)
{
    NJS_MATRIX *top = _nj_current_matrix_ptr_;

    if (_stack_size_ >= _max_stack_size_) {
        return FALSE;
    }
    _stack_size_++;
    shz_xmtrx_store_4x4((shz_mat4x4_t *)top);
    _nj_current_matrix_ptr_ = top + 1;
    if (m == NULL) {
        shz_xmtrx_store_4x4((shz_mat4x4_t *)(top + 1));
    } else {
        shz_xmtrx_load_unaligned_4x4(*m);
    }
    return TRUE;
}
#define njPushMatrix(m) zam_njPushMatrix(m)

ALWAYS_INLINE Bool zam_njPopMatrix(Uint32 n)
{
    Uint32 depth = (Uint32)_stack_size_ - n;

    if (depth == 0) {
        return FALSE;
    }
    _stack_size_ = (Sint32)depth;
    _nj_current_matrix_ptr_ -= n;
    shz_xmtrx_load_4x4((const shz_mat4x4_t *)_nj_current_matrix_ptr_);
    return TRUE;
}
#define njPopMatrix(n) zam_njPopMatrix(n)

ALWAYS_INLINE void zam_njTranslate(NJS_MATRIX *m, Float x, Float y, Float z)
{
    shz_vec4_t moved;

    if (m == NULL) {
        shz_xmtrx_translate(x, y, z);
        return;
    }
    moved = shz_mat4x4_transform_vec4((const shz_mat4x4_t *)m,
                                      shz_vec4_init(x, y, z, 1.0F));
    shz_vec4_deref(&(*m)[12]) = moved;
}
#define njTranslate(m, x, y, z) zam_njTranslate(m, x, y, z)

ALWAYS_INLINE void zam_njTranslateV(NJS_MATRIX *m, NJS_VECTOR *v)
{
    shz_vec3_t by = zam_load_vec3(v);

    zam_njTranslate(m, by.x, by.y, by.z);
}
#define njTranslateV(m, v) zam_njTranslateV(m, v)

ALWAYS_INLINE void zam_njScale(NJS_MATRIX *m, Float sx, Float sy, Float sz)
{
    if (m == NULL) {
        shz_xmtrx_scale(sx, sy, sz);
        return;
    }
    shz_vec4_deref(&(*m)[0]) = shz_vec4_scale(shz_vec4_deref(&(*m)[0]), sx);
    shz_vec4_deref(&(*m)[4]) = shz_vec4_scale(shz_vec4_deref(&(*m)[4]), sy);
    shz_vec4_deref(&(*m)[8]) = shz_vec4_scale(shz_vec4_deref(&(*m)[8]), sz);
}
#define njScale(m, x, y, z) zam_njScale(m, x, y, z)

ALWAYS_INLINE void zam_njScaleV(NJS_MATRIX *m, NJS_VECTOR *v)
{
    shz_vec3_t by = zam_load_vec3(v);

    zam_njScale(m, by.x, by.y, by.z);
}
#define njScaleV(m, v) zam_njScaleV(m, v)

ALWAYS_INLINE void zam_njCalcPoint(NJS_MATRIX *m, NJS_POINT3 *ps,
                                   NJS_POINT3 *pd)
{
    shz_vec4_t point = zam_load_vec4(ps, 1.0F);

    if (m == NULL) {
        point = shz_xmtrx_transform_vec4(point);
    } else {
        point = shz_mat4x4_transform_vec4((const shz_mat4x4_t *)m, point);
    }
    zam_store_vec3(pd, point.xyz);
}
#define njCalcPoint(m, ps, pd) zam_njCalcPoint(m, ps, pd)

ALWAYS_INLINE void zam_njGetTranslation(NJS_MATRIX *m, NJS_POINT3 *p)
{
    if (m == NULL) {
        zam_store_vec3(p, shz_xmtrx_get_translation());
    } else {
        zam_store_vec3(p, shz_vec3_init((*m)[12], (*m)[13], (*m)[14]));
    }
}
#define njGetTranslation(m, p) zam_njGetTranslation(m, p)

ALWAYS_INLINE Float zam_njInnerProduct(NJS_VECTOR *v1, NJS_VECTOR *v2)
{
    return zam_fipr(zam_load_vec4(v2, 0.0F), zam_load_vec4(v1, 0.0F));
}
#define njInnerProduct(v1, v2) zam_njInnerProduct(v1, v2)

ALWAYS_INLINE Float zam_njScalor(NJS_VECTOR *v)
{
    return __builtin_sqrtf(zam_squared_length(zam_load_vec3(v)));
}
#define njScalor(v) zam_njScalor(v)

ALWAYS_INLINE Float zam_njScalor2(NJS_VECTOR *v)
{
    return zam_squared_length(zam_load_vec3(v));
}
#define njScalor2(v) zam_njScalor2(v)

ALWAYS_INLINE Float zam_njUnitVector(NJS_VECTOR *v)
{
    shz_vec3_t unscaled = zam_load_vec3(v);
    Float squared = zam_squared_length(unscaled);
    Float scale = _builtin_fsrra(squared);

    zam_store_vec3(v, shz_vec3_scale(unscaled, scale));
    return squared * scale;
}
#define njUnitVector(v) zam_njUnitVector(v)

ALWAYS_INLINE Float zam_njSqrt(Float n)
{
    return n < 0.0F ? -__builtin_sqrtf(-n) : __builtin_sqrtf(n);
}
#define njSqrt(n) zam_njSqrt(n)

ALWAYS_INLINE Float zam_njInvertSqrt(Float n)
{
    return n < 0.0F ? -_builtin_fsrra(-n) : _builtin_fsrra(n);
}
#define njInvertSqrt(n) zam_njInvertSqrt(n)

ALWAYS_INLINE Float zam_njSin(Angle n)
{
    return zam_fsca(n).sin;
}
#define njSin(n) zam_njSin(n)

ALWAYS_INLINE Float zam_njCos(Angle n)
{
    return zam_fsca(n).cos;
}
#define njCos(n) zam_njCos(n)

ALWAYS_INLINE Float zam_njTan(Angle n)
{
    shz_sincos_t pair = zam_fsca(n);

    return pair.sin / pair.cos;
}
#define njTan(n) zam_njTan(n)

ALWAYS_INLINE void zam_copy_four(Uint32 *to, const Uint32 *from)
{
    Uint32 a = from[0];
    Uint32 b = from[1];
    Uint32 c = from[2];
    Uint32 d = from[3];

    to[0] = a;
    to[1] = b;
    to[2] = c;
    to[3] = d;
}

ALWAYS_INLINE void zam_copy_matrix(Float *md, const Float *ms)
{
    zam_copy_four((Uint32 *)md, (const Uint32 *)ms);
    zam_copy_four((Uint32 *)md + 4, (const Uint32 *)ms + 4);
    zam_copy_four((Uint32 *)md + 8, (const Uint32 *)ms + 8);
    zam_copy_four((Uint32 *)md + 12, (const Uint32 *)ms + 12);
}

enum zam_axis { ZAM_X, ZAM_Y, ZAM_Z };

/* XMTRX = XMTRX * R(ang): so duas colunas mudam (p' = c p + s q, q' = c q - s p). O
 * shz_xmtrx_rotate_*_u16 montava a matriz de rotacao e fazia uma multiplicacao 4x4 inteira
 * (quatro FTRV de 16 multiplicacoes e 12 somas em soft-float); aqui sao 8 multiplicacoes e 4
 * somas por linha. Conferido no host contra o SHZAM, bit a bit, nos tres eixos */
ALWAYS_INLINE void zam_turn(enum zam_axis axis, Angle ang)
{
    shz_sincos_t sc = shz_sincosu16((uint16_t)ang);
    float *m = shz_xmtrx_state_()->elem;
    unsigned p = axis == ZAM_X ? 1 : (axis == ZAM_Y ? 2 : 0);
    unsigned q = axis == ZAM_X ? 2 : (axis == ZAM_Y ? 0 : 1);
    unsigned i;

    for (i = 0; i < 4; i++) {
        float a = m[p * 4 + i];
        float b = m[q * 4 + i];

        m[p * 4 + i] = sc.cos * a + sc.sin * b;
        m[q * 4 + i] = sc.cos * b - sc.sin * a;
    }
}

ALWAYS_INLINE void zam_turn_rows(NJS_MATRIX *m, enum zam_axis axis, Angle ang)
{
    unsigned first = axis == ZAM_X ? 1 : 0;
    unsigned second = axis == ZAM_Z ? 1 : 2;
    shz_vec4_t *row_first = &shz_vec4_deref(&(*m)[4 * first]);
    shz_vec4_t *row_second = &shz_vec4_deref(&(*m)[4 * second]);
    shz_vec4_t keep_first = shz_xmtrx_read_col(first);
    shz_vec4_t keep_second = shz_xmtrx_read_col(second);

    shz_xmtrx_write_col(first, *row_first);
    shz_xmtrx_write_col(second, *row_second);
    zam_turn(axis, ang);
    *row_first = shz_xmtrx_read_col(first);
    *row_second = shz_xmtrx_read_col(second);
    shz_xmtrx_write_col(first, keep_first);
    shz_xmtrx_write_col(second, keep_second);
}

#endif /* NINJA_ZAM_H */
