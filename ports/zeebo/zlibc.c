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

void zlog(const char *what, const char *name, int value);

void exit(int code)
{
    zlog("EXIT", "exit", (int)__builtin_return_address(0));
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
void zlog(const char *what, const char *name, int value);

int fprintf(void *stream, const char *format, ...)
{
    __builtin_va_list ap;
    int first;

    (void)stream;
    __builtin_va_start(ap, format);
    first = __builtin_va_arg(ap, int);
    __builtin_va_end(ap);
    zlog("fprintf", "arg1", first); /* o 1o argumento inteiro (ex.: alvo de JSR) */
    return 0;
}

/* log do app: aparece em "log do jogo" do zeebx; limitado para não inundar.
 * Também conta chamadas por (nome, chamador): quem passa de HOT_AT chamadas é
 * um laço de espera (hardware que não existe) e é logado como "HOT".
 *
 * O texto é montado à mão e entregue ao DBGPRINTF como "%s" único: o emitir com
 * vários %s/%x travou o zeebx em um argumento que não era texto. */
#define HOT_AT 20000
#define HOT_MAX 48

static struct {
    const char *name;
    int caller;
    unsigned int count;
} hot[HOT_MAX];

static char *put_str(char *d, char *end, const char *s)
{
    while (*s != 0 && d < end) {
        *d++ = *s++;
    }
    return d;
}

static char *put_hex(char *d, char *end, unsigned int v)
{
    static const char digits[] = "0123456789abcdef";
    int i;

    d = put_str(d, end, "0x");
    for (i = 28; i >= 0 && d < end; i -= 4) {
        *d++ = digits[(v >> i) & 15u];
    }
    return d;
}

void zhost_call(void (*fn)(void *), void *arg);

static void print_line(void *text)
{
    DBGPRINTF("%s", (const char *)text);
}

static void emit(const char *what, const char *name, int value)
{
    char buf[112];
    char *d = buf;
    char *end = buf + sizeof buf - 1;

    d = put_str(d, end, "sa1z: ");
    d = put_str(d, end, what);
    d = put_str(d, end, " ");
    d = put_str(d, end, name);
    d = put_str(d, end, " ");
    d = put_hex(d, end, (unsigned int)value);
    *d = 0;
    zhost_call(print_line, buf);
}

void zlog(const char *what, const char *name, int value)
{
    static int budget = 150;
    static int budget_jsr = 40;
    int i;

    /* ponteiro que não é texto (nulo/pequeno): vira número, não texto */
    if ((unsigned long)name < 0x10000UL) {
        value = (int)(unsigned long)name;
        name = "(ptr)";
    }
    if (what[0] == 'm' || what[0] == 'v') { /* diagnósticos já espaçados por quem chama */
        emit(what, name, value);
        return;
    }
    if (what[0] == 'j') { /* JSR do asm: cota própria */
        if (budget_jsr > 0) {
            budget_jsr--;
            emit(what, name, value);
        }
        return;
    }
    if (what[0] != 'g' && what[0] != 'n') { /* stubs e km*, não gdFs/njWait */
        for (i = 0; i < HOT_MAX; i++) {
            if (hot[i].name == name && hot[i].caller == value) {
                break;
            }
            if (hot[i].name == 0) {
                hot[i].name = name;
                hot[i].caller = value;
                break;
            }
        }
        if (i < HOT_MAX && ++hot[i].count == HOT_AT) {
            emit("HOT", name, value);
            return;
        }
    }
    if (budget > 0) {
        budget--;
        emit(what, name, value);
    }
}
