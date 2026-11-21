/* math.h — shim freestanding do port.
 *
 * O SH4ZAM SW usa sinf/cosf (FSCA) e sqrtf/fabsf (FSRRA). No build do Zeebo
 * não há newlib no caminho (-nostdinc); as implementações vêm de
 * src/zeebo/platform/softfloat.c. Aqui ficam só as declarações de que o SH4ZAM e o
 * port precisam — nada de incluir o math.h do decomp, que troca nomes por
 * macros _builtin_*. */
#ifndef SA1Z_MATH_H
#define SA1Z_MATH_H

float sinf(float x);
float cosf(float x);
float tanf(float x);
float sqrtf(float x);
float fabsf(float x);
double sqrt(double x);
double cos(double x);

#endif /* SA1Z_MATH_H */
