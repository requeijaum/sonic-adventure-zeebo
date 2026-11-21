/* fmaf.c — o non-matching do decomp usa __builtin_fmaf (njRotate,
 * njOuterProduct); no ARMv6 sem FPU o GCC emite uma chamada a fmaf, que a
 * libm do host daria. Aqui a multiplicação-soma é separada: não é a fusão
 * bit-exata do SH-4, e sim a mesma expressão aritmética. */
float fmaf(float a, float b, float c)
{
    return a * b + c;
}
