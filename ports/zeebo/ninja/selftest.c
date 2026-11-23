/* selftest.c — teste host (x86) do port ARM do non-matching do Ninja.
 *
 * Compila os 13 engine/dc/ninja/*.c reais do decomp contra o nosso
 * ninja_zam.h (SH4ZAM backend SW) e checa invariantes que não dependem da
 * convenção de sinal do SH-4:
 *   - identidade, translação, push/pop
 *   - caminho XMTRX (m == NULL) e caminho memória (m != NULL) concordam
 *   - round-trip njSetMatrix/njGetMatrix
 *   - njMultiMatrix(NULL, A) == carregar A
 *
 * Não é prova de fidelidade bit-a-bit do SH-4; é prova de que o port liga e
 * que as duas rotas do decomp usam a mesma matemática. */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <ninja.h>
#include "../zblend.h"

static NJS_MATRIX g_stack[16];

NJS_MATRIX *_nj_current_matrix_ptr_ = g_stack;
Sint32 _max_stack_size_ = 16;
Sint32 _stack_size_;
NJS_MATRIX _unit_matrix_ = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
Int _nj_matrix_flag_;

static int failures;

#define CLOSE(a, b) (fabsf((a) - (b)) < 0.001f)

static void check_point(const char *what, NJS_POINT3 got, float x, float y,
                        float z)
{
    if (!CLOSE(got.x, x) || !CLOSE(got.y, y) || !CLOSE(got.z, z)) {
        printf("FAIL %-28s got (%.3f, %.3f, %.3f) want (%.3f, %.3f, %.3f)\n",
               what, got.x, got.y, got.z, x, y, z);
        failures++;
        return;
    }
    printf("ok   %s\n", what);
}

static void check_flag(const char *what, int condition)
{
    if (!condition) {
        printf("FAIL %s\n", what);
        failures++;
        return;
    }
    printf("ok   %s\n", what);
}

int main(void)
{
    NJS_POINT3 p = {1.0f, 2.0f, 3.0f};
    NJS_POINT3 got;
    NJS_MATRIX m;

    /* o Ninja conta o nível-base: um push leva de 1 para 2 */
    _stack_size_ = 1;
    _nj_current_matrix_ptr_ = g_stack;

    /* mistura do PVR sem alfa de destino (RGB565 do Zeebo) */
    check_flag("blend 6 -> ONE", zblend_code(6) == 1);
    check_flag("blend 7 -> ZERO", zblend_code(7) == 0);
    check_flag("blend 4 fica", zblend_code(4) == 4 && zblend_code(5) == 5);
    check_flag("blend usa 3 bits", zblend_code(14) == 1 && zblend_code(12) == 4);

    /* identidade */
    njUnitMatrix(NULL);
    njCalcPoint(NULL, &p, &got);
    check_point("identity", got, 1.0f, 2.0f, 3.0f);

    /* translação */
    njUnitMatrix(NULL);
    njTranslate(NULL, 10.0f, 20.0f, 30.0f);
    njCalcPoint(NULL, &p, &got);
    check_point("translate", got, 11.0f, 22.0f, 33.0f);

    /* push/pop restaura a matriz */
    njUnitMatrix(NULL);
    check_flag("push", njPushMatrix(NULL) != FALSE);
    njTranslate(NULL, 5.0f, 0.0f, 0.0f);
    njCalcPoint(NULL, &p, &got);
    check_point("inside push", got, 6.0f, 2.0f, 3.0f);
    check_flag("pop", njPopMatrix(1) != FALSE);
    njCalcPoint(NULL, &p, &got);
    check_point("after pop", got, 1.0f, 2.0f, 3.0f);

    /* o caminho memória e o caminho XMTRX concordam */
    njUnitMatrix(NULL);
    njRotateZ(NULL, 1 << 14); /* quarto de volta */
    njGetMatrix(&m);
    njCalcPoint(NULL, &p, &got);
    {
        NJS_POINT3 via_memory;

        njCalcPoint(&m, &p, &via_memory);
        check_point("rotZ: XMTRX == memoria", via_memory, got.x, got.y, got.z);
    }
    {
        NJS_POINT3 ex = {1.0f, 0.0f, 0.0f};
        NJS_POINT3 turned;

        njCalcPoint(NULL, &ex, &turned);
        check_flag("rotZ: x vai para +/-y",
                   CLOSE(turned.x, 0.0f) &&
                       (CLOSE(turned.y, 1.0f) || CLOSE(turned.y, -1.0f)));
    }

    /* njSetMatrix/njGetMatrix fazem round-trip */
    njUnitMatrix(NULL);
    njTranslate(NULL, 7.0f, 8.0f, 9.0f);
    njGetMatrix(&m);
    njUnitMatrix(NULL);
    njSetMatrix(NULL, &m);
    njCalcPoint(NULL, &p, &got);
    check_point("set/get round-trip", got, 8.0f, 10.0f, 12.0f);

    /* njMultiMatrix(NULL, A) carrega A */
    {
        NJS_MATRIX a;
        NJS_POINT3 q = {0.0f, 0.0f, 0.0f};
        NJS_POINT3 via_multi;
        NJS_POINT3 via_load;

        njUnitMatrix(NULL);
        njRotateX(NULL, 1 << 12);
        njGetMatrix(&a);
        njUnitMatrix(NULL);
        njMultiMatrix(NULL, &a);
        njCalcPoint(NULL, &q, &via_multi);
        njUnitMatrix(NULL);
        njSetMatrix(NULL, &a);
        njCalcPoint(NULL, &q, &via_load);
        check_point("multi(NULL) == set", via_multi, via_load.x, via_load.y,
                    via_load.z);
    }

    printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "PASSED", failures,
           failures == 1 ? "" : "s");
    return failures != 0;
}
