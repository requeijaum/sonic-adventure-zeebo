/* zbench.c — custo de transformar vértices no ARM11 sem FPU (make ZBENCH=1).
 *
 * Compara a mesma conta (matriz 3x4 × ponto, como njCalcPoint) em float
 * soft (libgcc __aeabi_f*) e em ponto fixo s15.16 (GLfixed, o que o
 * Developer Guide §3.2.2 recomenda). Loga o tempo de cada uma por
 * GETUPTIMEMS; o zeebx também conta as instruções do run inteiro.
 * ZBENCH=1 roda as duas, 2 só float, 3 só ponto fixo, 4 nenhuma (base).
 * Roda a cada tique do timer (app.c): o custo é a diferença de instruções
 * por tique entre os modos, no relatório do zeebx. */
#include <AEEStdLib.h>

#define N 20000

typedef int fixed;

static volatile float fsink;
static volatile int xsink;

static void bench_float(void)
{
    static const float m[12] = {0.8F, 0.1F, -0.5F, 10.0F, 0.0F, 0.9F,
                                0.3F, -4.0F, 0.6F, -0.2F, 0.7F, 100.0F};
    float x = 1.5F, y = -2.25F, z = 3.0F;
    float acc = 0.0F;
    int i;

    for (i = 0; i < N; i++) {
        float ox = m[0] * x + m[1] * y + m[2] * z + m[3];
        float oy = m[4] * x + m[5] * y + m[6] * z + m[7];
        float oz = m[8] * x + m[9] * y + m[10] * z + m[11];

        acc += ox + oy + oz;
        x += 0.001F;
    }
    fsink = acc;
}

#define FX(v) ((fixed)((v) * 65536.0))
#define FMUL(a, b) ((fixed)(((long long)(a) * (b)) >> 16))

static void bench_fixed(void)
{
    static const fixed m[12] = {FX(0.8), FX(0.1), FX(-0.5), FX(10.0),
                                FX(0.0), FX(0.9), FX(0.3), FX(-4.0),
                                FX(0.6), FX(-0.2), FX(0.7), FX(100.0)};
    fixed x = FX(1.5), y = FX(-2.25), z = FX(3.0);
    fixed acc = 0;
    int i;

    for (i = 0; i < N; i++) {
        fixed ox = FMUL(m[0], x) + FMUL(m[1], y) + FMUL(m[2], z) + m[3];
        fixed oy = FMUL(m[4], x) + FMUL(m[5], y) + FMUL(m[6], z) + m[7];
        fixed oz = FMUL(m[8], x) + FMUL(m[9], y) + FMUL(m[10], z) + m[11];

        acc += ox + oy + oz;
        x += 66;
    }
    xsink = acc;
}

void zbench_run(void)
{
    uint32 t0 = GETUPTIMEMS();
    uint32 t1;
    uint32 t2;

#if ZBENCH == 1 || ZBENCH == 2
    bench_float();
#endif
    t1 = GETUPTIMEMS();
#if ZBENCH == 1 || ZBENCH == 3
    bench_fixed();
#endif
    t2 = GETUPTIMEMS();
    (void)t0; (void)t1; (void)t2;
    if (0) DBGPRINTF("sa1z: bench %d vertices: float soft %u ms, ponto fixo %u ms",
              N, (unsigned)(t1 - t0), (unsigned)(t2 - t1));
}
