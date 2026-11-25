/* zshinobi.c — o que o jogo pede do Shinobi (sy*) e dados soltos.
 *
 * Os stubs de bu, fm, kb, pd e sd são gerados por tools/gen_stubs.py a partir
 * dos headers (zstubs.c). Aqui ficam o heap, os dados e o que precisa de
 * comportamento: syMalloc/syFree vão para o heap do BREW, o cache é no-op. */

#include <AEEStdLib.h>
#include <string.h>
int strncmp(const char *a, const char *b, unsigned int n);
char *strncpy(char *d, const char *s, unsigned int n);

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

/* reset do jogo (zthreads.c): esquece os tratadores */
void zchain_reset(void)
{
    int i;

    for (i = 0; i < CHAIN_MAX; i++) {
        chain[i].used = 0;
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
 * Um cartão de 200 blocos de 512 bytes no drive A1, guardado inteiro num
 * arquivo do app (VMU_FILE) pelo IFileMgr: lido no buInit, regravado a cada
 * save ou exclusão. As operações terminam na hora (buStat fica pronto); as
 * que o jogo espera pelo retorno (montagem, load, save) avisam a função de
 * término no próximo buExecServer, como a biblioteca. A imagem do arquivo
 * (cabeçalho, ícones, CRC) vem do vmsutil.c do decomp. */
#define VMU_FILE "sa1vmu.bin"
#define VMU_BLOCK 512
#define VMU_BLOCKS 200
#define VMU_FILES 24
#define VMU_DRIVE 0

int zfile_size(const char *path);
int zfile_read(const char *path, void *dst, int cap);
int zfile_write(const char *path, const void *src, int len);
void zlog(const char *what, const char *name, int value);

struct vmu_entry {
    char name[16];
    unsigned short first; /* bloco */
    unsigned short blocks;
    unsigned char time[8];
};
static struct {
    unsigned int magic;
    struct vmu_entry files[VMU_FILES];
    unsigned char data[VMU_BLOCKS * VMU_BLOCK];
} vmu;
#define VMU_MAGIC 0x31554d56u /* "VMU1" */
static int vmu_find;
static int (*vmu_complete)(int drive, int op, int status, unsigned int param);
static int (*vmu_progress)(int drive, int op, int count, int max);
static int vmu_pending_op, vmu_pending_status;

static void vmu_post(int op, int status)
{
    vmu_pending_op = op;
    vmu_pending_status = status;
}

static int vmu_used(void)
{
    int i;
    int used = 0;

    for (i = 0; i < VMU_FILES; i++) {
        if (vmu.files[i].name[0] != 0) {
            used += vmu.files[i].blocks;
        }
    }
    return used;
}

static struct vmu_entry *vmu_lookup(const char *name)
{
    int i;

    for (i = 0; i < VMU_FILES; i++) {
        if (vmu.files[i].name[0] != 0 && strncmp(vmu.files[i].name, name, 12) == 0) {
            return &vmu.files[i];
        }
    }
    return 0;
}

/* compacta a tabela e os dados: os arquivos ficam no começo da tabela e
 * contíguos nos blocos, na mesma ordem */
static void vmu_pack(void)
{
    int i;
    int n = 0;
    int at = 0;

    for (i = 0; i < VMU_FILES; i++) {
        if (vmu.files[i].name[0] != 0) {
            if (i != n) {
                vmu.files[n] = vmu.files[i];
                memset(&vmu.files[i], 0, sizeof vmu.files[i]);
            }
            n++;
        }
    }
    for (i = 0; i < n; i++) {
        struct vmu_entry *e = &vmu.files[i];

        if (e->first != at) {
            memmove(vmu.data + at * VMU_BLOCK, vmu.data + e->first * VMU_BLOCK,
                    (unsigned)e->blocks * VMU_BLOCK);
            e->first = (unsigned short)at;
        }
        at += e->blocks;
    }
}

static void vmu_store(void)
{
    vmu_pack();
    vmu.magic = VMU_MAGIC;
    if (zfile_write(VMU_FILE, &vmu,
                    (int)(sizeof vmu - sizeof vmu.data) + vmu_used() * VMU_BLOCK) <= 0) {
        zlog("bu", "falha ao gravar o cartao", 0);
    }
}

int buInit(int capacity, int drives, void *work, void (*func)(void))
{
    (void)capacity;
    (void)drives;
    (void)work;
    memset(&vmu, 0, sizeof vmu);
    if (zfile_size(VMU_FILE) > 0) {
        zfile_read(VMU_FILE, &vmu, (int)sizeof vmu);
        if (vmu.magic != VMU_MAGIC) {
            memset(&vmu, 0, sizeof vmu);
        }
    }
    zlog("bu", "cartao: blocos usados", vmu_used());
    if (func != 0) {
        func();
    }
    vmu_post(1 /* BUD_OP_MOUNT */, 0);
    return 0;
}

void buSetCompleteCallback(int (*func)(int, int, int, unsigned int))
{
    vmu_complete = func;
}

void buSetProgressCallback(int (*func)(int, int, int, int))
{
    vmu_progress = func;
}

void buDisableServer(void)
{
}

void buExecServer(void)
{
    if (vmu_pending_op != 0) {
        int op = vmu_pending_op;

        vmu_pending_op = 0;
        if (vmu_complete != 0) {
            vmu_complete(VMU_DRIVE, op, vmu_pending_status, 0);
        }
    }
}

int buStat(int drive)
{
    (void)drive;
    return 0; /* BUD_STAT_READY */
}

int buIsReady(int drive)
{
    /* como vmsfs_ismount: diferente de zero = montado. Antes devolvia 0 para o
     * A1 e 0xffffff01 (verdadeiro) para os outros: o jogo via o A1 como vazio e
     * os outros 7 slots como cartoes prontos, e a selecao caia fora do A1. */
    return drive == VMU_DRIVE ? 1 : 0;
}

struct vmu_diskinfo {
    unsigned char volume[32];
    unsigned short total_blocks, total_user_blocks, free_blocks;
    unsigned short free_user_blocks, total_exe_blocks, free_exe_blocks;
    unsigned short block_size, icon_no;
    unsigned char time[8];
};

int buGetDiskInfo(int drive, struct vmu_diskinfo *info)
{
    int free_blocks = VMU_BLOCKS - vmu_used();

    if (drive != VMU_DRIVE) {
        return (int)0xffffff01;
    }
    memset(info, 0, sizeof *info);
    info->total_blocks = 256;
    info->total_user_blocks = VMU_BLOCKS;
    info->free_blocks = (unsigned short)free_blocks;
    info->free_user_blocks = (unsigned short)free_blocks;
    info->total_exe_blocks = VMU_BLOCKS;
    info->free_exe_blocks = (unsigned short)free_blocks;
    info->block_size = VMU_BLOCK;
    return 0;
}

static int vmu_next(char *fname)
{
    while (vmu_find < VMU_FILES) {
        struct vmu_entry *e = &vmu.files[vmu_find++];

        if (e->name[0] != 0) {
            memcpy(fname, e->name, 12);
            fname[12] = 0;
            return 0;
        }
    }
    return (int)0xffffff05; /* BUD_ERR_FILE_NOT_FOUND */
}

int buFindFirstFile(int drive, char *fname)
{
    if (drive != VMU_DRIVE) {
        return (int)0xffffff01;
    }
    vmu_find = 0;
    return vmu_next(fname);
}

int buFindNextFile(int drive, char *fname)
{
    (void)drive;
    return vmu_next(fname);
}

int buGetFileSize(int drive, const char *fname)
{
    struct vmu_entry *e = vmu_lookup(fname);

    (void)drive;
    return e != 0 ? e->blocks : (int)0xffffff05;
}

int buIsExistFile(int drive, const char *fname)
{
    (void)drive;
    return vmu_lookup(fname) != 0 ? 0 : (int)0xffffff05;
}

int buLoadFile(int drive, const char *fname, void *buf, unsigned int nblock)
{
    struct vmu_entry *e = vmu_lookup(fname);

    (void)drive;
    if (e == 0) {
        vmu_post(11 /* BUD_OP_LOADFILE */, (int)0xffffff05);
        return (int)0xffffff05;
    }
    if (nblock > e->blocks) {
        nblock = e->blocks;
    }
    memcpy(buf, vmu.data + e->first * VMU_BLOCK, nblock * VMU_BLOCK);
    vmu_post(11, 0);
    return 0;
}

int buSaveFile(int drive, const char *fname, const void *buf, unsigned int nblock,
               void *time, unsigned int flag)
{
    struct vmu_entry *e = vmu_lookup(fname);
    int i;

    (void)drive;
    (void)flag;
    if (vmu_used() - (e != 0 ? e->blocks : 0) + (int)nblock > VMU_BLOCKS) {
        vmu_post(12, (int)0xffffff06); /* sem espaço */
        return (int)0xffffff06;
    }
    if (e != 0) {
        e->name[0] = 0; /* substitui */
    }
    vmu_pack();
    e = 0;
    for (i = 0; i < VMU_FILES && e == 0; i++) {
        if (vmu.files[i].name[0] == 0) {
            e = &vmu.files[i];
        }
    }
    if (e == 0) {
        vmu_post(12, (int)0xffffff06);
        return (int)0xffffff06;
    }
    /* entra no fim da tabela e dos blocos, já compactados */
    i = vmu_used();
    memset(e, 0, sizeof *e);
    strncpy(e->name, fname, 12);
    e->blocks = (unsigned short)nblock;
    e->first = (unsigned short)i;
    memcpy(vmu.data + e->first * VMU_BLOCK, buf, nblock * VMU_BLOCK);
    if (time != 0) {
        memcpy(e->time, time, 8);
    }
    vmu_store();
    zlog("bu", "save", (int)nblock);
    if (vmu_progress != 0) {
        vmu_progress(VMU_DRIVE, 12, (int)nblock, (int)nblock);
    }
    vmu_post(12, 0);
    return 0;
}

int buDeleteFile(int drive, const char *fname)
{
    struct vmu_entry *e = vmu_lookup(fname);

    (void)drive;
    if (e == 0) {
        return (int)0xffffff05;
    }
    e->name[0] = 0;
    vmu_store();
    vmu_post(7, 0);
    return 0;
}

/* --- memória flash do console (configuração de internet, src/save/mail.c) ---
 * Não há flash: fmMngInit falha e o jogo segue sem a configuração. O stub
 * gerado devolvia 0 (sucesso) e o servidor nunca terminava a leitura. */
int fmMngInit(int partition, void *work)
{
    (void)partition;
    (void)work;
    return -1;
}

int fmMngExecServer(void)
{
    return 3; /* FMD_STAT_ERR */
}

/* o trial.c dos menus chama estas duas do 1ST_READ pelos nomes menus_, sem
 * protótipo: cada argumento vem como int (engine/native/shim.c faz o mesmo) */
void set_character_number(short number);
void stage_set_stage_act(unsigned char stage, unsigned char act);

void menus_set_character_number(int number)
{
    set_character_number((short)number);
}

void menus_set_stage_act(int stage, int act)
{
    stage_set_stage_act((unsigned char)stage, (unsigned char)act);
}
