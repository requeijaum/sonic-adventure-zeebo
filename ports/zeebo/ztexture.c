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
