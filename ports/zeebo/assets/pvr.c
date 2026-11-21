/* sa1-zeebo — parser/decoder de texturas PVM/PVRT (Sega/Dreamcast).
 *
 * O campo fmt do PVRT: byte baixo = pixel format; byte seguinte = "image data
 * type" (sega-pvr): 0x01 square twiddled, 0x02 twiddled+mipmap, 0x03 VQ,
 * 0x04 VQ+mipmap, 0x05 8bpp CLUT, 0x06 4bpp CLUT, 0x07 8bpp direct,
 * 0x08 4bpp direct, 0x09 rectangle, 0x0D rectangle twiddled, 0x10 small VQ,
 * 0x11 small VQ+mipmap, 0x12 square twiddled+mipmap.
 *
 * Distribuicao medida nos 441 PVR do jogo: 0x09 rectangle (391), 0x02 (21),
 * 0x01 (20), 0x11 (4), 0x04 (3), 0x03 (2).
 */
#include "pvr.h"

static unsigned int rd16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static unsigned int rd32(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

int pvr_scan(const unsigned char *buf, int len, PvrTexture *out, int max)
{
    int count = 0;
    int i = 0;

    if (!buf || len < 16 || max < 0 || (out && max == 0))
        return 0;

    while (i <= len - 16) {
        if (buf[i] == 'P' && buf[i + 1] == 'V' && buf[i + 2] == 'R' &&
            buf[i + 3] == 'T') {
            unsigned int size = rd32(buf + i + 4);
            unsigned int fmt = rd32(buf + i + 8);
            unsigned int w = rd16(buf + i + 12);
            unsigned int h = rd16(buf + i + 14);

            if (size >= 8u && size <= (unsigned int)(len - i - 8) &&
                w >= 1 && w <= 4096 && h >= 1 && h <= 4096) {
                if (out && count < max) {
                    out[count].pixel_format = fmt & 0xff;
                    out[count].data_format = (fmt >> 8) & 0xff;
                    out[count].width = w;
                    out[count].height = h;
                    out[count].data_offset = (unsigned int)(i + 16);
                    out[count].size = size;
                }
                count++;
                i += (int)size + 8;
                continue;
            }
        }
        i++;
    }
    return count;
}

unsigned int pvr_untwiddle_index(unsigned int w, unsigned int h, unsigned int p)
{
    unsigned int ddx = 1, ddy = w, q = 0;
    int i;

    for (i = 0; i < 16; i++) {
        h >>= 1;
        if (h) {
            if (p & 1)
                q |= ddy;
            p >>= 1;
        }
        ddy <<= 1;
        w >>= 1;
        if (w) {
            if (p & 1)
                q |= ddx;
            p >>= 1;
        }
        ddx <<= 1;
    }
    return q;
}

static int pvr_expand(unsigned int pixel_format, unsigned int v,
                      unsigned char *out)
{
    unsigned int r, g, b, a = 255;
    switch (pixel_format) {
    case 0: /* ARGB1555 */
        a = (v >> 15) & 1 ? 255 : 0;
        r = (v >> 10) & 0x1f;
        g = (v >> 5) & 0x1f;
        b = v & 0x1f;
        out[0] = (unsigned char)((r << 3) | (r >> 2));
        out[1] = (unsigned char)((g << 3) | (g >> 2));
        out[2] = (unsigned char)((b << 3) | (b >> 2));
        out[3] = (unsigned char)a;
        return 0;
    case 1: /* RGB565 */
        r = (v >> 11) & 0x1f;
        g = (v >> 5) & 0x3f;
        b = v & 0x1f;
        out[0] = (unsigned char)((r << 3) | (r >> 2));
        out[1] = (unsigned char)((g << 2) | (g >> 4));
        out[2] = (unsigned char)((b << 3) | (b >> 2));
        out[3] = 255;
        return 0;
    case 2: /* ARGB4444 */
        a = (v >> 12) & 0x0f;
        r = (v >> 8) & 0x0f;
        g = (v >> 4) & 0x0f;
        b = v & 0x0f;
        out[0] = (unsigned char)((r << 4) | r);
        out[1] = (unsigned char)((g << 4) | g);
        out[2] = (unsigned char)((b << 4) | b);
        out[3] = (unsigned char)((a << 4) | a);
        return 0;
    default:
        return -1;
    }
}

static unsigned char pvr_clamp8(int v)
{
    return v < 0 ? 0 : (v > 255 ? 255 : (unsigned char)v);
}

/* Offset do nivel-base de texturas twiddled+mipmap. Os mips ficam do menor
 * para o maior; para 0x02 a tabela conta texels de 16 bits e exclui os dois
 * ultimos; 0x12 e o layout alternativo sem esse corte. */
static int pvr_mip_base_offset(unsigned int type, unsigned int w,
                               unsigned int h, int *offset)
{
    static const unsigned int other_mip_point[11] = {
        3, 4, 8, 24, 88, 344, 1368, 5464, 21848, 87384, 349528
    };
    unsigned int level = 0;

    if (!offset || w != h || w < 8 || (w & (w - 1)) != 0)
        return -1;
    while ((1u << level) < w && level < 10)
        level++;
    if ((1u << level) != w)
        return -1;

    if (type == 0x02 || type == 0x12) {
        *offset = (int)((other_mip_point[level] -
                         (type == 0x12 ? 0 : 2)) * 2);
        return 0;
    }
    return -1;
}

/* Numero de entradas do codebook VQ (8 bytes cada): 256 no VQ normal; no
 * small VQ, 16/32/128/256 conforme a largura (0x10) ou 16/64/256 (0x11). */
static int pvr_vq_codebook_entries(unsigned int type, unsigned int w)
{
    if (type == 0x03 || type == 0x04)
        return 256;
    if (type == 0x10) {
        if (w <= 16)
            return 16;
        if (w == 32)
            return 32;
        if (w == 64)
            return 128;
        return 256;
    }
    if (type == 0x11) {
        if (w <= 16)
            return 16;
        if (w == 32)
            return 64;
        return 256;
    }
    return -1;
}

/* Bytes da cadeia de mips (niveis menores) antes do nivel-base, apos o
 * codebook. Independente do tamanho do codebook. */
static int pvr_vq_mip_bytes(unsigned int w, int *out)
{
    static const unsigned int vq_mip_point[11] = {
        2048, 2049, 2050, 2054, 2070, 2134, 2390, 3414, 7510, 23894, 89430
    };
    unsigned int level = 0;

    if (!out || w < 8 || (w & (w - 1)) != 0)
        return -1;
    while ((1u << level) < w && level < 10)
        level++;
    if ((1u << level) != w)
        return -1;
    *out = (int)(vq_mip_point[level] - 2048);
    return 0;
}

/* YUV422 (y u v) -> RGBA8888, coeficientes BT.601 usados pelo PVR. */
static void pvr_yuv_to_rgba(unsigned int y, unsigned int u, unsigned int v,
                            unsigned char *out)
{
    int u2 = (int)u - 128;
    int v2 = (int)v - 128;
    int r = (int)y + (11 * v2) / 8;
    int g = (int)y - (11 * u2) / 32 - (11 * v2) / 16;
    int b = (int)y + (55 * u2) / 32;
    out[0] = pvr_clamp8(r);
    out[1] = pvr_clamp8(g);
    out[2] = pvr_clamp8(b);
    out[3] = 255;
}

int pvr_decode_rgba8888(const PvrTexture *t, const unsigned char *data,
                        int data_len, unsigned char *dst, int dst_cap)
{
    unsigned int w, h, n, p;
    unsigned int type;
    int twiddled, rect, vq;
    int mip_base = 0;

    if (!t || !data || !dst || data_len <= 0 || dst_cap <= 0)
        return -1;
    if (t->pixel_format > 3)
        return -1; /* bumpmap nao suportado */

    w = t->width;
    h = t->height;
    /* The supported dimension bound makes all texel/byte products fit int. */
    if (w == 0 || w > 4096u || h == 0 || h > 4096u)
        return -1;
    n = w * h;
    if (n > (unsigned int)dst_cap / 4u)
        return -1;

    type = t->data_format;
    twiddled = (type == 0x01 || type == 0x02 || type == 0x12);
    rect = (type == 0x09);
    vq = (type == 0x03 || type == 0x04 || type == 0x10 || type == 0x11);

    if ((twiddled || vq) && ((w & (w - 1)) || (h & (h - 1))))
        return -1;
    if (vq && (w < 2 || h < 2))
        return -1;
    if ((type == 0x04 || type == 0x11) && w != h)
        return -1;

    if ((type == 0x02 || type == 0x12) &&
        pvr_mip_base_offset(type, w, h, &mip_base) != 0)
        return -1;

    if (t->pixel_format == 3) { /* YUV422 */
        if (!rect || (w & 1) || data_len < (int)n * 2)
            return -1;
        for (p = 0; p < n; p += 2) {
            unsigned int u = data[p * 2];
            unsigned int y0 = data[p * 2 + 1];
            unsigned int v = data[p * 2 + 2];
            unsigned int y1 = data[p * 2 + 3];
            pvr_yuv_to_rgba(y0, u, v, dst + (p * 4));
            pvr_yuv_to_rgba(y1, u, v, dst + ((p + 1) * 4));
        }
        return (int)(n * 4);
    }

    if (vq) {
        /* VQ: codebook (entradas de 8 bytes) + 1 byte/bloco (w/2 x h/2).
         * Com mipmap, a cadeia de mips fica entre o codebook e o nivel-base. */
        unsigned int bw = w / 2, bh = h / 2, blocks = bw * bh;
        int entries = pvr_vq_codebook_entries(type, w);
        int indices_offset;
        if (entries < 0)
            return -1;
        indices_offset = entries * 8;
        if (type == 0x04 || type == 0x11) {
            int mip_bytes;
            if (pvr_vq_mip_bytes(w, &mip_bytes) != 0)
                return -1;
            indices_offset += mip_bytes;
        }
        if (data_len < indices_offset ||
            blocks > (unsigned int)(data_len - indices_offset))
            return -1;
        /* Validate the entire index plane before dereferencing any codeword. */
        for (p = 0; p < blocks; p++)
            if (data[indices_offset + p] >= (unsigned int)entries)
                return -1;
        for (p = 0; p < blocks; p++) {
            unsigned int idx = data[indices_offset + p];
            const unsigned char *cb = data + idx * 8;
            unsigned int t0 = (unsigned int)cb[0] | ((unsigned int)cb[1] << 8);
            unsigned int t1 = (unsigned int)cb[2] | ((unsigned int)cb[3] << 8);
            unsigned int t2 = (unsigned int)cb[4] | ((unsigned int)cb[5] << 8);
            unsigned int t3 = (unsigned int)cb[6] | ((unsigned int)cb[7] << 8);
            unsigned int lin = pvr_untwiddle_index(bw, bh, p);
            unsigned int base = (lin % bw) * 2 + ((lin / bw) * 2) * w;
            if (pvr_expand(t->pixel_format, t0, dst + (base * 4)) != 0)
                return -1;
            if (pvr_expand(t->pixel_format, t2, dst + ((base + 1) * 4)) != 0)
                return -1;
            if (pvr_expand(t->pixel_format, t1, dst + ((base + w) * 4)) != 0)
                return -1;
            if (pvr_expand(t->pixel_format, t3,
                           dst + ((base + w + 1) * 4)) != 0)
                return -1;
        }
        return (int)(n * 4);
    }

    if (!rect && !twiddled)
        return -1; /* paleta / rectangle twiddled ainda nao suportados */

    if (data_len < mip_base || (int)n * 2 > data_len - mip_base)
        return -1;

    for (p = 0; p < n; p++) {
        unsigned int byte_offset = (unsigned int)mip_base + p * 2;
        unsigned int v = (unsigned int)data[byte_offset] |
                         ((unsigned int)data[byte_offset + 1] << 8);
        unsigned int lin = rect ? p : pvr_untwiddle_index(w, h, p);
        if (pvr_expand(t->pixel_format, v, dst + (lin * 4)) != 0)
            return -1;
    }
    return (int)(n * 4);
}
