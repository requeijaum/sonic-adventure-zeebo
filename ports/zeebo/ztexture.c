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

int kmLoadTexture(struct surface *s, const unsigned char *data, int dma,
                  int wait)
{
    PvrTexture t;
    unsigned char *rgba;
    int bytes;
    int index;

    (void)dma;
    (void)wait;
    if (s == 0 || data == 0 || s->width == 0 || s->height == 0) {
        return -1;
    }
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
    /* o Ninja não diz o tamanho dos dados; o decodificador para no que o
     * formato pede */
    if (pvr_decode_rgba8888(&t, data, 0x7fffffff, rgba, bytes) <= 0) {
        zlog("tex", "formato sem decodificador", (int)s->type);
        FREE(rgba);
        return -1;
    }
    alpha_bleed(rgba, (int)s->width, (int)s->height);
    index = zstrip_tex_new((int)s->width, (int)s->height, rgba);
    FREE(rgba);
    if (index < 0) {
        return -1;
    }
    s->surface = (uint32)index + 1u;
    return 0;
}

int kmFreeTexture(struct surface *s)
{
    if (s != 0) {
        if (s->surface != 0) {
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
