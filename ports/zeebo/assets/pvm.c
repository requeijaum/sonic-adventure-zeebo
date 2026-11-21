/* sa1-zeebo — parser de containers PVM (PVMH), freestanding.
 *
 * Layout confirmado no PVM real do EGM (extract/prs/EGM1EGGMAN.prs):
 *   0x00 "PVMH"
 *   0x04 u32 header_size (0x1F8); os chunks comecam em 8 + header_size (0x200)
 *   0x08 u16 flags (0x0109)
 *   0x0A u16 count (14)
 *   ...
 *   chunk i: "PVRT" + u32 size + u32 fmt + u16 w + u16 h + payload
 *            proximo = offset + 8 + size  (size conta fmt+w+h+payload)
 *   o ultimo termina exatamente no fim do buffer.
 *
 * Nada aqui e freestanding-host; sem libc.
 */
#include "pvm.h"

static unsigned int pvm_rd16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static unsigned int pvm_rd32(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

int pvm_parse(const unsigned char *buf, int len, Pvm *out)
{
    unsigned int header_size, chunk_off;
    int i, count;

    if (!out)
        return PVM_ERR_ARG;
    out->buf = 0;
    out->len = 0;
    out->flags = 0;
    out->count = 0;
    if (!buf || len < 12)
        return PVM_ERR_ARG;

    if (buf[0] != 'P' || buf[1] != 'V' || buf[2] != 'M' || buf[3] != 'H')
        return PVM_ERR_MAGIC;

    /* header_size conta os bytes apos os 8 de (magic + tamanho); os dados
     * dos chunks comecam em 8 + header_size. Menos de 4 nao cobriria sequer
     * flags + count. */
    header_size = pvm_rd32(buf + 4);
    if (header_size < 4u || header_size > (unsigned int)len - 8u)
        return PVM_ERR_HEADER;
    chunk_off = 8u + header_size;

    count = (int)pvm_rd16(buf + 10);
    if (count <= 0 || count > PVM_MAX_TEXTURES)
        return PVM_ERR_COUNT;

    for (i = 0; i < count; i++) {
        PvmTexture *t = &out->tex[i];
        unsigned int size, fmt, w, h;

        if (chunk_off > (unsigned int)len ||
            (unsigned int)len - chunk_off < 16u)
            return PVM_ERR_CHUNK;
        if (buf[chunk_off] != 'P' || buf[chunk_off + 1] != 'V' ||
            buf[chunk_off + 2] != 'R' || buf[chunk_off + 3] != 'T')
            return PVM_ERR_PVRT;

        size = pvm_rd32(buf + chunk_off + 4);
        fmt = pvm_rd32(buf + chunk_off + 8);
        w = pvm_rd16(buf + chunk_off + 12);
        h = pvm_rd16(buf + chunk_off + 14);

        /* size cobre fmt + w + h + payload; os 8 primeiros bytes (PVRT +
         * size) ficam fora. Exige pelo menos o cabecalho de 8 bytes. */
        if (size < 8u)
            return PVM_ERR_CHUNK;
        if (size > (unsigned int)len - chunk_off - 8u)
            return PVM_ERR_CHUNK;
        if (w < 1u || w > 4096u || h < 1u || h > 4096u)
            return PVM_ERR_CHUNK;

        t->pvr.pixel_format = fmt & 0xffu;
        t->pvr.data_format = (fmt >> 8) & 0xffu;
        t->pvr.width = w;
        t->pvr.height = h;
        t->pvr.data_offset = chunk_off + 16u;
        t->pvr.size = size;
        t->data = buf + chunk_off + 16u;
        t->data_len = (int)(size - 8u);
        t->chunk_offset = chunk_off;

        chunk_off = chunk_off + 8u + size;
    }

    out->buf = buf;
    out->len = len;
    out->flags = pvm_rd16(buf + 8);
    out->count = count;
    return count;
}

/* Zera `n` bytes sem libc (o host tem memset builtin; o alvo freestanding
 * usa string.c, mas manter local evita dependencia de header). */
static void pvm_zero(unsigned char *p, int n)
{
    while (n-- > 0)
        *p++ = 0;
}

/* Invalida os `count` offsets contados. Usado em toda falha apos o count ser
 * validado: o chamador nunca deve enxergar uma saida parcial como valida. */
static void pvm_clear_offsets(const Pvm *pvm, int *offsets)
{
    int i;
    if (offsets)
        for (i = 0; i < pvm->count; i++)
            offsets[i] = -1;
}

int pvm_decode_all(const Pvm *pvm, unsigned char *dst, int dst_cap,
                   int *offsets)
{
    int i, total = 0;

    /* Sem container valido nao ha count confiavel: nao da para invalidar os
     * offsets sem saber quantos sao, entao `offsets` fica intacto. */
    if (!pvm || pvm->count <= 0 || pvm->count > PVM_MAX_TEXTURES)
        return PVM_ERR_ARG;

    if (!dst || dst_cap <= 0) {
        pvm_clear_offsets(pvm, offsets);
        return PVM_ERR_ARG;
    }

    /* Preflight: valida dimensoes e capacidade acumulada sem escrever nada.
     * Assim uma falha tardia de capacidade nao deixa imagens parciais. */
    for (i = 0; i < pvm->count; i++) {
        const PvmTexture *t = &pvm->tex[i];
        unsigned int w = t->pvr.width, h = t->pvr.height;

        if (w == 0 || w > 4096u || h == 0 || h > 4096u) {
            pvm_clear_offsets(pvm, offsets);
            return PVM_ERR_DECODE;
        }
        /* w*h <= 4096*4096 e *4 cabe em int; o total tambem porque so cresce
         * apos provar que cabe no restante de dst_cap. */
        if (w * h > (unsigned int)(dst_cap - total) / 4u) {
            pvm_clear_offsets(pvm, offsets);
            return PVM_ERR_SPACE;
        }
        total += (int)(w * h * 4u);
    }

    total = 0;
    for (i = 0; i < pvm->count; i++) {
        const PvmTexture *t = &pvm->tex[i];
        unsigned int w = t->pvr.width, h = t->pvr.height;
        int bytes = (int)(w * h * 4u);
        int n;

        n = pvr_decode_rgba8888(&t->pvr, t->data, t->data_len,
                                dst + total, dst_cap - total);
        if (n != bytes) {
            /* Apaga so o prefixo tentado (imagens ja decodificadas mais a que
             * falhou), nunca alem de dst_cap. */
            int clear = total + bytes;
            if (clear > dst_cap)
                clear = dst_cap;
            pvm_zero(dst, clear);
            pvm_clear_offsets(pvm, offsets);
            return PVM_ERR_DECODE;
        }
        if (offsets)
            offsets[i] = total;
        total += n;
    }

    return total;
}
