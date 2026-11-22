/* lt_arm.c — as calculadoras de luz do Ninja em C.
 *
 * sdk/ninja/njLtSrcs.c guarda com #ifdef DREAMCAST as 23 calculadoras
 * lt_rgb_XX, o lt_power_body e o lt_rgb_lights_body (asm SH-4: FIPR, FSRRA,
 * FMAC). No build ARM eles ficavam indefinidos e o ztraps.c os trocava por
 * no-ops: njCalculateLightsRGB devolvia cor zero e todo modelo saía com a cor
 * crua do material, sem luz.
 *
 * Cada função faz a conta do asm, na mesma ordem; tools/ltcmp (tests/lt_ref)
 * confere todas contra o asm traduzido por engine/tools/asm2c.py.
 *
 * Uma calculadora soma a parte de uma luz em color[0] (difusa, mais o alfa do
 * especular) e color[1] (r, g, b do especular). NJS_ARGB é {a, r, g, b}. */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

extern const Float fpow_tbl[17][256];
extern NJS_LIGHT *_nj_ptr_rgb_lt_[NJD_LIGHT_MAX];
extern NJS_ARGB *_nj_ptr_rgb_lt_argb_;
extern NJS_POINT3 *_nj_ptr_rgb_lt_pnt_;
extern NJS_VECTOR *_nj_ptr_rgb_lt_nml_;
typedef void lt_calc(NJS_LIGHT *light, NJS_ARGB *color, NJS_POINT3 *point,
                     NJS_VECTOR *normal);
extern lt_calc *_nj_rgb_lt_func_[NJD_LIGHT_MAX];

