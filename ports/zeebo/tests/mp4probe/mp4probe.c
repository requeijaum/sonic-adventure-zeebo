/* mp4probe: o Zeebo toca vídeo pelo IMedia do BREW?
 *
 * Para cada arquivo de VIDEOS (ao lado do .mod), tenta:
 *   1. ISHELL_CreateInstance(AEECLSID_MEDIAMPEG4)      -> "cls"
 *   2. IMediaUtil_CreateMedia(MMD_FILE_NAME)          -> "util" (e a classe)
 *   3. SetRect tela cheia, RegisterNotify, Play       -> "play"
 * e conta o que o IMedia avisa: START, FRAME, DONE, ABORT e o último status.
 * Cada arquivo tem 15 s; qualquer tecla passa para o próximo.
 *
 * O resultado aparece na tela (texto por cima do vídeo, embaixo), no
 * DBGPRINTF e em mp4probe.txt ao lado do .mod. */
#include "AEEStdLib.h"
#include "AEEAppGen.h"
#include "AEEShell.h"
#include "AEEFile.h"
#include "AEEMedia.h"
#include "AEEMediaUtil.h"

#define AEECLSID_MP4PROBE 0x0BE07004

static const char *const VIDEOS[] = {"probe.mp4", "probe264.mp4", 0};

typedef struct {
    AEEApplet a;
    IMedia *media;
    int file;          /* índice em VIDEOS */
    int ms;            /* tempo no arquivo atual */
    int n_start, n_frame, n_done, n_abort, n_other, last_status, last_cmd;
    char line[8][64];  /* linhas fixas na tela */
    int lines;
    AEECallback tick;
} Probe;

static void log_line(Probe *p, const char *s)
{
    IFileMgr *fm;

    DBGPRINTF("mp4probe: %s", s);
    if (p->lines < 8) {
        STRLCPY(p->line[p->lines++], s, 64);
    }
    if (ISHELL_CreateInstance(p->a.m_pIShell, AEECLSID_FILEMGR, (void **)&fm) == SUCCESS) {
        IFile *f = IFILEMGR_OpenFile(fm, "mp4probe.txt", _OFM_APPEND);

        if (f == NULL) {
            f = IFILEMGR_OpenFile(fm, "mp4probe.txt", _OFM_CREATE);
        }
        if (f != NULL) {
            IFILE_Write(f, s, STRLEN(s));
            IFILE_Write(f, "\n", 1);
            IFILE_Release(f);
        }
        IFILEMGR_Release(fm);
    }
}

static void draw(Probe *p)
{
    AEEDeviceInfo di;
    AECHAR w[64];
    char buf[64];
    int i, y;

    di.wStructSize = sizeof di;
    ISHELL_GetDeviceInfo(p->a.m_pIShell, &di);
    y = di.cyScreen - 16 * (p->lines + 1);
    for (i = 0; i < p->lines; i++, y += 16) {
        STRTOWSTR(p->line[i], w, sizeof w);
        IDISPLAY_DrawText(p->a.m_pIDisplay, AEE_FONT_NORMAL, w, -1, 4, y, NULL,
                          IDF_TEXT_TRANSPARENT);
    }
    SNPRINTF(buf, sizeof buf, "S%d F%d D%d A%d o%d st%d t%d", p->n_start,
             p->n_frame, p->n_done, p->n_abort, p->n_other, p->last_status,
             p->ms / 1000);
    STRTOWSTR(buf, w, sizeof w);
    IDISPLAY_DrawText(p->a.m_pIDisplay, AEE_FONT_NORMAL, w, -1, 4, y, NULL,
                      IDF_TEXT_TRANSPARENT);
    IDISPLAY_Update(p->a.m_pIDisplay);
}

static void notify(void *user, AEEMediaCmdNotify *n)
{
    Probe *p = user;

    p->last_status = n->nStatus;
    p->last_cmd = n->nCmd;
    switch (n->nStatus) {
    case MM_STATUS_START: p->n_start++; break;
    case MM_STATUS_FRAME: p->n_frame++; break;
    case MM_STATUS_DONE: p->n_done++; break;
    case MM_STATUS_ABORT: p->n_abort++; break;
    default: p->n_other++; break;
    }
}

