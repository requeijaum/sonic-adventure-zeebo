/* zshim.c — intrínsecos do SH-4 e pontas do slice nativo para o jogo em ARM.
 *
 * `_builtin_*` são intrínsecos do SHC (machine.h). Os que não têm efeito em
 * ARM ficam como no-op; os que o jogo usa para valor (strcmp, strcpy, fsca,
 * fsrra) são reais; os que dependem do banco de registradores do SH-4
 * (XMTRX) param o programa em vez de fingir. */

#include <stdint.h>

#include "play_math.h"

void zshim_trap(const char *what);

/* para o jogo em vez de seguir com valores falsos; o applet liga isto ao log */
void zlog(const char *what, const char *name, int value);

void __attribute__((weak)) zshim_trap(const char *what)
{
    zlog("TRAP", what, (int)__builtin_return_address(0));
    for (;;) {
    }
}

void _builtin_set_imask(int mask) { (void)mask; }
int _builtin_get_imask(void) { return 0; }
void *_builtin_get_vbr(void) { return 0; }
void _builtin_set_fpscr(int cr) { (void)cr; }
int _builtin_get_fpscr(void) { return 0; }
void _builtin_prefetch(void *p) { (void)p; }
void _builtin_trapa(int trap_no) { (void)trap_no; zshim_trap("trapa"); }

/* TAS: lê o byte, grava 0x80, devolve se o byte lido era 0 (T bit) */
int _builtin_tas(char *addr)
{
    char old = *addr;

    *addr = (char)(old | 0x80);
    return old == 0;
}

/* st_ext: XF0..XF15 em m, na ordem. Com um XMTRX só (ninja/zxmtrx.c), o
 * banco X do asm traduzido é o XMTRX de todos */
float *native_xmtrx(void);

void _builtin_st_ext(float (*m)[4])
{
    const float *x = native_xmtrx();
    float *d = &m[0][0];
    int i;

    for (i = 0; i < 16; i++) {
        d[i] = x[i];
    }
}

char *_builtin_strcpy(char *s1, const char *s2)
{
    char *d = s1;

    while ((*d++ = *s2++) != 0) {
    }
    return s1;
}

int _builtin_strcmp(const char *s1, const char *s2)
{
    while (*s1 != 0 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

/* FSCA/FSRRA como o play_math os dá */
void _builtin_fsca(int32_t angle, float *sine, float *cosine)
{
    float out[2];

    sa_fsca((uint32_t)angle, out);
    *sine = out[0];
    *cosine = out[1];
}

float _builtin_fsrra(float n)
{
    return sa_fsrra(n);
}

/* o _atan2 de mesh.src chama a versão em float do runtime do jogo */
extern float _in_atan2s(float y, float x);

float native_atan2(float y, float x)
{
    return _in_atan2s(y, x);
}


/* --- perfil (make PROF=1): ms por step/draw de task raiz, por endereço --- */
unsigned int zprof_ms(void); /* zlive.c */
void zlog(const char *what, const char *name, int value);
#define ZPROF_MAX 192
static struct { void *fn; unsigned int ms, calls; } zp[ZPROF_MAX];
static unsigned int zp_t0, zp_since, zp_depth;
static void *zp_fn;
static unsigned int zp_other; /* fora dos steps/draws: o resto do quadro */

static unsigned int zp_win;

static int zprof_dec(char *o, int p, unsigned int v)
{
    char t[12];
    int n = 0;

    do {
        t[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (n) {
        o[p++] = t[--n];
    }
    return p;
}

void zprof_begin(void *fn, int root)
{
    static int first;

    if (first < 3) {
        first++;
        zlog("vpin", "begin root", root);
        zlog("vpin", "self", (int)(unsigned long)&zprof_begin);
    }
    if (!root) {
        return;
    }
    zp_fn = fn;
    zp_t0 = zprof_ms();
    if (zp_since == 0) {
        zp_since = zp_t0;
    }
}

void zprof_end(int root)
{
    unsigned int now = zprof_ms(), i;
    static int firste;

    static int cnt;

    if ((++cnt & 4095) == 0) {
        zlog("vpin", "ms desde", (int)(now - zp_since));
        zlog("vpin", "agora", (int)now);
    }
    if (firste < 3) {
        firste++;
        zlog("vpin", "end root", root);
        zlog("vpin", "ms", (int)(now - zp_since));
    }
    if (!root) {
        return;
    }
    for (i = 0; i < ZPROF_MAX; i++) {
        if (zp[i].fn == zp_fn || zp[i].fn == 0) {
            break;
        }
    }
    if (i < ZPROF_MAX) {
        zp[i].fn = zp_fn;
        zp[i].ms += now - zp_t0;
        zp[i].calls++;
    }
    if (now - zp_since >= 8000) {
        unsigned int total = now - zp_since, sum = 0, k, n;

        for (n = 0; n < 14; n++) { /* os 14 maiores */
            unsigned int best = 0, bi = ZPROF_MAX;

            for (k = 0; k < ZPROF_MAX; k++) {
                if (zp[k].fn && zp[k].ms > best) {
                    best = zp[k].ms;
                    bi = k;
                }
            }
            if (bi == ZPROF_MAX) {
                break;
            }
            /* uma linha por função, única (janela, ordem): o log funde as iguais.
             * name = "w<janela> <ms> <chamadas>", value = endereço */
            char nm[40];
            int p = 0;

            nm[p++] = 'w';
            p = zprof_dec(nm, p, zp_win);
            nm[p++] = ' ';
            p = zprof_dec(nm, p, zp[bi].ms);
            nm[p++] = 'm';
            nm[p++] = ' ';
            p = zprof_dec(nm, p, zp[bi].calls);
            nm[p++] = 'c';
            nm[p] = 0;
            zlog("vpa", nm, (int)(unsigned long)zp[bi].fn);
            sum += zp[bi].ms;
            zp[bi].ms = 0;
        }
        zlog("vptt", "janela ms", (int)total);
        zlog("vpts", "soma dos 14 ms", (int)sum);
        for (k = 0; k < ZPROF_MAX; k++) {
            zp[k].ms = 0;
            zp[k].calls = 0;
        }
        zp_since = now;
        zp_win++;
    }
    (void)zp_depth; (void)zp_other;
}