static float dot3(const NJS_VECTOR *a, const NJS_VECTOR *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

/* o FMAC do SH-4 soma sem arredondar o produto; em ARM sem FMA o fmaf do
 * port (ports/zeebo/ninja/fmaf.c) dá o mesmo resultado */
extern float fmaf(float, float, float);

/* color[0].rgb += k * mdif.rgb */
static void add_diffuse(const NJS_LIGHT *light, NJS_ARGB *color, float k)
{
    color[0].r = fmaf(k, light->ltcal.mdif.r, color[0].r);
    color[0].g = fmaf(k, light->ltcal.mdif.g, color[0].g);
    color[0].b = fmaf(k, light->ltcal.mdif.b, color[0].b);
}

/* color[1].rgb += s * mspc.rgb e color[0].a += s * mspc.a */
static void add_specular(const NJS_LIGHT *light, NJS_ARGB *color, float s)
{
    color[1].r = fmaf(s, light->ltcal.mspc.r, color[1].r);
    color[1].g = fmaf(s, light->ltcal.mspc.g, color[1].g);
    color[1].b = fmaf(s, light->ltcal.mspc.b, color[1].b);
    color[0].a = fmaf(s, light->ltcal.mspc.a, color[0].a);
}

/* o lt_power_body. passo 0: 1. passos 1 a 16: value elevado a 0,5; 1; 1,5; 2;
 * 3; 4; 6; 8; 12; 16; 24; 32; 48; 64; 96; 128; qualquer outro passo: 128.
 * O asm é uma cadeia de quadrados que cai de um rótulo no seguinte: o passo
 * par n entra em P(n) e eleva ao quadrado (n / 2) vezes até P2; o ímpar de 5
 * a 15 entra em P(n), eleva ao quadrado (n - 5) / 2 vezes e fecha em P5 com
 * value * v * v, onde v é o valor já elevado. Conferido por tests/lt_ref. */
float lt_power_body(float value, int step)
{
    float v = value;
    int n;

    if (step == 0) {
        return 1.0F;
    }
    if (step < 0 || step > 16) {
        step = 16;
    }
    if (step == 1) {
        return __builtin_sqrtf(value);
    }
    if (step == 2) {
        return value;
    }
    if (step == 3) {
        return value * __builtin_sqrtf(value);
    }
    if (step & 1) { /* 5, 7, ..., 15: value^(3 * 2^k), k = (passo - 5) / 2 */
        float cube;

        for (n = (step - 5) / 2; n > 0; n--) {
            v *= v;
        }
        cube = v * v * v;
        return cube;
    }
    for (n = step / 2 - 1; n > 0; n--) { /* 4, 6, ..., 16 */
        v *= v;
    }
    return v;
}

/* --- as calculadoras, por tipo de luz --- */

typedef NJS_LIGHT L;
typedef NJS_ARGB C;
typedef NJS_POINT3 P;
typedef NJS_VECTOR V;

/* o ponto de uma luz pontual: d = lpnt - point */
static void light_to(const L *l, const P *p, V *d)
{
    d->x = l->ltcal.lpnt.x - p->x;
    d->y = l->ltcal.lpnt.y - p->y;
    d->z = l->ltcal.lpnt.z - p->z;
}

/* 0x01, NJD_AMBIENT: soma mamb em color[0] */
void lt_rgb_01(L *l, C *c, P *p, V *n)
{
    (void)p; (void)n;
    c[0].r += l->ltcal.mamb.r;
    c[0].g += l->ltcal.mamb.g;
    c[0].b += l->ltcal.mamb.b;
}

/* 0x02, NJD_DIR_LIGHT: difusa -(n . lvctr) onde positivo */
void lt_rgb_02(L *l, C *c, P *p, V *n)
{
    float k = -dot3(n, &l->ltcal.lvctr);

    (void)p;
    if (k > 0.0F) {
        add_diffuse(l, c, k);
    }
}

/* 0x82 e o padrão: difusa (1 - n . lvctr) / 2 */
void lt_rgb_82(L *l, C *c, P *p, V *n)
{
    float d = dot3(n, &l->ltcal.lvctr);

    (void)p;
    add_diffuse(l, c, (1.0F - d) * 0.5F);
}

/* 0x04 e 0x84: luz pontual */
void lt_rgb_04(L *l, C *c, P *p, V *n)
{
    V d;
    float dd, nd, k;

    light_to(l, p, &d);
    nd = dot3(n, &d);
    dd = dot3(&d, &d);
    if (!(l->ltcal.frr > dd) || !(nd > 0.0F)) {
        return;
    }
    k = nd / __builtin_sqrtf(dd);
    if (dd > l->ltcal.nrr) {
        k = (k / dd) * l->ltcal.nrr;
    }
    add_diffuse(l, c, k);
}

/* 0xc4 */
void lt_rgb_c4(L *l, C *c, P *p, V *n)
{
    float k;

    (void)p;
    if (l->ltcal.ratten == 0.0F) {
        return;
    }
    k = dot3(n, &l->ltcal.lmvctr);
    if (!(k > 0.0F)) {
        return;
    }
    add_diffuse(l, c, k * l->ltcal.ratten);
}

/* o índice (Int)(k2 * 256) de fpow_tbl, ou -1 se passa de 255 */
static int pow_index(float k2)
{
    int i = (int)(k2 * 256.0F);

    return i >= 256 ? -1 : i;
}

/* 0x10, NJD_SPEC_DIR */
void lt_rgb_10(L *l, C *c, P *p, V *n)
{
    float k = -dot3(n, &l->ltcal.lvctr);
    float k2, s;

    (void)p;
    if (!(k > 0.0F)) {
        return;
    }
    add_diffuse(l, c, k);
    k2 = k * k;
    if (!(k2 > l->ltcal.cose)) {
        return;
    }
    s = 1.0F;
    if (k2 < l->ltcal.cosi) {
        s = lt_power_body(k2, l->ltcal.exp) * l->ltcal.rate;
    }
    add_specular(l, c, s);
}

/* 0x90: como o 0x10, com a tabela fpow_tbl */
void lt_rgb_90(L *l, C *c, P *p, V *n)
{
    float k = -dot3(n, &l->ltcal.lvctr);
    float k2, s;
    int i;

    (void)p;
    if (!(k > 0.0F)) {
        return;
    }
    add_diffuse(l, c, k);
    k2 = k * k;
    if (!(k2 > l->ltcal.cose)) {
        return;
    }
    s = 1.0F;
    if (k2 < l->ltcal.cosi) {
        i = pow_index(k2);
        if (i >= 0) {
            s = fpow_tbl[l->ltcal.exp][i] * l->ltcal.rate;
        }
    }
    add_specular(l, c, s);
}

/* 0x20, NJD_SPEC_POINT */
void lt_rgb_20(L *l, C *c, P *p, V *n)
{
    V d;
    float dd, nd, inv, k, a, k2, s;

    light_to(l, p, &d);
    nd = dot3(n, &d);
    dd = dot3(&d, &d);
    if (!(l->ltcal.frr > dd) || !(nd > 0.0F)) {
        return;
    }
    inv = 1.0F / __builtin_sqrtf(dd);
    a = 1.0F;
    if (dd > l->ltcal.nrr) {
        a = l->ltcal.nrr * inv * inv;
    }
    k = nd * inv;
    add_diffuse(l, c, k);
    k2 = k * k;
    if (!(k2 > l->ltcal.cose)) {
        return;
    }
    s = a;
    if (k2 < l->ltcal.cosi) {
        s = lt_power_body(k2, l->ltcal.exp) * l->ltcal.rate * a;
    }
    add_specular(l, c, s);
}

/* 0xa0: como o 0x20, com fpow_tbl */
void lt_rgb_a0(L *l, C *c, P *p, V *n)
{
    V d;
    float dd, nd, inv, k, a, k2, s;
    int i;

    light_to(l, p, &d);
    nd = dot3(n, &d);
    dd = dot3(&d, &d);
    if (!(l->ltcal.frr > dd) || !(nd > 0.0F)) {
        return;
    }
    inv = 1.0F / __builtin_sqrtf(dd);
    a = 1.0F;
    if (dd > l->ltcal.nrr) {
        a = l->ltcal.nrr * inv * inv;
    }
    k = nd * inv;
    add_diffuse(l, c, k);
    k2 = k * k;
    if (!(k2 > l->ltcal.cose)) {
        return;
    }
    s = a;
    if (k2 < l->ltcal.cosi) {
        i = pow_index(k2);
        if (i >= 0) {
            s = fpow_tbl[l->ltcal.exp][i] * l->ltcal.rate * a;
        }
    }
    add_specular(l, c, s);
}

/* 0xc8: k = ratten * (n . lmvctr)^2 */
void lt_rgb_c8(L *l, C *c, P *p, V *n)
{
    float d, k;

    (void)p;
    if (l->ltcal.ratten == 0.0F) {
        return;
    }
    d = dot3(n, &l->ltcal.lmvctr);
    if (!(d > 0.0F)) {
        return;
    }
    k = d * d * l->ltcal.ratten;
    add_diffuse(l, c, k);
    if (l->ltcal.ipd > l->ltcal.cosi) {
        add_specular(l, c, k);
    }
}

/* 0xe0: k = n . lmvctr */
void lt_rgb_e0(L *l, C *c, P *p, V *n)
{
    float k, k2, s;
    int i;

    (void)p;
    if (l->ltcal.ratten == 0.0F) {
        return;
    }
    k = dot3(n, &l->ltcal.lmvctr);
    if (!(k > 0.0F)) {
        return;
    }
    add_diffuse(l, c, k);
    k2 = k * k;
    if (!(k2 > l->ltcal.cose)) {
        return;
    }
    s = l->ltcal.ratten;
    if (k2 < l->ltcal.cosi) {
        i = pow_index(k2);
        if (i >= 0) {
            s = fpow_tbl[l->ltcal.exp][i] * l->ltcal.rate * l->ltcal.ratten;
        }
    }
    add_specular(l, c, s);
}

/* 0x40, NJD_USER_LIGHT: a função do usuário */
void lt_rgb_40(L *l, C *c, P *p, V *n)
{
    void (*fn)(NJS_ARGB *, NJS_POINT3 *, NJS_VECTOR *, NJS_LIGHT *) =
        (void (*)(NJS_ARGB *, NJS_POINT3 *, NJS_VECTOR *, NJS_LIGHT *))
            l->attr.func;

    if (fn != 0) {
        fn(c, p, n, l);
    }
}

/* 0x08, NJD_SPOT_LIGHT e 0x88. d = lpnt - point; nd = n . d; dd = d . d;
 * dl = d . lvctr; k = nd^2 / dd, vezes nrr / dd onde dd > nrr; c = dl^2 / dd.
 * nada sem nd > 0, dd < frr, dl < 0 e c > cose. onde c < cosi: difusa
 * k * rate * lt_power(c, exp) (0x88: k * rate * (c - cose)); onde c >= cosi:
 * difusa k e especular k */
static void spot(L *l, C *c, P *p, V *n, int smooth)
{
    V d;
    float nd, dd, dl, inv, k, cc;

    light_to(l, p, &d);
    nd = dot3(n, &d);
    dd = dot3(&d, &d);
    dl = dot3(&d, &l->ltcal.lvctr);
    inv = 1.0F / dd;
    k = nd * nd * inv;
    cc = dl * dl * inv;
    if (!(nd > 0.0F) || !(l->ltcal.frr > dd) || !(-dl > 0.0F) ||
        !(cc > l->ltcal.cose)) {
        return;
    }
    if (dd > l->ltcal.nrr) {
        k = k * l->ltcal.nrr * inv;
    }
    if (cc < l->ltcal.cosi) {
        float m = smooth ? (cc - l->ltcal.cose) : lt_power_body(cc, l->ltcal.exp);

        add_diffuse(l, c, k * l->ltcal.rate * m);
        return;
    }
    add_specular(l, c, k);
    add_diffuse(l, c, k);
}

void lt_rgb_08(L *l, C *c, P *p, V *n) { spot(l, c, p, n, 0); }
void lt_rgb_88(L *l, C *c, P *p, V *n) { spot(l, c, p, n, 1); }

/* 0x03: o 0x01 e depois difusa -(n . lvctr) onde n . lvctr não é positivo */
void lt_rgb_03(L *l, C *c, P *p, V *n)
{
    float d;

    lt_rgb_01(l, c, p, n);
    d = dot3(n, &l->ltcal.lvctr);
    if (!(d > 0.0F)) {
        add_diffuse(l, c, -d);
    }
}

void lt_rgb_83(L *l, C *c, P *p, V *n) { lt_rgb_01(l, c, p, n); lt_rgb_82(l, c, p, n); }
void lt_rgb_05(L *l, C *c, P *p, V *n) { lt_rgb_01(l, c, p, n); lt_rgb_04(l, c, p, n); }
void lt_rgb_c5(L *l, C *c, P *p, V *n) { lt_rgb_01(l, c, p, n); lt_rgb_c4(l, c, p, n); }
void lt_rgb_13(L *l, C *c, P *p, V *n) { lt_rgb_01(l, c, p, n); lt_rgb_10(l, c, p, n); }
void lt_rgb_93(L *l, C *c, P *p, V *n) { lt_rgb_01(l, c, p, n); lt_rgb_90(l, c, p, n); }
void lt_rgb_25(L *l, C *c, P *p, V *n) { lt_rgb_01(l, c, p, n); lt_rgb_20(l, c, p, n); }
void lt_rgb_a5(L *l, C *c, P *p, V *n) { lt_rgb_01(l, c, p, n); lt_rgb_a0(l, c, p, n); }
void lt_rgb_e5(L *l, C *c, P *p, V *n) { lt_rgb_01(l, c, p, n); lt_rgb_e0(l, c, p, n); }

/* o corpo de lt_rgb_lights: chama a calculadora de cada luz da fila, na ordem
 * da fila, com color, point e normal da conta corrente */
void lt_rgb_lights_body(void)
{
    Int i;

    for (i = 0; i < _nj_light_queue_.count; i++) {
        _nj_rgb_lt_func_[i](_nj_ptr_rgb_lt_[i], _nj_ptr_rgb_lt_argb_,
                            _nj_ptr_rgb_lt_pnt_, _nj_ptr_rgb_lt_nml_);
    }
}
