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

GDFS gdFsOpen(const char *fname, GDFS_DIRREC gf_dirrec)
{
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
        got = want > 0 ? zfile_handle_read(gdfs->file, buf, want) : 0;
    }
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
