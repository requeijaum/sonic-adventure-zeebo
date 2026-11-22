/* sfdplay: dá para decodificar os filmes Sofdec do SA1 no ARM do Zeebo?
 *
 * Os .sfd são MPEG-PS: vídeo MPEG-1 320x240 a 30 fps (stream 0xE0) e áudio
 * ADX num stream de áudio que o ffprobe chama de mp2. Aqui só o vídeo, com
 * pl_mpeg (phoboslab, MIT; o cabeçalho é baixado no build, não vai no git),
 * lido do arquivo ao lado do .mod pelo IFile em blocos de 64 KB.
 *
 * Mede: quadros decodificados por segundo de relógio (decode puro e decode +
 * conversão YUV -> RGB565). Escreve no DBGPRINTF e na tela. */
#include "AEEStdLib.h"
#include "AEEAppGen.h"
#include "AEEShell.h"
#include "AEEFile.h"

#define PLM_NO_STDIO
#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#define AEECLSID_SFDPLAY 0x0BE07005
#define SFD_NAME "sa1_600.sfd"
#define BENCH_FRAMES 300

/* libc que o pl_mpeg pede, pelo helper do BREW */
void *memset(void *d, int c, size_t n) { return MEMSET(d, c, n); }
void *memcpy(void *d, const void *s, size_t n) { return MEMMOVE(d, s, n); }
void *memmove(void *d, const void *s, size_t n) { return MEMMOVE(d, s, n); }
void *malloc(size_t n) { return MALLOC(n); }
void *realloc(void *p, size_t n) { return REALLOC(p, n); }
void free(void *p) { FREE(p); }

typedef struct {
    AEEApplet a;
    IFileMgr *fm;
    IFile *file;
    plm_t *plm; /* não usado: o cabeçalho de sistema do Sofdec não conta o vídeo */
    plm_demux_t *demux;
    plm_buffer_t *vbuf;
    plm_video_t *video;
    unsigned short *rgb;
    int frames;
    int convert;      /* 0: só decode; 1: decode + RGB565 */
    uint32 t0;
    uint32 decode_ms;
    char line[6][64];
    int lines;
    AEECallback tick;
} Sfd;

static void say(Sfd *s, const char *txt)
{
    AECHAR w[64];
    int i;

    DBGPRINTF("sfdplay: %s", txt);
    if (s->lines == 6) {
        for (i = 1; i < 6; i++) STRLCPY(s->line[i - 1], s->line[i], 64);
        s->lines = 5;
    }
    STRLCPY(s->line[s->lines++], txt, 64);
    IDISPLAY_ClearScreen(s->a.m_pIDisplay);
    for (i = 0; i < s->lines; i++) {
        STRTOWSTR(s->line[i], w, sizeof w);
        IDISPLAY_DrawText(s->a.m_pIDisplay, AEE_FONT_NORMAL, w, -1, 8, 8 + 20 * i,
                          NULL, IDF_TEXT_TRANSPARENT);
    }
    IDISPLAY_Update(s->a.m_pIDisplay);
}

static void load(plm_buffer_t *b, void *user)
{
    Sfd *s = user;
    static uint8_t chunk[64 * 1024];
    int32 got;

    if (s->file == NULL) {
        plm_buffer_signal_end(b);
        return;
    }
    got = IFILE_Read(s->file, chunk, sizeof chunk);
    {
        static int first = 1;
        if (first) { first = 0; DBGPRINTF("sfdplay: primeira leitura %d", (int)got); }
    }
    if (got <= 0) {
        plm_buffer_signal_end(b);
        return;
    }
    plm_buffer_write(b, chunk, (size_t)got);
}

