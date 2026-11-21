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

float sqrtf(float x)
{
    union { float f; unsigned int i; } u;
    float r;
    int i;

    if (x <= 0.0f)
        return 0.0f;

    u.f = x;
    u.i = (u.i >> 1) + 0x1fc00000u;
    r = u.f;
    for (i = 0; i < 4; i++)
        r = 0.5f * (r + x / r);
    return r;
}

float fabsf(float x)
{
    return x < 0.0f ? -x : x;
}
