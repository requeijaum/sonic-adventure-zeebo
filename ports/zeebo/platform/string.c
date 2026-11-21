/* sa1hello — memoria minima sem libc (o core usa memset; memcpy se o
 * linker pedir). Byte a byte, sem alinhamento esperto: suficiente para
 * sombras de matriz e caches pequenos do motor. */
#include "glshim/string.h"

void *memset(void *s, int c, unsigned int n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *d, const void *s, unsigned int n)
{
    unsigned char *to = (unsigned char *)d;
    const unsigned char *from = (const unsigned char *)s;

    while (n--)
        *to++ = *from++;
    return d;
}
