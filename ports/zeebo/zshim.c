/* zshim.c — intrínsecos do SH-4 e pontas do slice nativo para o jogo em ARM.
 *
 * `_builtin_*` são intrínsecos do SHC (machine.h). Os que não têm efeito em
 * ARM ficam como no-op; os que o jogo usa para valor (strcmp, strcpy, fsca,
 * fsrra) são reais; os que dependem do banco de registradores do SH-4
 * (XMTRX) param o programa em vez de fingir. */

#include <stdint.h>

#include "play_math.h"

void zshim_trap(const char *what);

/* para o jogo em vez de seguir com valores falsos; o applet liga isto ao log */
void zlog(const char *what, const char *name, int value);

void __attribute__((weak)) zshim_trap(const char *what)
{
    zlog("TRAP", what, (int)__builtin_return_address(0));
    for (;;) {
    }
}

void _builtin_set_imask(int mask) { (void)mask; }
int _builtin_get_imask(void) { return 0; }
void *_builtin_get_vbr(void) { return 0; }
void _builtin_set_fpscr(int cr) { (void)cr; }
int _builtin_get_fpscr(void) { return 0; }
void _builtin_prefetch(void *p) { (void)p; }
void _builtin_trapa(int trap_no) { (void)trap_no; zshim_trap("trapa"); }

/* TAS: lê o byte, grava 0x80, devolve se o byte lido era 0 (T bit) */
int _builtin_tas(char *addr)
{
    char old = *addr;

    *addr = (char)(old | 0x80);
    return old == 0;
}

/* STC/STS do banco XMTRX: não existe em ARM; njModelS é quem usa */
void _builtin_st_ext(float (*m)[4])
{
    (void)m;
    zshim_trap("_builtin_st_ext (XMTRX)");
}

char *_builtin_strcpy(char *s1, const char *s2)
{
    char *d = s1;

    while ((*d++ = *s2++) != 0) {
    }
    return s1;
}

int _builtin_strcmp(const char *s1, const char *s2)
{
    while (*s1 != 0 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

/* FSCA/FSRRA como o play_math os dá */
void _builtin_fsca(int32_t angle, float *sine, float *cosine)
{
    float out[2];

    sa_fsca((uint32_t)angle, out);
    *sine = out[0];
    *cosine = out[1];
}

float _builtin_fsrra(float n)
{
    return sa_fsrra(n);
}

/* o _atan2 de mesh.src chama a versão em float do runtime do jogo */
extern float _in_atan2s(float y, float x);

float native_atan2(float y, float x)
{
    return _in_atan2s(y, x);
}
