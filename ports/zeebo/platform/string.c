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

/* o resto do que o jogo e o SDK de runtime pedem da libc de string */
char *strchr(const char *s, int c)
{
    for (;; s++) {
        if (*s == (char)c) {
            return (char *)s;
        }
        if (*s == 0) {
            return 0;
        }
    }
}

unsigned int strspn(const char *s, const char *accept)
{
    unsigned int n = 0;

    while (s[n] != 0 && strchr(accept, s[n]) != 0) {
        n++;
    }
    return n;
}

unsigned int strcspn(const char *s, const char *reject)
{
    unsigned int n = 0;

    while (s[n] != 0 && strchr(reject, s[n]) == 0) {
        n++;
    }
    return n;
}

static int lower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

int strcasecmp(const char *a, const char *b)
{
    while (*a != 0 && lower((unsigned char)*a) == lower((unsigned char)*b)) {
        a++;
        b++;
    }
    return lower((unsigned char)*a) - lower((unsigned char)*b);
}

/* abort: sem processo para encerrar; trava onde o depurador vê */
void zlog(const char *what, const char *name, int value);

void abort(void)
{
    {
        /* percorre a cadeia de frames APCS (-mapcs-frame): [fp-4]=lr, [fp-12]=fp anterior */
        unsigned long *fp = (unsigned long *)__builtin_frame_address(0);
        int i;

        for (i = 0; i < 6 && fp != 0; i++) {
            zlog("ABORT", "lr", (int)fp[-1]);
            fp = (unsigned long *)fp[-3];
        }
    }
    for (;;) {
    }
}
