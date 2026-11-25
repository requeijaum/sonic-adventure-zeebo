/* zthreads.c — as threads do jogo e o laço do jogo como corrotinas ARM.
 *
 * Mesmo desenho de engine/native/threads.c (contexts[], make/swap/jump), mas
 * sem ucontext/mmap: a troca é um stub ARM que guarda r4-r11 e lr (soft-float,
 * sem VFP) e as pilhas são arrays em .bss.
 *
 * O jogo é um laço bloqueante (sa1_main); o BREW é dirigido por timer. Por
 * isso sa1_main roda numa corrotina própria: zgame_step() (do timer) entra
 * nela, e native_vsync() (fim de frame do jogo) volta ao timer. */

#include "zthreads.h"

void *malloc(unsigned int n);

/* guarda r4-r11 e lr na pilha atual, grava sp em *from e continua em to */
void zctx_switch(void **from, void *to);
__asm__(".text\n"
        ".arm\n"
        ".globl zctx_switch\n"
        "zctx_switch:\n"
        "    push {r4-r11, lr}\n"
        "    str sp, [r0]\n"
        "    mov sp, r1\n"
        "    pop {r4-r11, pc}\n");

/* chama fn(arg) com sp = new_sp (alinhado a 8), depois volta à pilha atual */
void zctx_call_on(void (*fn)(void *), void *arg, void *new_sp);
__asm__(".text\n"
        ".arm\n"
        ".globl zctx_call_on\n"
        "zctx_call_on:\n"
        "    push {r4, lr}\n"
        "    mov r4, sp\n"
        "    mov sp, r2\n"
        "    mov r2, r0\n"
        "    mov r0, r1\n"
        "    mov lr, pc\n"
        "    bx r2\n"
        "    mov sp, r4\n"
        "    pop {r4, pc}\n");

#define CANARY 0x5a1c0de5u

/* contexto novo: ao ser retomado, começa em entry com a pilha limpa */
#define FILL 0xa5a5a5a5u

static void *make(unsigned char *stack, unsigned long size, void (*entry)(void))
{
    unsigned long *sp = (unsigned long *)(stack + size);
    unsigned long *w;
    int i;

    /* marca d'água: a pilha inteira com FILL; o que mudou foi usado */
    for (w = (unsigned long *)stack; w < sp; w++) {
        *w = FILL;
    }
    *(unsigned long *)stack = CANARY;
    *--sp = (unsigned long)entry; /* pc */
    for (i = 0; i < 8; i++) {     /* r11..r4 */
        *--sp = 0;
    }
    return sp;
}

/* bytes já usados de uma pilha feita por make(): do topo até a palavra mais
 * funda que deixou de ser FILL */
static unsigned long used(const unsigned char *stack, unsigned long size)
{
    const unsigned long *w = (const unsigned long *)stack + 1; /* pula o CANARY */
    const unsigned long *end = (const unsigned long *)(stack + size);

    while (w < end && *w == FILL) {
        w++;
    }
    return (unsigned long)((const unsigned char *)end - (const unsigned char *)w);
}

/* --- threads do jogo --- */

#define CONTEXTS 32
#define STACK_SIZE 0x8000UL /* 32 KB: os quadros ARM são maiores que os de SH-4 */

extern void *thread_current;
void thread_next(void);
void thread_exit(void);

/* Pontas do thread.c do jogo. thread_current continua uma definição fraca (os dois
 * lados usam o mesmo símbolo). thread_next e thread_exit são só REFERÊNCIAS fracas:
 * no .mod o jogo inteiro fica fraco (--weaken) e o primeiro fraco vence, então um corpo
 * vazio aqui (o applet entra primeiro no link) tapava o thread_next do jogo, e nenhuma
 * thread (diálogos de NPC, mensagens) jamais recebia a vez. Sem o jogo (selftest) as
 * referências valem 0 e as chamadas são puladas. */
void *thread_current __attribute__((weak));
void thread_next(void) __attribute__((weak));
void thread_exit(void) __attribute__((weak));

