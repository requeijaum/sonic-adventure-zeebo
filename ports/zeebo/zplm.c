/* pl_mpeg (phoboslab, MIT) e o lado dos filmes que mexe nas estruturas
 * dele, compilados à parte: com o prelúdio e os cabeçalhos do jogo o
 * zmovie.c lia os pacotes com outro layout e o decoder recebia lixo. */
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* cópias próprias: com o memmove que o jogo linka (helper do BREW) o
 * decoder recebia bits errados e devolvia quadros vazios */
static void *zplm_memmove(void *d, const void *s, size_t n)
{
    unsigned char *dd = d;
    const unsigned char *ss = s;

    if (dd == ss || n == 0) {
        return d;
    }
    if (dd < ss) {
        /* para frente, de 4 em 4 quando os dois estão alinhados */
        if ((((unsigned long)dd | (unsigned long)ss) & 3) == 0) {
            while (n >= 16) {
                unsigned int *dw = (unsigned int *)dd;
                const unsigned int *sw = (const unsigned int *)ss;
                unsigned int a0 = sw[0], a1 = sw[1], a2 = sw[2], a3 = sw[3];

                dw[0] = a0; dw[1] = a1; dw[2] = a2; dw[3] = a3;
                dd += 16; ss += 16; n -= 16;
            }
        }
        while (n--) *dd++ = *ss++;
    } else {
        dd += n;
        ss += n;
        while (n--) *--dd = *--ss;
    }
    return d;
}
static void *zplm_memset(void *d, int c, size_t n)
{
    unsigned char *dd = d;
    unsigned int w = (unsigned char)c * 0x01010101u;

    while (n && ((unsigned long)dd & 3)) { *dd++ = (unsigned char)c; n--; }
    while (n >= 16) {
        unsigned int *dw = (unsigned int *)dd;

        dw[0] = w; dw[1] = w; dw[2] = w; dw[3] = w;
        dd += 16; n -= 16;
    }
    while (n--) *dd++ = (unsigned char)c;
    return d;
}
#define memmove zplm_memmove
#define memcpy zplm_memmove
#define memset zplm_memset
#define PLM_NO_STDIO
#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

void zlog(const char *what, const char *name, int value);
int zfile_handle_read(void *h, void *dst, int len);

struct zplm {
    void *file;
    plm_demux_t *demux;
    plm_video_t *video;
    plm_frame_t *frame;
};

static unsigned char chunk[32 * 1024];

static void read_more(plm_buffer_t *b, void *user)
{
    struct zplm *z = user;
    int got = zfile_handle_read(z->file, chunk, sizeof chunk);

    if (got <= 0) {
        plm_buffer_signal_end(b);
        return;
    }
    plm_buffer_write(b, chunk, (size_t)got);
}

/* o vídeo pede bytes: os pacotes 0xE0 do demux (o cabeçalho de sistema do
 * Sofdec só declara o áudio, então o plm_t de alto nível não serve) */
static void feed_video(plm_buffer_t *vb, void *user)
{
    struct zplm *z = user;
    plm_packet_t *p;

    while ((p = plm_demux_decode(z->demux)) != NULL) {
        if (p->type == PLM_DEMUX_PACKET_VIDEO_1) {
            plm_buffer_write(vb, p->data, p->length);
            return;
        }
    }
    plm_buffer_signal_end(vb);
}

struct zplm *zplm_open(void *file)
{
    struct zplm *z = malloc(sizeof *z);
    plm_buffer_t *b, *vb;

    if (z == NULL) {
        return NULL;
    }
    z->file = file;
    z->frame = NULL;
    b = plm_buffer_create_with_capacity(256 * 1024);
    plm_buffer_set_load_callback(b, read_more, z);
    z->demux = plm_demux_create(b, 1);
    vb = plm_buffer_create_with_capacity(256 * 1024);
    plm_buffer_set_load_callback(vb, feed_video, z);
    z->video = plm_video_create_with_buffer(vb, 1);
    return z;
}

/* liga/desliga o pulo dos quadros B (o decoder entrega o anterior) */
void zplm_skip_b(struct zplm *z, int on)
{
    if (z != NULL && z->video != NULL) {
        plm_video_set_skip_b(z->video, on);
    }
}

int zplm_decode(struct zplm *z)
{
    if (z == NULL) {
        return 0;
    }
    z->frame = plm_video_decode(z->video);
    return z->frame != NULL;
}

/* YUV 4:2:0 -> RGB565, inteiro */
int zplm_convert(struct zplm *z, unsigned short *out, int maxw, int maxh,
                 int *pw, int *ph)
{
    const plm_frame_t *f = z ? z->frame : NULL;
    int x, y, w, h;

    if (f == NULL) {
        return 0;
    }
    int step = 1;

    /* os filmes 320x448 (SLH448/SAH448) cabem em 256 linhas pulando uma */
    while ((int)f->height / step > maxh) {
        step++;
    }
    w = (int)f->width < maxw ? (int)f->width : maxw;
    h = (int)f->height / step;
    for (y = 0; y < h; y++) {
        int sy = y * step;
        const uint8_t *py = f->y.data + sy * f->y.width;
        const uint8_t *pcb = f->cb.data + (sy >> 1) * f->cb.width;
        const uint8_t *pcr = f->cr.data + (sy >> 1) * f->cr.width;
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
    *pw = w;
    *ph = h;
    return 1;
}

void zplm_close(struct zplm *z)
{
    if (z == NULL) {
        return;
    }
    plm_video_destroy(z->video);
    plm_demux_destroy(z->demux);
    free(z);
}

/* o áudio de um .sfd: os pacotes 0xC0 juntos formam um ADX inteiro.
 * Devolve o buffer (malloc) e o tamanho, ou NULL. Lê o arquivo todo. */
unsigned char *zplm_extract_audio(void *file, int *len)
{
    struct zplm z;
    plm_buffer_t *b = plm_buffer_create_with_capacity(256 * 1024);
    plm_demux_t *d;
    plm_packet_t *p;
    unsigned char *out = NULL;
    size_t used = 0, cap = 0;

    z.file = file;
    plm_buffer_set_load_callback(b, read_more, &z);
    d = plm_demux_create(b, 1);
    while ((p = plm_demux_decode(d)) != NULL) {
        if (p->type != PLM_DEMUX_PACKET_AUDIO_1) {
            continue;
        }
        if (used + p->length > cap) {
            size_t ncap = cap ? cap * 2 : 1024 * 1024;
            unsigned char *n;

            while (ncap < used + p->length) {
                ncap *= 2;
            }
            n = realloc(out, ncap);
            if (n == NULL) {
                break;
            }
            out = n;
            cap = ncap;
        }
        zplm_memmove(out + used, p->data, p->length);
        used += p->length;
    }
    plm_demux_destroy(d);
    *len = (int)used;
    return out;
}
