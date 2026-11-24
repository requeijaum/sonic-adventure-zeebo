/* sa1hello — memoria minima sem libc (o core usa memset; memcpy se o
 * linker pedir). Byte a byte, sem alinhamento esperto: suficiente para
 * sombras de matriz e caches pequenos do motor. */
#include "glshim/string.h"

/* palavras quando os dois ponteiros estao alinhados em 4 (matrizes, vetores e quase todo o
 * resto que o jogo copia): o byte a byte custava ~5 instrucoes por byte, ou 320 por matriz
 * de 64 bytes, e o memcpy era ~2,5% das instrucoes do 3D */
void *memset(void *s, int c, unsigned int n)
{
    unsigned char *p = (unsigned char *)s;
    unsigned int v = (unsigned char)c;

    if ((((unsigned long)p) & 3) == 0 && n >= 4) {
        unsigned int *w = (unsigned int *)p;

        v |= v << 8;
        v |= v << 16;
        for (; n >= 16; n -= 16) {
            w[0] = v;
            w[1] = v;
            w[2] = v;
            w[3] = v;
            w += 4;
        }
        for (; n >= 4; n -= 4) {
            *w++ = v;
        }
        p = (unsigned char *)w;
    }
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *d, const void *s, unsigned int n)
{
    unsigned char *to = (unsigned char *)d;
    const unsigned char *from = (const unsigned char *)s;

    if ((((unsigned long)to | (unsigned long)from) & 3) == 0 && n >= 4) {
        unsigned int *wt = (unsigned int *)to;
        const unsigned int *wf = (const unsigned int *)from;

        for (; n >= 16; n -= 16) {
            unsigned int a = wf[0], b = wf[1], c = wf[2], e = wf[3];

            wt[0] = a;
            wt[1] = b;
            wt[2] = c;
            wt[3] = e;
            wt += 4;
            wf += 4;
        }
        for (; n >= 4; n -= 4) {
            *wt++ = *wf++;
        }
        to = (unsigned char *)wt;
        from = (const unsigned char *)wf;
    }
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