static struct context {
    void *thread;
    void (*entry)(void *);
    void *arg;
    void *sp; /* sp salvo */
    int ever; /* a pilha já foi preparada */
} contexts[CONTEXTS];
/* pilhas no heap (primeiro uso): ~1 MB de .bss iam zerados dentro do .mod */
static unsigned char *stacks[CONTEXTS + 1];
static void *restart_sp;

static unsigned char *stack_of(int slot)
{
    if (stacks[slot] == 0) {
        void *p = malloc(STACK_SIZE + 8);

        stacks[slot] = (unsigned char *)(((unsigned long)p + 7u) & ~7ul);
    }
    return stacks[slot];
}

static int stands_on(int slot)
{
    char here;

    return (unsigned long)&here - (unsigned long)stack_of(slot) < STACK_SIZE;
}

static void fatal(void)
{
    for (;;) {
    }
}

static struct context *context_of(void *thread)
{
    struct context *free_slot = 0;
    int i;

    for (i = 0; i < CONTEXTS; i++) {
        if (contexts[i].thread == thread) {
            return &contexts[i];
        }
        if (!contexts[i].thread && !free_slot && !stands_on(i)) {
            free_slot = &contexts[i];
        }
    }
    if (!free_slot) {
        fatal(); /* mais de CONTEXTS threads */
    }
    free_slot->thread = thread;
    return free_slot;
}

static void start(void)
{
    struct context *self = context_of(thread_current);

    self->entry(self->arg);
    if (thread_exit) {
        thread_exit();
    }
}

static void renew(struct context *self)
{
    int slot = (int)(self - contexts);

    if (stands_on(slot)) {
        fatal();
    }
    self->sp = make(stack_of(slot), STACK_SIZE, start);
    self->ever = 1;
}

void prepare(void *thread, void *entry, void *arg)
{
    struct context *self = context_of(thread);

    self->entry = (void (*)(void *))entry;
    self->arg = arg;
    renew(self);
}

static void restarted(void)
{
    struct context *self = context_of(thread_current);
    void *dummy;

    renew(self);
    zctx_switch(&dummy, self->sp);
}

void run(void *entry, void *arg)
{
    struct context *self = context_of(thread_current);
    void *dummy;

    self->entry = (void (*)(void *))entry;
    self->arg = arg;
    restart_sp = make(stack_of(CONTEXTS), STACK_SIZE, restarted);
    zctx_switch(&dummy, restart_sp);
}

void thread_yield(void)
{
    struct context *self = context_of(thread_current);
    struct context *next;

    if (thread_next) {
        thread_next();
    }
    next = context_of(thread_current);
    if (next != self) {
        zctx_switch(&self->sp, next->sp);
    }
}

void resume(void)
{
    void *dummy;

    zctx_switch(&dummy, context_of(thread_current)->sp);
}

void native_thread_freed(void *memory)
{
    int i;

    for (i = 0; i < CONTEXTS; i++) {
        if (contexts[i].thread == memory) {
            contexts[i].thread = 0;
        }
    }
}

/* 0 se algum canário de pilha foi pisado */
int zthreads_check(void)
{
    int i;

    for (i = 0; i <= CONTEXTS; i++) {
        if (*(unsigned long *)stack_of(i) != CANARY && contexts[i % CONTEXTS].ever) {
            return 0;
        }
    }
    return 1;
}

/* --- o laço do jogo como corrotina do timer do BREW --- */

#define GAME_STACK 0x20000UL
static unsigned char game_stack[GAME_STACK] __attribute__((aligned(8)));
static void *host_sp;
static void *game_sp;
static void (*game_entry)(void);
static int game_done;

static void game_main(void)
{
    game_entry();
    game_done = 1;
    for (;;) {
        zctx_switch(&game_sp, host_sp);
    }
}

void zgame_start(void (*entry)(void))
{
    game_entry = entry;
    game_done = 0;
    game_sp = make(game_stack, GAME_STACK, game_main);
}

static int game_running;
static int reset_wanted;

/* chamado de dentro do jogo: abandona a pilha dele e, no próximo quadro,
 * começa o main do zero (como o reset do console). Usado quando o jogo pede
 * um programa que o .mod não tem (zgdfs.c): em vez de cair, volta ao logo. */
