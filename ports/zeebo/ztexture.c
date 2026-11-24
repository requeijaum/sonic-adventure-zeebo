/* ztexture.c — as texturas da Kamui que o Ninja do jogo cria, no GLES1.
 *
 * O Ninja (sdk/ninja/njTexLoad.c, njTexinner.c) descreve cada textura numa
 * KMSURFACEDESC, pede a superfície com kmCreateTextureSurface (tamanho e o
 * tipo do PVR: formato de cor no byte baixo, organização no seguinte) e manda
 * os pixels com kmLoadTexture. Aqui a superfície guarda o índice da textura
 * GL (zstrip) em pSurface, mais um (0 é "sem textura"). njkm_zeebo.c lê esse
 * índice quando o jogo seleciona a textura (njSetkmTextureNumG).
 *
 * Decodifica com assets/pvr.c (twiddled, retângulo, VQ; paleta não). */
#include <AEEStdLib.h>

#include "assets/pvr.h"
#include "zstrip.h"

void zlog(const char *what, const char *name, int value);

/* o layout de KMSURFACEDESC (sdk/include/kamui.h) e de NJS_TEXSURFACE */
struct surface {
    uint32 type;
    uint32 bit_depth;
    uint32 pixel_format;
    uint32 width;
    uint32 height;
    uint32 size;
    uint32 flags;
    uint32 surface; /* índice GL + 1 */
    uint32 virt;
    uint32 phys;
};

/* tipo do PVR: formato de cor e organização (KM_TEXTURE_*) */
int kmCreateTextureSurface(struct surface *s, int width, int height,
                           unsigned int type)
{
    if (s == 0) {
        return -1;
    }
    s->type = type;
    s->pixel_format = type & 0xffu;
    s->width = (uint32)width;
    s->height = (uint32)height;
    /* TextureSize: o Ninja le daqui para decidir se manda os pixels (e em que
     * tamanho). cota superior de qualquer formato nao paletizado (mipmap
     * incluso); o total real vem do arquivo */
    s->size = (uint32)width * (uint32)height * 4u + 4096u;
    s->surface = 0;
    return 0;
}

/* O RGB dos texels transparentes das texturas de recorte (as folhas das
 * palmeiras) é lixo colorido (azul). O PVR não deixa isso vazar, mas o GL
 * filtra RGBA sem pré-multiplicar e a borda das folhas ficava ciano. Aqui
 * os texels quase transparentes herdam a cor dos vizinhos opacos (até 3
 * passos), sem mexer no alfa. */
static void alpha_bleed(unsigned char *p, int w, int h)
{
    unsigned char *ok, *nx;
    int i, x, y, pass, any_t = 0, any_o = 0;

    for (i = 0; i < w * h; i++) {
        if (p[i * 4 + 3] < 128) {
            any_t = 1;
        } else {
            any_o = 1;
        }
    }
    if (!any_t || !any_o) {
        return;
    }
    ok = (unsigned char *)MALLOC((uint32)(w * h * 2));
    if (ok == 0) {
        return;
    }
    nx = ok + w * h;
    for (i = 0; i < w * h; i++) {
        ok[i] = p[i * 4 + 3] >= 128;
    }
    for (pass = 0; pass < 3; pass++) {
        for (i = 0; i < w * h; i++) {
            nx[i] = ok[i];
        }
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                int dx, dy, n = 0, r = 0, g = 0, b = 0;

                if (ok[y * w + x]) {
                    continue;
                }
                for (dy = -1; dy <= 1; dy++) {
                    for (dx = -1; dx <= 1; dx++) {
                        int xx = x + dx, yy = y + dy;

                        if (xx < 0 || yy < 0 || xx >= w || yy >= h ||
                            !ok[yy * w + xx]) {
                            continue;
                        }
                        r += p[(yy * w + xx) * 4];
                        g += p[(yy * w + xx) * 4 + 1];
                        b += p[(yy * w + xx) * 4 + 2];
                        n++;
                    }
                }
                if (n > 0) {
                    p[(y * w + x) * 4] = (unsigned char)(r / n);
                    p[(y * w + x) * 4 + 1] = (unsigned char)(g / n);
                    p[(y * w + x) * 4 + 2] = (unsigned char)(b / n);
                    nx[y * w + x] = 1;
                }
            }
        }
        for (i = 0; i < w * h; i++) {
            ok[i] = nx[i];
        }
    }
    FREE(ok);
}

/* decodifica len bytes de pixels e sobe a textura ao GL. quiet: falha sem log
 * (kmLoadTexturePart tenta a cada pedaco e so o ultimo completa) */
