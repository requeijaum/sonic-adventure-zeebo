/* zshinobi.c — o que o jogo pede do Shinobi (sy*) e dados soltos.
 *
 * Os stubs de bu, fm, kb, pd e sd são gerados por tools/gen_stubs.py a partir
 * dos headers (zstubs.c). Aqui ficam o heap, os dados e o que precisa de
 * comportamento: syMalloc/syFree vão para o heap do BREW, o cache é no-op. */

#include <AEEStdLib.h>

void *syMalloc(unsigned int size)
{
    return MALLOC(size);
}

void syFree(void *p)
{
    if (p != 0) {
        FREE(p);
    }
}

/* o monte do Shinobi é o do BREW */
void syMallocInit(void *heap, unsigned int size)
{
    (void)heap;
    (void)size;
}

void syMallocFinish(void)
{
}

/* cache do SH-4: ARM11 com cache coerente para o que o jogo faz */
void syCacheICI_P2_(void)
{
}

/* globais do Shinobi lidos por sbinit/interrupções */
int sygIntMode;
unsigned char sygIntStack[4096];
volatile int syIntNestCount;

/* driver de som: handle da DMA do G2 e área de trabalho do driver */
void *sdgG2DmaHandle;
void *sdgWrkPtr;

/* --- API do engine/dc que o jogo chama (HUD de depuração, 16:9, sprites) --- */

void zlog(const char *what, const char *name, int value);
#define STUB(name) int name() { zlog("stub", #name, (int)__builtin_return_address(0)); return 0; }

/* HUD de desempenho/depuração do engine/dc/overlay.c: desligado */
STUB(overlay_draw)
STUB(overlay_end)
STUB(overlay_mark)
STUB(overlay_note)
STUB(overlay_pad)
STUB(overlay_page)
/* o engine do slice nativo (sa_present_*): não há engine separado aqui */
STUB(sa_present_mode)
STUB(sa_present_window)
STUB(save_queue_cursor)
STUB(restore_queue_cursor)
STUB(ufAutoSynch)   /* TODO: filmes/áudio do jogo */
STUB(ufInitSystem)
STUB(ufPlayTime)
STUB(ufReadAllHead)
STUB(ufSynch)
STUB(pad_jump_buttons_signed) /* TODO: asm do pad do jogo */
STUB(SHZ_PREFETCH)

/* o jogo parou de forma que não volta (arquivo ausente etc.): trava aqui */
void zshim_trap(const char *what);
void port_stop(const char *what, const char *name, unsigned int number)
{
    (void)name;
    (void)number;
    zshim_trap(what);
    for (;;) {
    }
}

/* luz "easy" do engine/dc: um vetor estático (o jogo só lê/escreve) */
static float easy_light[3];
float *port_easy_light(void)
{
    return easy_light;
}

/* sprites 3D: engine/dc/sprite.c (sabor Dreamcast, game.mk DC_SET) */

/* set count bytes at mem to value's low byte (src/system/math.h) */
void fill_bytes(void *mem, int value, int count)
{
    unsigned char *p = (unsigned char *)mem;
    unsigned int n = (unsigned int)count;

    while (n--) {
        *p++ = (unsigned char)value;
    }
}

/* qsort: ordenação por inserção em blocos (n pequeno nos usos do jogo) */
void qsort(void *base, unsigned int n, unsigned int size,
           int (*cmp)(const void *, const void *))
{
    unsigned char *a = (unsigned char *)base;
    unsigned int i, j, k;

    for (i = 1; i < n; i++) {
        for (j = i; j > 0 && cmp(a + (j - 1) * size, a + j * size) > 0; j--) {
            for (k = 0; k < size; k++) {
                unsigned char t = a[(j - 1) * size + k];

                a[(j - 1) * size + k] = a[j * size + k];
                a[j * size + k] = t;
            }
        }
    }
}

/* dados que o jogo acessa por nome e que o slice nativo reserva em zeros
 * (engine/native/stubs.txt, tipo "zeros"; lá 64 KB, aqui o tamanho do tipo). No DC vêm do
 * asm do Ninja e do Kamui. */
#define ZEROS(name, size) unsigned char name[size] __attribute__((aligned(32)))
/* tamanhos pelos tipos dos headers (com folga); os sem tipo conhecido, 4 KB */
ZEROS(_nj_cnk_easy_multilight_buffer_, 16 * 4);
ZEROS(_nj_cnk_easy_multilight_buffer_dst_, 32 * 4);
ZEROS(_nj_view_, 64);           /* NJS_VIEW *: ponteiro */
ZEROS(_nj_view_matrix_, 64);    /* NJS_MATRIX */
ZEROS(_nj_camera_matrix_, 64);  /* NJS_MATRIX */
ZEROS(_nj_light_control_, 4096);
ZEROS(_nj_light_material_, 1024);
ZEROS(_nj_light_queue_, 4096);
ZEROS(kmiCurrentListType, 64);
ZEROS(kmiCurrentListState, 256);
ZEROS(kmiCurrentVertexBuff, 64);
ZEROS(kmiDstVertexBuffer, 64);
ZEROS(kmiBackGroundPlaneFuncs, 18 * 4);

/* --- cadeia de interrupções (sg_chain.h): só o VBLANK existe --------------
 * O jogo registra tratadores (ADX, vsync do frame) e espera que rodem a cada
 * vblank; aqui rodam em native_vsync, antes de o controle voltar ao timer. */
#define CHAIN_MAX 16

struct chain {
    void (*func)(void *);
    void *arg;
    int used;
};

static struct chain chain[CHAIN_MAX];

void *syChainAddHandler(int event, void (*func)(void *), int priority, void *arg)
{
    int i;

    (void)event;
    (void)priority;
    for (i = 0; i < CHAIN_MAX; i++) {
        if (!chain[i].used) {
            chain[i].func = func;
            chain[i].arg = arg;
            chain[i].used = 1;
            return &chain[i];
        }
    }
    return 0;
}

void syChainDeleteHandler(void *handle)
{
    struct chain *c = (struct chain *)handle;

    if (c != 0) {
        c->used = 0;
    }
}

void zchain_run(void)
{
    int i;

    for (i = 0; i < CHAIN_MAX; i++) {
        if (chain[i].used && chain[i].func != 0) {
            chain[i].func(chain[i].arg);
        }
    }
}

/* --- memória (VMU) -----------------------------------------------------
 * buInit chama a função de início antes de voltar, como a biblioteca faz
 * quando sobe (o menu espera por ela: modules/advertise/card.c). Sem cartão:
 * os demais bu* são stubs que devolvem 0 (nenhum drive pronto). TODO: save em
 * arquivo pelo IFileMgr (fase E3). */
int buInit(int capacity, int drives, void *work, void (*func)(void))
{
    (void)capacity;
    (void)drives;
    (void)work;
    if (func != 0) {
        func();
    }
    return 0;
}
