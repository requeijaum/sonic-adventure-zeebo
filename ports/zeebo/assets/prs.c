/*
 * sa1-zeebo — descompressao PRS (Sega LZ77), freestanding.
 *
 * Port fiel do algoritmo de referencia (Sylverant prs-decomp.c / Sega PRS):
 * fluxo de bits LSB-first; flag 1 = literal; flag 0 = copia:
 *   - bit seguinte 1: copia longa. short LE; 0 => fim de stream.
 *     size = short & 7; offset = (short >> 3) - 8192.
 *     se size == 0, size = byte + 1; senao size += 2.
 *   - bit seguinte 0: copia curta. primeiro bit = bit alto do tamanho;
 *     size = (primeiro << 1 | segundo) + 2; offset = byte - 256.
 *
 * Copia sempre de tras para frente: dst[out] = dst[out + offset].
 * Sem libc: o chamador fornece o destino; erros retornam negativo.
 */

#include "prs.h"

typedef struct {
    const unsigned char *src;
    int len;
    int pos;
    int flags;
    int bits;
} PrsBits;

static int prs_bit(PrsBits *b)
{
    if (b->bits == 0) {
        if (b->pos >= b->len)
            return -1;
        b->flags = b->src[b->pos++];
        b->bits = 8;
    }
    {
        int r = b->flags & 1;
        b->flags >>= 1;
        b->bits--;
        return r;
    }
}

static int prs_byte(PrsBits *b)
{
    if (b->pos >= b->len)
        return -1;
    return b->src[b->pos++];
}

static int prs_short(PrsBits *b)
{
    int v;
    if (b->pos + 1 >= b->len)
        return -1;
    v = b->src[b->pos] | (b->src[b->pos + 1] << 8);
    b->pos += 2;
    return v;
}

/* counting != 0: apenas mede; dst pode ser NULL. */
static int prs_run(const unsigned char *src, int src_len,
                   unsigned char *dst, int dst_cap, int counting)
{
    PrsBits b;
    int out = 0;

    b.src = src;
    b.len = src_len;
    b.pos = 0;
    b.flags = 0;
    b.bits = 0;

    for (;;) {
        int flag = prs_bit(&b);
        if (flag < 0)
            return -1;

        if (flag) {
            int ch = prs_byte(&b);
            if (ch < 0)
                return -1;
            if (!counting) {
                if (out >= dst_cap)
                    return -1;
                dst[out] = (unsigned char)ch;
            }
            out++;
            continue;
        }

        flag = prs_bit(&b);
        if (flag < 0)
            return -1;

        {
            int size;
            int offset;

            if (flag) {
                int sh = prs_short(&b);
                if (sh < 0)
                    return -1;
                if (sh == 0)
                    return out; /* fim de stream */
                size = sh & 7;
                offset = (sh >> 3) - 0x2000;
                if (size == 0) {
                    int s = prs_byte(&b);
                    if (s < 0)
                        return -1;
                    size = s + 1;
                } else {
                    size += 2;
                }
            } else {
                int a = prs_bit(&b);
                int c = prs_bit(&b);
                int o;
                if (a < 0 || c < 0)
                    return -1;
                size = ((a << 1) | c) + 2;
                o = prs_byte(&b);
                if (o < 0)
                    return -1;
                offset = o - 256;
            }

            while (size-- > 0) {
                int idx = out + offset;
                if (idx < 0)
                    return -1;
                if (!counting) {
                    if (out >= dst_cap)
                        return -1;
                    dst[out] = dst[idx];
                }
                out++;
            }
        }
    }
}

int prs_decompressed_size(const unsigned char *src, int src_len)
{
    if (!src || src_len < 3)
        return -1;
    return prs_run(src, src_len, 0, 0, 1);
}

int prs_unpack(const unsigned char *src, int src_len,
                   unsigned char *dst, int dst_cap)
{
    if (!src || !dst || src_len < 3 || dst_cap < 0)
        return -1;
    return prs_run(src, src_len, dst, dst_cap, 0);
}

/* a chamada do jogo (src/system/prs.h): prs_decompress(de, para), sem tamanhos,
 * como a rotina em asm do 1ST_READ. Devolve o tamanho descomprimido */
int prs_decompress(const void *from, void *to)
{
    return prs_unpack((const unsigned char *)from, 0x7fffffff,
                      (unsigned char *)to, 0x7fffffff);
}
