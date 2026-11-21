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

#define CANARY 0x5a1c0de5u

/* contexto novo: ao ser retomado, começa em entry com a pilha limpa */
static void *make(unsigned char *stack, unsigned long size, void (*entry)(void))
{
    unsigned long *sp = (unsigned long *)(stack + size);
    int i;

    *(unsigned long *)stack = CANARY;
    *--sp = (unsigned long)entry; /* pc */
    for (i = 0; i < 8; i++) {     /* r11..r4 */
        *--sp = 0;
    }
    return sp;
}

/* --- threads do jogo --- */

#define CONTEXTS 32
#define STACK_SIZE 0x8000UL /* 32 KB: os quadros ARM são maiores que os de SH-4 */

extern void *thread_current;
void thread_next(void);
void thread_exit(void);

/* Pontas do thread.c do jogo. Enquanto o jogo não está linkado, ficam fracas
 * (o selftest não passa por elas); com o jogo, as definições dele vencem. */
void *thread_current __attribute__((weak));
void thread_next(void) __attribute__((weak));
void thread_exit(void) __attribute__((weak));
void thread_next(void) {}
void thread_exit(void) {}

static struct context {
    void *thread;
    void (*entry)(void *);
    void *arg;
    void *sp; /* sp salvo */
    int ever; /* a pilha já foi preparada */
} contexts[CONTEXTS];
static unsigned char stacks[CONTEXTS + 1][STACK_SIZE]
    __attribute__((aligned(8)));
static void *restart_sp;

static unsigned char *stack_of(int slot)
{
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
    thread_exit();
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

    thread_next();
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

/* roda o jogo até o próximo native_vsync; 0 se ele terminou */
int zgame_step(void)
{
    if (game_done) {
        return 0;
    }
    zctx_switch(&host_sp, game_sp);
    return !game_done;
}

/* fim de frame do jogo: devolve o controle ao timer */
void native_vsync(void)
{
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
