/* As operacoes de FPU do SH-4 que o jogo le (FTRV, FIPR, FMAC e as comparacoes), no ARM sem FPU.
 * engine/numeric/fast.c da os bits exatos do console, em inteiros: ~16% das instrucoes do 3D
 * (dot, sa_ftrv, sa_fipr). O port nao precisa dos bits do console: aqui sao as contas diretas
 * em float (soft-float do ARM). Diferem do console no ultimo bit em casos raros (FMAC e
 * FTRV do console acumulam sem arredondar entre os produtos) */
#include <stdint.h>

float sa_fmac(float f0, float fm, float fn)
{
    return f0 * fm + fn;
}

float sa_fipr(const float a[4], const float b[4])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

void sa_ftrv(const float m[16], const float v[4], float out[4])
{
    float v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];
    union {
        float f;
        uint32_t u;
    } w;
    float r0, r1, r2, r3;

    w.f = v3;
    r0 = m[0] * v0 + m[4] * v1 + m[8] * v2;
    r1 = m[1] * v0 + m[5] * v1 + m[9] * v2;
    r2 = m[2] * v0 + m[6] * v1 + m[10] * v2;
    r3 = m[3] * v0 + m[7] * v1 + m[11] * v2;
    if (w.u == 0x3f800000u) { /* ponto: w = 1 */
        r0 += m[12];
        r1 += m[13];
        r2 += m[14];
        r3 += m[15];
    } else if ((w.u << 1) != 0) {
        r0 += m[12] * v3;
        r1 += m[13] * v3;
        r2 += m[14] * v3;
        r3 += m[15] * v3;
    } /* w = 0 (vetor): nada a somar */
    out[0] = r0;
    out[1] = r1;
    out[2] = r2;
    out[3] = r3;
}

int32_t sa_fcmp_gt(float a, float b)
{
    return a > b;
}

int32_t sa_fcmp_eq(float a, float b)
{
    return a == b;
}

float sqrtf(float x);

/* FSQRT do console e IEEE (raiz exata); o fast.c a refaz em inteiros sobre um chute de sqrtf */
float sa_fsqrt(float a)
{
    union {
        float f;
        uint32_t u;
    } r;

    if (a < 0.0f) {
        r.u = 0x7fc00000u; /* NaN */
        return r.f;
    }
    return sqrtf(a);
}

/* FSRRA: 1/raiz, o console aproxima; aqui 1/sqrtf */
float sa_fsrra(float a)
{
    union {
        float f;
        uint32_t u;
    } r;

    if (a > 0.0f) {
        return 1.0f / sqrtf(a);
    }
    r.u = a == 0.0f ? 0x7f800000u : 0x7fc00000u; /* +inf ou NaN */
    return r.f;
}

/* njCalcPoint / njCalcVector com a matriz corrente (m == NULL), os mais chamados do 3D (a
 * colisao transforma os pontos de cada malha por quadro): o asm traduzido do SH-4 passava por
 * enter/narrow/leave, pela copia dos bancos de registradores e por um FTRV de 28 operacoes em
 * soft-float; aqui sao 9 produtos. Com matriz explicita ou qualquer outra coisa, o original
 * (game.mk o renomeia para *_asm) */
float *native_xmtrx(void);
void njCalcPoint_asm(void *m, void *src, void *dst);
void njCalcVector_asm(void *m, void *src, void *dst);

typedef struct {
    float x, y, z;
} zp3_t;

void njCalcPoint(void *m, zp3_t *src, zp3_t *dst)
{
    const float *x;
    float px, py, pz;

    if (m != 0) {
        njCalcPoint_asm(m, src, dst);
        return;
    }
    x = native_xmtrx();
    px = src->x;
    py = src->y;
    pz = src->z;
    dst->x = x[0] * px + x[4] * py + x[8] * pz + x[12];
    dst->y = x[1] * px + x[5] * py + x[9] * pz + x[13];
    dst->z = x[2] * px + x[6] * py + x[10] * pz + x[14];
}

void njCalcVector(void *m, zp3_t *src, zp3_t *dst)
{
    const float *x;
    float px, py, pz;

    if (m != 0) {
        njCalcVector_asm(m, src, dst);
        return;
    }
    x = native_xmtrx();
    px = src->x;
    py = src->y;
    pz = src->z;
    dst->x = x[0] * px + x[4] * py + x[8] * pz;
    dst->y = x[1] * px + x[5] * py + x[9] * pz;
    dst->z = x[2] * px + x[6] * py + x[10] * pz;
}
