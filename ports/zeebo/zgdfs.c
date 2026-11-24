/* zgdfs.c — subconjunto do GDFS do Shinobi sobre o BREW IFileMgr.
 *
 * O jogo lê assets do GD-ROM por gdFsOpen -> gdFsGetFileSize -> gdFsRead
 * (setores de 2048 bytes) -> gdFsClose (src/system/loader.c). Aqui o mesmo
 * contrato lê arquivos do armazenamento do app. A leitura é síncrona e
 * completa na chamada, então stat/wait nunca ficam pendentes.
 *
 * Nomes: o disco ignora caixa e traz ";1" em alguns; tenta o nome exato,
 * depois sem sufixo ";N", depois em minúsculas (o pacote é gerado assim).
 * Não implementados (devolvem erro): diretórios, gdFsOpenRange, DMA/Trans32,
 * TOC. */

#include <kamui.h>
#include <shinobi.h>

#include "zstrip.h"
#include "zrebase.h"
#include "assets/prs.h"

#define SECTOR 2048
#define NHANDLE 8
#define NAMEMAX 64

struct GDS_FS_HANDLE {
    void *file; /* IFile; 0 = livre */
    Sint32 size;
    Sint32 pos; /* próximo setor */
    Sint32 stat;
    Sint32 err;
};

static struct GDS_FS_HANDLE handles[NHANDLE];
static Sint32 global_err;

static int try_open(const char *name, struct GDS_FS_HANDLE *h)
{
    h->file = zfile_handle_open(name);
    if (h->file == 0) {
        return 0;
    }
    h->size = zfile_size(name);
    if (h->size < 0) {
        zfile_handle_close(h->file);
        h->file = 0;
        return 0;
    }
    return 1;
}

void zlog(const char *what, const char *name, int value);

/* os programas de campo, stage, chefe, jardim e evento: o .mod ainda não os
 * tem (game.mk PROGRAMS só linka advertise e summary). Abrir um deles hoje
 * leva a um salto para dados do SH-4; em vez disso, o jogo recomeça. */
/* nomes dos programas sem código no .mod (game.mk gera zprogs.c a partir de
 * config/, sem os de PROGRAMS); fraco: o applet sozinho não tem a lista */
extern const char *const zmissing_programs[] __attribute__((weak));

/* 0 quando iguais, como strcmp (sem libc no applet) */
static int zstreq(const char *a, const char *b)
{
    while (*a != 0 && *a == *b) {
        a++;
        b++;
    }
    return *a != *b;
}

static int missing_program(const char *name)
{
    char low[24];
    int i;
    int dot = -1;

    if (zmissing_programs == 0) {
        return 0;
    }
    for (i = 0; i < 23 && name[i] != 0; i++) {
        low[i] = (char)(name[i] >= 'A' && name[i] <= 'Z' ? name[i] + 32 : name[i]);
        if (low[i] == '.') {
            dot = i;
        }
    }
    low[i] = 0;
    /* só o binário do programa: <nome>.prs ou <nome>.bin */
    if (dot <= 0 || !(zstreq(low + dot, ".prs") == 0 || zstreq(low + dot, ".bin") == 0)) {
        return 0;
    }
    low[dot] = 0;
    for (i = 0; zmissing_programs[i] != 0; i++) {
        if (zstreq(low, zmissing_programs[i]) == 0) {
            return 1;
        }
    }
    return 0;
}


/* overlays de dados dos programas (game.mk OVL_PROGRAMS, tools/gen_ovl.py): o
 * .data de cada programa mora em <P>.ovl (PRS) e e carregado na regiao unica
 * zovl_region quando o jogo abre <P>.prs/<P>.bin. Formato em gen_ovl.py. */
extern unsigned char zovl_region[] __attribute__((weak));
extern unsigned char zovl_end[] __attribute__((weak));
void *malloc(unsigned int n);
void free(void *p);
void *memcpy(void *d, const void *s, unsigned long n);

struct zovl_hdr {
    unsigned int magic, ro_ref, rw_ref, rw_start, nrel, rel_raw, rel_prs, dat_raw, dat_prs;
};

static unsigned int zovl_rd(const unsigned char *p)
{
    return (unsigned int)p[0] | (unsigned int)p[1] << 8 | (unsigned int)p[2] << 16 |
           (unsigned int)p[3] << 24;
}

