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

/* sprites 3D: a profundidade é guardada; o desenho vem na Fase C */
static float sprite_depth;
void sprite_set_depth(float depth) { sprite_depth = depth; }
float sprite_get_depth(void) { return sprite_depth; }
void sprite_draw_3d(void *sprite, int entry, unsigned int attr)
{
    (void)sprite;
    (void)entry;
    (void)attr; /* TODO: sprite 3D no sink GLES1 */
}

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