void zgame_reset(void)
{
    void zchain_reset(void) __attribute__((weak));
    void zstrip_tex_release_all(void);

    reset_wanted = 1;
    for (;;) {
        zctx_switch(&game_sp, host_sp);
    }
}

/* roda fn(arg) na pilha do host se o jogo está rodando (o zeebx só aceita as
 * chamadas de helper do BREW com sp na pilha do app); senão, direto */
void zhost_call(void (*fn)(void *), void *arg)
{
    if (game_running && host_sp != 0) {
        /* abaixo do sp salvo: ali o frame do timer já não usa mais nada */
        unsigned long top = ((unsigned long)host_sp - 512u) & ~7ul;

        zctx_call_on(fn, arg, (void *)top);
    } else {
        fn(arg);
    }
}

/* roda o jogo até o próximo native_vsync; 0 se ele terminou */
extern int game_mode __attribute__((weak));
extern int frame_step_count __attribute__((weak));
extern int frame_interval __attribute__((weak));
void zlog(const char *what, const char *name, int value);

void zinst_mode(int mode, unsigned frames, int movie_request_now);

int zgame_step(void)
{
    static int frames, last_mode = -1;
    static unsigned long most;

    if (&game_mode != 0 && game_mode != last_mode) {
        extern int movie_request __attribute__((weak));

        zinst_mode(game_mode, (unsigned)frames, &movie_request != 0 ? movie_request : -1);
        last_mode = game_mode;
        zlog("modo", "game_mode", game_mode);
    }
    {
        /* quantos passos de logica por quadro e quantos vsyncs por quadro (o jogo
         * roda frame_step_count passos e espera frame_interval vsyncs) */
        static int last_steps = -1, last_interval = -1;

        if (&frame_step_count != 0 && (frame_step_count != last_steps ||
                                       frame_interval != last_interval)) {
            last_steps = frame_step_count;
            last_interval = frame_interval;
            zlog("modo", "passos por quadro", frame_step_count);
            zlog("modo", "vsyncs por quadro", frame_interval);
        }
    }
    frames++;
    /* B4: pilha da corrotina do jogo; loga quando passa do máximo anterior
     * em mais de 1 KB, a cada 64 quadros */
    if ((frames & 63) == 0) {
        unsigned long now = used(game_stack, GAME_STACK);

        if (now > most + 1024) {
            most = now;
            zlog("main", "pilha do jogo (bytes)", (int)now);
        }
    }
    if (reset_wanted) {
        extern void zchain_reset(void) __attribute__((weak));
        void zstrip_tex_release_all(void);

        reset_wanted = 0;
        zlog("main", "reset do jogo (programa ausente)", frames);
        if (zchain_reset != 0) {
            zchain_reset(); /* o boot registra os tratadores de novo */
        }
        zstrip_tex_release_all();
        game_sp = make(game_stack, GAME_STACK, game_main);
    }
    if (game_done) {
        return 0;
    }
    game_running = 1;
    zctx_switch(&host_sp, game_sp);
    game_running = 0;
    return !game_done;
}

/* fim de frame do jogo: devolve o controle ao timer */
/* vblank: roda os tratadores da cadeia e o vsync do Ninja, devolve ao timer */
extern void zchain_run(void) __attribute__((weak));
extern void zvsync_run(void) __attribute__((weak));

void zlog(const char *what, const char *name, int value);

void native_vsync(void)
{
    if (zchain_run != 0) {
        zchain_run();
    }
    if (zvsync_run != 0) {
        zvsync_run();
    }
    zctx_switch(&game_sp, host_sp);
}

/* --- teste: ping-pong entre duas pilhas --- */

static int pong_count;
static void *pong_sp, *ping_sp;

static void pong_entry(void)
{
    for (;;) {
        pong_count++;
        zctx_switch(&pong_sp, ping_sp);
    }
}

int zthreads_selftest(void)
{
    static unsigned char pstack[0x2000] __attribute__((aligned(8)));
    int r;

    pong_count = 0;
    pong_sp = make(pstack, sizeof pstack, pong_entry);
    for (r = 0; r < 5; r++) {
        zctx_switch(&ping_sp, pong_sp);
    }
    return pong_count == 5;
}