static void zovl_load(const char *low)
{
    char name[40];
    unsigned char *buf, *rel;
    unsigned int h[11], i, delta_ro, delta_rw;
    unsigned char *region;
    int n, size;

    if (zovl_region == 0) {
        return;
    }
    for (n = 0; low[n] != 0 && n < 30; n++) {
        name[n] = low[n];
    }
    name[n++] = '.'; name[n++] = 'o'; name[n++] = 'v'; name[n++] = 'l'; name[n] = 0;
    size = zfile_size(name);
    if (size < 44) {
        return; /* programa sem overlay (advertise, summary, ...) */
    }
    buf = (unsigned char *)malloc((unsigned int)size);
    if (buf == 0 || zfile_read(name, buf, size) != size) {
        zlog("ovl", "falha ao ler", size);
        free(buf);
        return;
    }
    for (i = 0; i < 11; i++) {
        h[i] = zovl_rd(buf + 4 * i);
    }
    if (h[0] != 0x314c564fu) { /* 'OVL1' */
        zlog("ovl", "magia invalida", (int)h[0]);
        free(buf);
        return;
    }
    /* a regiao do grupo deste programa (modulo, field, extra ou tikal) */
    delta_rw = (unsigned int)(unsigned long)zovl_region - h[2];
    region = (unsigned char *)(unsigned long)(h[9] + delta_rw);
    if (h[7] > h[10]) {
        zlog("ovl", "maior que a regiao", (int)h[7]);
        free(buf);
        return;
    }
    rel = (unsigned char *)malloc(h[5] + 4);
    if (rel == 0 || prs_unpack(buf + 44, (int)h[6], rel, (int)h[5]) < 0 ||
        prs_unpack(buf + 44 + h[6], (int)h[8], region, (int)h[7]) < 0) {
        zlog("ovl", "PRS invalido", size);
        free(rel);
        free(buf);
        return;
    }
    /* reloca: a diferenca entre o endereco real e o de link de cada classe */
    delta_ro = (unsigned int)(unsigned long)&gdFsOpen - h[1];
    for (i = 0; i < h[4]; i++) {
        unsigned int e = zovl_rd(rel + 4 * i);
        unsigned char *p = region + (e & 0x7fffffffu);
        unsigned int v = zovl_rd(p) + ((e >> 31) != 0 ? delta_rw : delta_ro);

        p[0] = (unsigned char)v;
        p[1] = (unsigned char)(v >> 8);
        p[2] = (unsigned char)(v >> 16);
        p[3] = (unsigned char)(v >> 24);
    }
    zlog("ovl", name, (int)h[7]);
    free(rel);
    free(buf);
}

/* zdups.c (gerado): programa linkado aberto -> seus dados duplicados */
void zprog_enter(const char *low) __attribute__((weak));

/* o último nome aberto: o som do jogo lê um MLT e o entrega logo ao sdMultiUnitDownload */
static char last_name[28];

const char *zgdfs_last_name(void) { return last_name; }

GDFS gdFsOpen(const char *fname, GDFS_DIRREC gf_dirrec)
{
    if (fname != 0) {
        int q;

        for (q = 0; q < 27 && fname[q] != 0; q++) {
            last_name[q] = fname[q];
        }
        last_name[q] = 0;
    }
    if (fname != 0 && (zprog_enter != 0 || zovl_region != 0)) {
        char low[24];
        int k, dot = -1;

        for (k = 0; k < 23 && fname[k] != 0; k++) {
            low[k] = (char)(fname[k] >= 'A' && fname[k] <= 'Z' ? fname[k] + 32 : fname[k]);
            if (low[k] == '.') {
                dot = k;
            }
        }
        low[k] = 0;
        if (dot > 0 && (zstreq(low + dot, ".prs") == 0 || zstreq(low + dot, ".bin") == 0)) {
            low[dot] = 0;
            zovl_load(low);
            if (zprog_enter != 0) {
                zprog_enter(low);
            }
        }
    }
    if (fname != 0 && missing_program(fname)) {
        void zgame_reset(void) __attribute__((weak));

        zlog("gdFs", "programa ausente, reset", 0);
        zlog("gdFs", fname, 0);
        if (zgame_reset != 0) {
            zgame_reset();
        }
    }
    char buf[NAMEMAX];
    int n = 0;
    int i;
    struct GDS_FS_HANDLE *h = 0;

    (void)gf_dirrec;
    for (i = 0; i < NHANDLE && h == 0; i++) {
        if (handles[i].file == 0) {
            h = &handles[i];
        }
    }
    if (h == 0) {
        global_err = GDD_ERR_NOHNDL;
        return 0;
    }
    if (fname == 0) {
        global_err = GDD_ERR_NOTFOUND;
        return 0;
    }
    /* copia sem o sufixo ";N" */
    while (fname[n] != 0 && fname[n] != ';' && n < NAMEMAX - 1) {
        buf[n] = fname[n];
        n++;
    }
    buf[n] = 0;
    if (!try_open(buf, h)) {
        for (i = 0; i < n; i++) {
            if (buf[i] >= 'A' && buf[i] <= 'Z') {
                buf[i] = (char)(buf[i] + 32);
            }
        }
        if (!try_open(buf, h)) {
            zlog("gdFsOpen MISS", fname, 0);
            global_err = GDD_ERR_NOTFOUND;
            return 0;
        }
    }
    zlog("gdFsOpen ok", fname, h->size);
    h->pos = 0;
    h->stat = GDD_STAT_IDLE;
    h->err = GDD_ERR_OK;
    global_err = GDD_ERR_OK;
    return h;
}