/* YUV 4:2:0 -> RGB565, inteiro */
static void to_rgb565(plm_frame_t *f, unsigned short *out)
{
    int x, y;
    int w = f->width, h = f->height;

    for (y = 0; y < h; y++) {
        const uint8_t *py = f->y.data + y * f->y.width;
        const uint8_t *pcb = f->cb.data + (y >> 1) * f->cb.width;
        const uint8_t *pcr = f->cr.data + (y >> 1) * f->cr.width;
        unsigned short *o = out + y * w;

        for (x = 0; x < w; x++) {
            int Y = (py[x] - 16) * 298;
            int cb = pcb[x >> 1] - 128, cr = pcr[x >> 1] - 128;
            int r = (Y + 409 * cr + 128) >> 8;
            int g = (Y - 100 * cb - 208 * cr + 128) >> 8;
            int b = (Y + 516 * cb + 128) >> 8;

            r = r < 0 ? 0 : r > 255 ? 255 : r;
            g = g < 0 ? 0 : g > 255 ? 255 : g;
            b = b < 0 ? 0 : b > 255 ? 255 : b;
            o[x] = (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        }
    }
}

/* o decoder de vídeo pede bytes: passa os pacotes 0xE0 do demux */
static void feed_video(plm_buffer_t *vb, void *user)
{
    Sfd *s = user;
    plm_packet_t *p;

    while ((p = plm_demux_decode(s->demux)) != NULL) {
        if (p->type == PLM_DEMUX_PACKET_VIDEO_1) {
            plm_buffer_write(vb, p->data, p->length);
            return;
        }
    }
    plm_buffer_signal_end(vb);
}

static int open_movie(Sfd *s)
{
    plm_buffer_t *b;

    s->file = IFILEMGR_OpenFile(s->fm, SFD_NAME, _OFM_READ);
    if (s->file == NULL) {
        say(s, "sem " SFD_NAME);
        return 0;
    }
    b = plm_buffer_create_with_capacity(256 * 1024);
    plm_buffer_set_load_callback(b, load, s);
    /* o plm_t de alto nível olha o cabeçalho de sistema, onde o Sofdec só
     * declara o áudio; o demux e o vídeo direto ignoram isso */
    s->demux = plm_demux_create(b, 1);
    s->vbuf = plm_buffer_create_with_capacity(256 * 1024);
    plm_buffer_set_load_callback(s->vbuf, feed_video, s);
    s->video = plm_video_create_with_buffer(s->vbuf, 1);
    return 1;
}

static void close_movie(Sfd *s)
{
    if (s->video) { plm_video_destroy(s->video); s->video = NULL; }
    if (s->demux) { plm_demux_destroy(s->demux); s->demux = NULL; }
    if (s->file) { IFILE_Release(s->file); s->file = NULL; }
}

static void step(void *user)
{
    Sfd *s = user;
    char buf[64];
    int i;

    /* um lote de quadros por chamada, para não segurar o shell demais */
    for (i = 0; i < 10 && s->frames < BENCH_FRAMES; i++) {
        uint32 a = GETUPTIMEMS();
        plm_frame_t *f = plm_video_decode(s->video);

        if (f == NULL) {
            s->frames = BENCH_FRAMES;
            break;
        }
        if (s->convert) {
            to_rgb565(f, s->rgb);
        }
        s->decode_ms += GETUPTIMEMS() - a;
        s->frames++;
        if (s->frames == 1) {
            SNPRINTF(buf, sizeof buf, "%dx%d %d.%02d fps", f->width, f->height,
                     (int)plm_video_get_framerate(s->video),
                     (int)(plm_video_get_framerate(s->video) * 100) % 100);
            say(s, buf);
        }
    }
    if (s->frames >= BENCH_FRAMES) {
        uint32 ms = s->decode_ms ? s->decode_ms : 1;

        SNPRINTF(buf, sizeof buf, "%s: %d quadros %d ms = %d.%d fps",
                 s->convert ? "decode+rgb" : "decode", s->frames, (int)ms,
                 (int)(s->frames * 1000 / ms), (int)(s->frames * 10000 / ms) % 10);
        say(s, buf);
        close_movie(s);
        if (!s->convert) {
            s->convert = 1;
            s->frames = 0;
            s->decode_ms = 0;
            if (!open_movie(s)) return;
        } else {
            say(s, "fim");
            return;
        }
    }
    ISHELL_SetTimerEx(s->a.m_pIShell, 1, &s->tick);
}

static boolean handle(Sfd *s, AEEEvent ev, uint16 w, uint32 dw)
{
    (void)w; (void)dw;
    switch (ev) {
    case EVT_APP_START:
        say(s, "sfdplay 1 (pl_mpeg)");
        if (ISHELL_CreateInstance(s->a.m_pIShell, AEECLSID_FILEMGR, (void **)&s->fm) != SUCCESS) {
            say(s, "sem IFileMgr");
            return TRUE;
        }
        s->rgb = MALLOC(320 * 240 * 2);
        if (!open_movie(s)) return TRUE;
        CALLBACK_Init(&s->tick, step, s);
        ISHELL_SetTimerEx(s->a.m_pIShell, 1, &s->tick);
        return TRUE;
    case EVT_APP_STOP:
        CALLBACK_Cancel(&s->tick);
        close_movie(s);
        if (s->fm) IFILEMGR_Release(s->fm);
        return TRUE;
    default:
        return FALSE;
    }
}

int AEEClsCreateInstance(AEECLSID cls, IShell *shell, IModule *mod, void **obj)
{
    *obj = NULL;
    if (cls != AEECLSID_SFDPLAY) return EFAILED;
    return AEEApplet_New(sizeof(Sfd), cls, shell, mod, (IApplet **)obj,
                         (AEEHANDLER)handle, NULL) ? SUCCESS : EFAILED;
}
