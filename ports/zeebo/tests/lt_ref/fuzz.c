/* fuzz: calculadora do asm traduzido x a minha, com a mesma luz aleatória */
#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lt.c"

char fpow_tbl[17*256*4] __attribute__((aligned(32)));
char _nj_ptr_rgb_lt_[256], _nj_light_queue_[256], _nj_ptr_rgb_lt_nml_[8], _nj_ptr_rgb_lt_pnt_[8], _nj_rgb_lt_func_[256];
char _nj_ptr_rgb_lt_argb_[8];

/* o lado meu, com as mesmas definições de tipo do jogo */
typedef float Float; typedef int Int; typedef unsigned Angle;
typedef struct { Float x, y, z; } NJS_POINT3, NJS_VECTOR;
typedef struct { Float a, r, g, b; } NJS_ARGB;
typedef Float NJS_MATRIX[16];
typedef struct {
    Float ratten, ipd; Float nrr, frr, cosi, cose, idev, odev; Float rate, intns; Int exp; Int reserve;
    NJS_POINT3 lpnt; NJS_VECTOR lvctr; NJS_VECTOR lmvctr; NJS_ARGB atten, amb, dif, spc; NJS_ARGB mamb, mdif, mspc;
} NJS_LIGHT_CAL;
typedef struct { Int lsrc; Float iamb, idif, ispc; Float nrang, frang; void *func; Angle iang, oang; NJS_ARGB argb; } NJS_LIGHT_ATTR;
typedef struct { NJS_MATRIX mtrx; NJS_POINT3 pnt; NJS_VECTOR vctr; Int stat; Int reserve; NJS_LIGHT_CAL ltcal; NJS_LIGHT_ATTR attr; } NJS_LIGHT;
#define _nj_ptr_rgb_lt_argb_ my_argb_ptr
static NJS_ARGB *my_argb_ptr;
static float my_fpow[17][256];
#include "mine_lt.inc"
#undef _nj_ptr_rgb_lt_argb_

static float rnd(float lo, float hi) { return lo + (hi - lo) * (rand() / (float)RAND_MAX); }
static void fill(NJS_LIGHT *l)
{
    float *f = (float *)l;
    for (unsigned i = 0; i < sizeof *l / 4; i++) f[i] = rnd(-2.0f, 2.0f);
    l->ltcal.exp = rand() % 17;
    l->ltcal.frr = rnd(0.5f, 6.0f); l->ltcal.nrr = rnd(0.1f, 2.0f);
    l->ltcal.cosi = rnd(0.3f, 0.9f); l->ltcal.cose = rnd(0.0f, 0.3f);
    l->ltcal.rate = rnd(0.0f, 2.0f); l->ltcal.ratten = (rand() % 5 == 0) ? 0.0f : rnd(0.0f, 2.0f);
}
int main(void)
{
    native_asm_init();
    srand(1234);
    { float *t = (float *)fpow_tbl; for (int i = 0; i < 17*256; i++) { t[i] = 1.0f - i / 300.0f; ((float*)my_fpow)[i] = t[i]; } }
    const struct { const char *n; void (*ref)(uintptr_t,uintptr_t,uintptr_t,uintptr_t); void (*mine)(NJS_LIGHT*,NJS_ARGB*,NJS_POINT3*,NJS_VECTOR*); } T[] = {
        {"01", lt_rgb_01, my_lt_rgb_01}, {"02", lt_rgb_02, my_lt_rgb_02}, {"82", lt_rgb_82, my_lt_rgb_82},
        {"04", lt_rgb_04, my_lt_rgb_04}, {"c4", lt_rgb_c4, my_lt_rgb_c4},
        {"10", lt_rgb_10, my_lt_rgb_10}, {"90", lt_rgb_90, my_lt_rgb_90}, {"20", lt_rgb_20, my_lt_rgb_20},
        {"a0", lt_rgb_a0, my_lt_rgb_a0}, {"c8", lt_rgb_c8, my_lt_rgb_c8}, {"e0", lt_rgb_e0, my_lt_rgb_e0}, {"08", lt_rgb_08, my_lt_rgb_08}, {"88", lt_rgb_88, my_lt_rgb_88}, {"03", lt_rgb_03, my_lt_rgb_03}, {"83", lt_rgb_83, my_lt_rgb_83}, {"05", lt_rgb_05, my_lt_rgb_05}, {"c5", lt_rgb_c5, my_lt_rgb_c5}, {"13", lt_rgb_13, my_lt_rgb_13}, {"93", lt_rgb_93, my_lt_rgb_93}, {"25", lt_rgb_25, my_lt_rgb_25}, {"a5", lt_rgb_a5, my_lt_rgb_a5}, {"e5", lt_rgb_e5, my_lt_rgb_e5}, 
    };
    void *blk = mmap(0, 1<<20, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, -1, 0);
    NJS_LIGHT *L = blk; NJS_ARGB *Cc = (NJS_ARGB*)((char*)blk + 4096); NJS_POINT3 *Pp = (NJS_POINT3*)((char*)blk + 8192); NJS_VECTOR *Nn = (NJS_VECTOR*)((char*)blk + 8256);
    NJS_ARGB *Cm = (NJS_ARGB*)((char*)blk + 12288);
    for (unsigned t = 0; t < sizeof T / sizeof *T; t++) {
        int bad = 0; double worst = 0;
        for (int it = 0; it < 20000; it++) {
            fill(L);
            Pp->x = rnd(-3,3); Pp->y = rnd(-3,3); Pp->z = rnd(-3,3);
            Nn->x = rnd(-1,1); Nn->y = rnd(-1,1); Nn->z = rnd(-1,1);
            for (int i = 0; i < 8; i++) { ((float*)Cc)[i] = rnd(0,1); ((float*)Cm)[i] = ((float*)Cc)[i]; }
            T[t].ref((uintptr_t)L, (uintptr_t)Cc, (uintptr_t)Pp, (uintptr_t)Nn);
            T[t].mine(L, Cm, Pp, Nn);
            for (int i = 0; i < 8; i++) {
                double d = __builtin_fabs(((float*)Cc)[i] - ((float*)Cm)[i]); double sc = __builtin_fabs(((float*)Cc)[i]) + 1;
                if (d / sc > worst) worst = d / sc;
                if (d / sc > 1e-4) { bad++; if (!strcmp(T[t].n,"90") && bad<=4) printf("  it=%d campo=%d ref=%g mine=%g exp=%d rate=%g cosi=%g cose=%g\n", it, i, ((float*)Cc)[i], ((float*)Cm)[i], L->ltcal.exp, L->ltcal.rate, L->ltcal.cosi, L->ltcal.cose); }
            }
        }
        printf("lt_rgb_%s: %s (pior erro relativo %.2e, %d campos acima de 1e-4)\n", T[t].n, bad ? "DIFERE" : "igual", worst, bad);
    }
    return 0;
}
