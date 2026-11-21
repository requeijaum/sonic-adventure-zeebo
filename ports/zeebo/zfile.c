/* zfile.c — leitura de arquivos pelo BREW IFileMgr (substitui o GDFS do
 * Dreamcast). Os assets do jogo vivem no armazenamento do app, não no .mod. */

#include <AEEStdLib.h>
#include <AEEShell.h>
#include <AEEClassIDs.h>
#include <AEEFile.h>

#include "zstrip.h"

static IFileMgr *fm;

int zfile_open(IShell *shell)
{
    if (fm != 0) {
        return 1;
    }
    if (shell == 0 ||
        ISHELL_CreateInstance(shell, AEECLSID_FILEMGR, (void **)&fm) !=
            AEE_SUCCESS) {
        fm = 0;
        return 0;
    }
    return 1;
}

void zfile_close(void)
{
    if (fm != 0) {
        IFILEMGR_Release(fm);
        fm = 0;
    }
}

/* tamanho do arquivo, ou -1 */
int zfile_size(const char *path)
{
    FileInfo info;

    if (fm == 0 || IFILEMGR_GetInfo(fm, path, &info) != SUCCESS) {
        return -1;
    }
    return (int)info.dwSize;
}

/* lê o arquivo inteiro em dst (até cap bytes); devolve os bytes lidos ou -1 */
int zfile_read(const char *path, void *dst, int cap)
{
    IFile *f;
    int total = 0;

    if (fm == 0 || (f = IFILEMGR_OpenFile(fm, path, _OFM_READ)) == 0) {
        return -1;
    }
    while (total < cap) {
        int n = IFILE_Read(f, (char *)dst + total, (uint32)(cap - total));

        if (n <= 0) {
            break;
        }
        total += n;
    }
    IFILE_Release(f);
    return total;
}

/* grava (cria/substitui) path; usado para instalar o pacote de assets */
int zfile_write(const char *path, const void *src, int len)
{
    IFile *f;
    int n;

    if (fm == 0) {
        return -1;
    }
    (void)IFILEMGR_Remove(fm, path);
    f = IFILEMGR_OpenFile(fm, path, _OFM_CREATE);
    if (f == 0) {
        return -1;
    }
    n = IFILE_Write(f, (const char *)src, (uint32)len);
    IFILE_Release(f);
    return n;
}

/* --- handles (para o shim de GDFS) --- */

void *zfile_handle_open(const char *path)
{
    return fm != 0 ? (void *)IFILEMGR_OpenFile(fm, path, _OFM_READ) : 0;
}

void zfile_handle_close(void *h)
{
    if (h != 0) {
        IFILE_Release((IFile *)h);
    }
}

int zfile_handle_read(void *h, void *dst, int len)
{
    return IFILE_Read((IFile *)h, (char *)dst, (uint32)len);
}

/* posiciona em offset bytes desde o início; 0 se ok */
int zfile_handle_seek(void *h, int offset)
{
    return IFILE_Seek((IFile *)h, _SEEK_START, offset) == SUCCESS ? 0 : -1;
}
