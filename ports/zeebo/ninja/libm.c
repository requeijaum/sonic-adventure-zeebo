/* libm.c — o que o decomp usa de math double (ADX_GetCoefficient usa
 * sqrt/cos). No ARM sem FPU, converte para as versões float do softfloat. */
float sqrtf(float x);
float cosf(float x);

double sqrt(double x)
{
    return (double)sqrtf((float)x);
}

double cos(double x)
{
    return (double)cosf((float)x);
}
