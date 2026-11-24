/* sa1hello — transcendentais em float sem libm.
 *
 * Implementacoes adaptadas de ../zeebo_demoscene/src/mat4.c (provas no
 * emulador e mesma precisao sub-pixel a 640x480): Taylor de 7a ordem com
 * reducao de quadrante para seno/cosseno, tangente como razao, raiz por
 * chute de meio-expoente + 4 iteracoes de Newton. Nomes padrao C99 para
 * satisfazer o w3dGraphics.cpp real do core.
 */

static float ajusta(float x)
{
    const float dois_pi = 6.28318530718f;
    const float meio_pi = 3.14159265359f;

    while (x >  meio_pi) x -= dois_pi;
    while (x < -meio_pi) x += dois_pi;
    return x;
}

static float meio_seno(float x)
{
    float x2 = x * x;
    return x * (1.0f + x2 * (-0.16666667f +
                    x2 * (0.008333333f + x2 * (-0.00019841270f))));
}

static float seno_base(float x)
{
    const float meio_pi = 1.57079632679f;
    const float pi      = 3.14159265359f;

    x = ajusta(x);
    if (x >  meio_pi) x =  pi - x;
    else if (x < -meio_pi) x = -pi - x;
    return meio_seno(x);
}

float sinf(float x)
{
    return seno_base(x);
}

float cosf(float x)
{
    return seno_base(x + 1.57079632679f);
}

float tanf(float x)
{
    float c = cosf(x);
    if (c == 0.0f)
        return 0.0f;
    return sinf(x) / c;
}

/* raiz por digitos sobre a mantissa, em inteiros (4 iteracoes de Newton com divisao em
 * soft-float custavam ~600 instrucoes; sqrtf era 7% do tempo do 3D). Arredondado
 * para o mais proximo, igual ao sqrtf correto */
float sqrtf(float x)
{
    union { float f; unsigned int i; } u;
    unsigned long long v, res = 0, bit;
    int e;
    unsigned int m;

    u.f = x;
    if ((u.i & 0x80000000u) != 0 || u.i == 0) {
        return 0.0f; /* negativo ou zero */
    }
    e = (int)((u.i >> 23) & 0xff);
    if (e == 0) {
        return 0.0f; /* subnormal */
    }
    if (e == 255) {
        return x; /* infinito ou NaN */
    }
    m = (u.i & 0x7fffffu) | 0x800000u;
    e -= 127;
    if (e & 1) {
        m <<= 1;
        e -= 1;
    }
    v = (unsigned long long)m << 23; /* sqrt(m * 2^23) esta em [2^23, 2^24) */
    for (bit = 1ULL << 46; bit != 0; bit >>= 2) {
        if (v >= res + bit) {
            v -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
    }
    if (v > res) { /* resto acima da raiz: arredonda para o mais proximo */
        res++;
    }
    u.i = ((unsigned int)(e / 2 + 127) << 23) + ((unsigned int)res - 0x800000u);
    return u.f;
}

/* sa_fsca (engine/numeric/fast.c) e da tabela do jogo; no applet sozinho (sem o jogo) este fraco
 * cobre o link: no .mod o forte de ninja/zxmtrx.c vence */
__attribute__((weak)) void sa_fsca(unsigned int angle, float out[2])
{
    float a = (float)(angle & 0xffffu) * (6.28318530718f / 65536.0f);

    out[0] = sinf(a);
    out[1] = cosf(a);
}

float fabsf(float x)
{
    return x < 0.0f ? -x : x;
}