Bool gdFsGetFileSize(GDFS gdfs, Sint32 *fsize)
{
    if (gdfs == 0 || gdfs->file == 0 || fsize == 0) {
        return FALSE;
    }
    *fsize = gdfs->size;
    return TRUE;
}

Sint32 gdFsSeek(GDFS gdfs, Sint32 sctno, Sint32 type)
{
    Sint32 total;

    if (gdfs == 0 || gdfs->file == 0) {
        return GDD_ERR_ILLHNDL;
    }
    total = (gdfs->size + SECTOR - 1) / SECTOR;
    if (type == GDD_SEEK_CUR) {
        sctno += gdfs->pos;
    } else if (type == GDD_SEEK_END) {
        sctno = total - sctno;
    }
    if (sctno < 0 || sctno > total ||
        zfile_handle_seek(gdfs->file, sctno * SECTOR) != 0) {
        return GDD_ERR_ILLREQ;
    }
    gdfs->pos = sctno;
    return GDD_ERR_OK;
}

Sint32 gdFsTell(GDFS gdfs)
{
    return (gdfs != 0 && gdfs->file != 0) ? gdfs->pos : GDD_ERR_ILLHNDL;
}

Sint32 gdFsCheckEof(GDFS gdfs, Bool *iseof)
{
    if (gdfs == 0 || gdfs->file == 0 || iseof == 0) {
        return GDD_ERR_ILLHNDL;
    }
    *iseof = (gdfs->pos * SECTOR >= gdfs->size) ? TRUE : FALSE;
    return GDD_ERR_OK;
}

/* o jogo lê MOVIE.BIN por cima de movie_entry e outros programas por cima de
 * loader_entry: aqui são funções em ROM, então a leitura anda o arquivo e não
 * grava (até haver carga de programas) */
extern int movie_entry(int list) __attribute__((weak));
extern void loader_entry(void) __attribute__((weak));

/* arquivos com ponteiros do DC lidos para dentro da arena (hoje só as falas
 * do campo, field_messages em zrebase.c): cada palavra alinhada que aponta
 * para a arena (0x0c900000..0x0cffffff, ou 0x8c...) vira o endereço ARM */
extern unsigned char field_messages[] __attribute__((weak));
#define FIELD_MESSAGES_MAX 0x8000
static void zgdfs_relocate(void *buf, int bytes)
{
    unsigned long *w = (unsigned long *)buf;
    int n = bytes / 4;
    int i;
    int moved = 0;

    if (field_messages == 0 || (unsigned char *)buf < field_messages ||
        (unsigned char *)buf >= field_messages + FIELD_MESSAGES_MAX ||
        ((unsigned long)buf & 3u) != 0) {
        return;
    }
    for (i = 0; i < n; i++) {
        unsigned long a = w[i] & 0x1fffffffu;

        if ((w[i] >> 29) != 0 && (w[i] >> 29) != 4) {
            continue; /* só P0/P1 (0x0c... e 0x8c...) */
        }
        if (a >= 0x0c900000u && a < 0x0d000000u) {
            w[i] = ZDC(a);
            moved++;
        }
    }
    zlog("gdFs", "ponteiros do DC relocados", moved);
}

extern unsigned int zgdfs_now(void);
/* lê até nsct setores a partir do próximo; para no fim do arquivo */
Sint32 gdFsRead(GDFS gdfs, Sint32 nsct, void *buf)
{
    Sint32 want;
    Sint32 left;
    int got;

    if (gdfs == 0 || gdfs->file == 0 || buf == 0 || nsct < 0) {
        return GDD_ERR_ILLHNDL;
    }
    left = gdfs->size - gdfs->pos * SECTOR;
    want = nsct * SECTOR;
    if (want > left) {
        want = left;
    }
    if ((movie_entry != 0 && buf == (void *)movie_entry) ||
        (loader_entry != 0 && buf == (void *)loader_entry)) {
        zlog("gdFs", "leitura sobre codigo, ignorada", want);
        got = want;
    } else {
        {
            unsigned int t0 = zgdfs_now();

            got = want > 0 ? zfile_handle_read(gdfs->file, buf, want) : 0;
            t0 = zgdfs_now() - t0;
            if (t0 > 50) {
                zlog("gdFs", "leitura lenta ms", (int)t0);
                zlog("gdFs", "bytes", want);
            }
        }
    }
    zgdfs_relocate(buf, got);
    if (got < want) {
        gdfs->stat = GDD_STAT_ERR;
        gdfs->err = GDD_ERR_FATAL;
        return GDD_ERR_FATAL;
    }
    gdfs->pos += (got + SECTOR - 1) / SECTOR;
    gdfs->stat = GDD_STAT_COMPLETE;
    gdfs->err = GDD_ERR_OK;
    return GDD_ERR_OK;
}

