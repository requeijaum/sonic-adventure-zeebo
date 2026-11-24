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
#define HOT_AT 3000
#define HOT_MAX 256

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

static char *put_dec(char *d, char *end, int v)
{
    char tmp[12];
    int n = 0;
    unsigned u = v < 0 ? 0u - (unsigned)v : (unsigned)v;

    if (v < 0 && d < end) {
        *d++ = '-';
    }
    do {
        tmp[n++] = (char)('0' + u % 10u);
        u /= 10u;
    } while (u != 0 && n < 11);
    while (n > 0 && d < end) {
        *d++ = tmp[--n];
    }
    return d;
}

/* contadores e leituras de estado: valor em decimal com sinal, sem cota (quem chama
 * limita: so quando o valor muda, no relatorio de cada janela de 2 s) */
void zlogd(const char *what, const char *name, int value)
{
    char buf[112];
    char *d = buf;
    char *end = buf + sizeof buf - 1;

    d = put_str(d, end, "sa1z: ");
    d = put_str(d, end, what);
    d = put_str(d, end, " ");
    d = put_str(d, end, name);
    d = put_str(d, end, " = ");
    d = put_dec(d, end, value);
    *d = 0;
    zhost_call(print_line, buf);
}

void zlog(const char *what, const char *name, int value)
{
    /* cota por categoria (antes: 150 linhas para todas juntas, e uma
     * categoria nova ficava muda quando as outras ja a tinham esgotado) */
    static struct {
        const char *what;
        int left;
    } cats[24];
    static int budget_jsr = 40;
    int i;
    int c;

    /* ponteiro que não é texto (nulo/pequeno): vira número, não texto */
    if ((unsigned long)name < 0x10000UL) {
        value = (int)(unsigned long)name;
        name = "(ptr)";
    }
    if (what[0] == 'm' || what[0] == 'v' || what[0] == 'g' || what[0] == 't' ||
        what[0] == 'E' || what[0] == 'A' || what[0] == 'T') { /* diagnósticos raros: sem cota */
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
    for (c = 0; c < 24; c++) {
        if (cats[c].what == what) {
            break;
        }
        if (cats[c].what == 0) {
            cats[c].what = what;
            cats[c].left = 60;
            break;
        }
    }
    if (c == 24) { /* mais categorias que a tabela: cota global do resto */
        static int rest = 60;

        if (rest > 0) {
            rest--;
            emit(what, name, value);
        }
        return;
    }
    if (cats[c].left > 0) {
        cats[c].left--;
        emit(what, name, value);
    }
}