static int surface_upload(struct surface *s, const unsigned char *data,
                          int len, int quiet)
{
    PvrTexture t;
    unsigned char *rgba;
    int bytes;
    int index;
    uint32 t_start = GETUPTIMEMS();
    int reload;

    if (s == 0 || data == 0 || s->width == 0 || s->height == 0) {
        return -1;
    }
    reload = s->surface != 0;
    t.pixel_format = s->type & 0xffu;
    t.data_format = (s->type >> 8) & 0xffu;
    t.width = s->width;
    t.height = s->height;
    t.data_offset = 0;
    t.size = 0;
    bytes = (int)(s->width * s->height * 4u);
    rgba = (unsigned char *)MALLOC((uint32)bytes);
    if (rgba == 0) {
        return -1;
    }
    if (pvr_decode_rgba8888(&t, data, len, rgba, bytes) <= 0) {
        if (!quiet) {
            zlog("tex", "formato sem decodificador", (int)s->type);
            zinst_tex_upload(0, s->type, (int)s->width, (int)s->height, -1,
                             (unsigned)(GETUPTIMEMS() - t_start), reload);
        }
        FREE(rgba);
        return -1;
    }
    alpha_bleed(rgba, (int)s->width, (int)s->height);
    index = zstrip_tex_new((int)s->width, (int)s->height, rgba);
    FREE(rgba);
    if (index < 0) {
        zinst_tex_upload(0, s->type, (int)s->width, (int)s->height, -1,
                         (unsigned)(GETUPTIMEMS() - t_start), reload);
        return -1;
    }
    s->surface = (uint32)index + 1u;
    zinst_tex_upload(1, s->type, (int)s->width, (int)s->height, index,
                     (unsigned)(GETUPTIMEMS() - t_start), reload);
    return 0;
}

int kmLoadTexture(struct surface *s, const unsigned char *data, int dma,
                  int wait)
{
    (void)dma;
    (void)wait;
    /* o Ninja não diz o tamanho dos dados; o decodificador para no que o
     * formato pede */
    return surface_upload(s, data, 0x7fffffff, 0);
}

/* kmLoadTexturePart: o Ninja (njLoadTexMemFromFile) manda a textura de um
 * ARQUIVO .PVR, como o letreiro da fase (s_stage01.PVR), em pedacos: o
 * deslocamento e o tamanho de cada um. Era um STUB e o TextureSize da
 * superficie ficava 0 (kmCreateTextureSurface nao o preenchia), entao o Ninja
 * nem chamava: nada subia ao GL e o jogo desenhava o quadro branco da
 * textura de erro. Os pedacos se juntam num buffer que cresce; a cada pedaco
 * tenta decodificar com o que ja chegou, e o decodificador recusa enquanto
 * faltar dado (o total so se sabe no ultimo). */
static unsigned char *part_buf;
static unsigned int part_len;
static const struct surface *part_for;

int kmLoadTexturePart(struct surface *s, const unsigned char *data,
                      unsigned int offset, unsigned int size)
{
    unsigned char *grown;
    unsigned int end;

    if (s == 0 || data == 0 || size == 0) {
        return -1;
    }
    zinst_tex_part();
    if (offset == 0 || part_for != s) {
        if (part_buf != 0) {
            FREE(part_buf);
        }
        part_buf = 0;
        part_len = 0;
        part_for = s;
    }
    if (offset > part_len) {
        return -1; /* pedaco fora de ordem */
    }
    end = offset + size;
    grown = (unsigned char *)MALLOC(end);
    if (grown == 0) {
        zlog("tex", "kmLoadTexturePart sem memoria", (int)end);
        return -1;
    }
    if (part_buf != 0) {
        MEMCPY(grown, part_buf, part_len < end ? part_len : end);
        FREE(part_buf);
    }
    MEMCPY(grown + offset, data, size);
    part_buf = grown;
    part_len = end;
    if (surface_upload(s, part_buf, (int)part_len, 1) == 0) {
        FREE(part_buf);
        part_buf = 0;
        part_len = 0;
        part_for = 0;
    }
    return 0;
}

int kmFreeTexture(struct surface *s)
{
    if (s != 0) {
        if (s->surface != 0) {
            zinst_tex_free((int)s->surface - 1);
            zstrip_tex_free((int)s->surface - 1);
        }
        s->surface = 0;
    }
    return 0;
}

/* o índice GL da superfície, ou -1 */
int ztexture_index(const void *surface)
{
    const struct surface *s = (const struct surface *)surface;

    return (s != 0 && s->surface != 0) ? (int)s->surface - 1 : -1;
}