Sint32 gdFsGetStat(GDFS gdfs)
{
    zlog("poll", "gdFsGetStat", (int)__builtin_return_address(0));
    return (gdfs != 0 && gdfs->file != 0) ? gdfs->stat : GDD_STAT_FATAL;
}

Sint32 gdFsGetErrStat(GDFS gdfs)
{
    return (gdfs != 0 && gdfs->file != 0) ? gdfs->err : GDD_ERR_ILLHNDL;
}

Sint32 gdFsStopRd(GDFS gdfs)
{
    if (gdfs != 0 && gdfs->file != 0) {
        gdfs->stat = GDD_STAT_IDLE;
    }
    return GDD_ERR_OK;
}

void gdFsClose(GDFS gdfs)
{
    if (gdfs != 0 && gdfs->file != 0) {
        zfile_handle_close(gdfs->file);
        gdfs->file = 0;
    }
}

/* como loader_read do jogo: abre, mede, lê em setores, fecha. Devolve o
 * tamanho em bytes ou -1. dst precisa de cap >= tamanho arredondado a setor */
int zgdfs_read_file(const char *name, void *dst, int cap)
{
    Sint32 size;
    Sint32 sectors;
    GDFS f = gdFsOpen(name, 0);

    if (f == 0) {
        return -1;
    }
    if (!gdFsGetFileSize(f, &size) || size > cap) {
        gdFsClose(f);
        return -1;
    }
    sectors = (size + SECTOR - 1) / SECTOR;
    if (gdFsRead(f, sectors, dst) != GDD_ERR_OK) {
        gdFsClose(f);
        return -1;
    }
    gdFsClose(f);
    return size;
}

/* --- ciclo de vida e estado da unidade ----------------------------------
 * Não há unidade: a "mídia" é o armazenamento do app. A unidade fica sempre
 * pronta (nem OPEN nem BUSY), então o loader nunca pede troca de disco. */

Sint32 gdFsInit(Sint32 max_open, void *gdfs_work, Sint32 max_dirent,
                void *dirbuf)
{
    (void)max_open;
    (void)gdfs_work;
    (void)max_dirent;
    (void)dirbuf;
    global_err = GDD_ERR_OK;
    return GDD_ERR_OK;
}

void gdFsFinish(void)
{
    int i;

    for (i = 0; i < NHANDLE; i++) {
        gdFsClose(&handles[i]);
    }
}

void gdFsExecServer(void)
{
}

/* DC: 2 = STANDBY (disco girando, pronto) */
Sint32 gdFsGetDrvStat(void)
{
    return 2;
}

Sint32 gdFsReqDrvStat(void)
{
    return GDD_ERR_OK;
}

void gdFsEntryErrFuncAll(GDFS_ERRFUNC errfunc, void *obj)
{
    (void)errfunc; /* nenhuma leitura falha de forma assíncrona */
    (void)obj;
}

Sint32 gdFsChangeDir(const char *dirname)
{
    (void)dirname; /* pacote plano: só o diretório do app */
    return GDD_ERR_OK;
}

Sint32 gdFsGetDirInfo(const char *name, GDFS_DIRINFO *dirinfo)
{
    GDFS f = gdFsOpen(name, 0);

    if (f == 0) {
        return GDD_ERR_NOTFOUND;
    }
    if (dirinfo != 0) {
        dirinfo->fad = 0; /* sem endereço de setor no armazenamento */
        dirinfo->fsize = f->size;
        dirinfo->flag = 0;
    }
    gdFsClose(f);
    return GDD_ERR_OK;
}

GDFS gdFsOpenRange(Sint32 stsct, Sint32 nsct)
{
    (void)stsct;
    (void)nsct;
    global_err = GDD_ERR_ILLREQ; /* sem endereçamento por setor absoluto */
    return 0;
}

Sint32 gdFsReqRd32(GDFS gdfs, Sint32 nsct, void *buf)
{
    return gdFsRead(gdfs, nsct, buf); /* síncrono, como gdFsRead */
}
