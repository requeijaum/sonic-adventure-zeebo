/* zlibc.c — o pouco de libc que o jogo e os shims do slice nativo pedem.
 *
 * No alvo não há libc (Developer Guide): malloc/free vão para o heap do BREW,
 * e stdio é descartado (o log vai por DBGPRINTF onde importar). */

#include <AEEStdLib.h>

#include "zstrip.h"

void *malloc(unsigned int n)
{
    return MALLOC(n);
}

void free(void *p)
{
    if (p != 0) {
        FREE(p);
    }
}

void *calloc(unsigned int n, unsigned int size)
{
    unsigned long total = (unsigned long)n * size;
    void *p;

    if (size != 0 && total / size != n) {
        return 0;
    }
    p = MALLOC((uint32)total);
    if (p != 0) {
        MEMSET(p, 0, (uint32)total);
    }
    return p;
}

void *realloc(void *p, unsigned int n)
{
    return REALLOC(p, n);
}

/* exit: o jogo pediu para terminar; sem processo, o applet decide parar */
void __attribute__((weak)) zlibc_exit_hook(int code)
{
    (void)code;
    for (;;) {
    }
}

void exit(int code)
{
    zlibc_exit_hook(code);
    for (;;) {
    }
}

char *getenv(const char *name)
{
    (void)name;
    return 0;
}

/* newlib: stderr é (_impure_ptr->_stderr). Um bloco zerado dá stderr nulo, e
 * fprintf descarta tudo. */
static unsigned char zreent[1024];
void *_impure_ptr = zreent;

int fprintf(void *stream, const char *format, ...)
{
    (void)stream;
    (void)format;
    return 0;
}