static void finish_file(Probe *p)
{
    char buf[64];

    SNPRINTF(buf, sizeof buf, "%s: S%d F%d D%d A%d o%d st%d", VIDEOS[p->file],
             p->n_start, p->n_frame, p->n_done, p->n_abort, p->n_other,
             p->last_status);
    if (p->media != NULL) {
        IMEDIA_Stop(p->media);
        IMEDIA_Release(p->media);
        p->media = NULL;
    }
    p->lines = p->lines > 0 ? p->lines - 1 : 0; /* troca a linha "play" */
    log_line(p, buf);
    p->file++;
}

static void start_file(Probe *p)
{
    const char *name = VIDEOS[p->file];
    IMedia *m = NULL;
    IMediaUtil *u = NULL;
    AEEMediaData md;
    AEEDeviceInfo di;
    AEERect r;
    char buf[64];
    int e;

    p->ms = 0;
    p->n_start = p->n_frame = p->n_done = p->n_abort = p->n_other = 0;
    p->last_status = -1;

    e = ISHELL_CreateInstance(p->a.m_pIShell, AEECLSID_MEDIAMPEG4, (void **)&m);
    SNPRINTF(buf, sizeof buf, "%s cls MPEG4 e=%d", name, e);
    log_line(p, buf);
    if (m != NULL) {
        IMEDIA_Release(m);
        m = NULL;
    }

    md.clsData = MMD_FILE_NAME;
    md.pData = (void *)name;
    md.dwSize = 0;
    e = ISHELL_CreateInstance(p->a.m_pIShell, AEECLSID_MEDIAUTIL, (void **)&u);
    if (e == SUCCESS) {
        e = IMEDIAUTIL_CreateMedia(u, &md, &m);
        IMEDIAUTIL_Release(u);
    }
    {
        AEECLSID c = 0;

        if (m != NULL) {
            IMEDIA_GetClassID(m, &c);
        }
        SNPRINTF(buf, sizeof buf, "util e=%d cls=0x%x", e, (unsigned)c);
        log_line(p, buf);
    }
    if (m == NULL) {
        log_line(p, "sem IMedia para o arquivo");
        p->file++;
        return;
    }
    di.wStructSize = sizeof di;
    ISHELL_GetDeviceInfo(p->a.m_pIShell, &di);
    SETAEERECT(&r, 0, 0, di.cxScreen, di.cyScreen - 16 * 9);
    IMEDIA_SetRect(m, &r, NULL);
    IMEDIA_RegisterNotify(m, notify, p);
    e = IMEDIA_Play(m);
    SNPRINTF(buf, sizeof buf, "play e=%d (%dx%d)", e, di.cxScreen, di.cyScreen);
    log_line(p, buf);
    p->media = m;
}

static void tick(void *user)
{
    Probe *p = user;

    if (VIDEOS[p->file] == 0) {
        if (p->ms == 0) {
            log_line(p, "fim do teste");
        }
        p->ms = 1;
        draw(p);
        return;
    }
    if (p->media == NULL) {
        start_file(p);
    } else {
        p->ms += 250;
        if (p->ms >= 15000 || p->n_done || p->n_abort) {
            finish_file(p);
        }
    }
    draw(p);
    ISHELL_SetTimerEx(p->a.m_pIShell, 250, &p->tick);
}

static boolean handle(Probe *p, AEEEvent ev, uint16 w, uint32 dw)
{
    (void)dw;
    switch (ev) {
    case EVT_APP_START:
        IDISPLAY_ClearScreen(p->a.m_pIDisplay);
        log_line(p, "mp4probe 1");
        CALLBACK_Init(&p->tick, tick, p);
        ISHELL_SetTimerEx(p->a.m_pIShell, 250, &p->tick);
        return TRUE;
    case EVT_KEY:
        (void)w;
        if (p->media != NULL) {
            finish_file(p);
        }
        return TRUE;
    case EVT_APP_STOP:
        CALLBACK_Cancel(&p->tick);
        if (p->media != NULL) {
            IMEDIA_Release(p->media);
            p->media = NULL;
        }
        return TRUE;
    default:
        return FALSE;
    }
}

int AEEClsCreateInstance(AEECLSID cls, IShell *shell, IModule *mod, void **obj)
{
    *obj = NULL;
    if (cls != AEECLSID_MP4PROBE) {
        return EFAILED;
    }
    return AEEApplet_New(sizeof(Probe), cls, shell, mod, (IApplet **)obj,
                         (AEEHANDLER)handle, NULL)
               ? SUCCESS
               : EFAILED;
}
