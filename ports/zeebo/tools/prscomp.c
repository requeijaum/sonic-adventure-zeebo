/*
 * prscomp: compressor PRS (Sega LZ77) para os overlays (.ovl) do port.
 * O formato e o do decodificador assets/prs.c (que o jogo ja usa), entao o
 * zeebo nao ganha codigo novo para ler o overlay.
 *
 *   prscomp c entrada saida     comprime
 *   prscomp d entrada saida     descomprime (usa assets/prs.c, teste)
 *
 * Copia curta: tamanho 2..5, distancia 1..256. Copia longa: tamanho 3..256,
 * distancia 1..8191. Casamentos por cadeia de hash de 3 bytes, guloso.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../assets/prs.h"

static unsigned char *out;
static long on, ctrl_at;
static int ctrl_bits, ctrl_val;

static void put_bit(int b)
{
    if (ctrl_bits == 0) {
        ctrl_at = on++;
        ctrl_val = 0;
    }
    ctrl_val |= (b & 1) << ctrl_bits;
    ctrl_bits++;
    out[ctrl_at] = (unsigned char)ctrl_val;
    if (ctrl_bits == 8) {
        ctrl_bits = 0;
    }
}

static void put_byte(int v)
{
    out[on++] = (unsigned char)v;
}

#define HASH_BITS 16
#define CHAIN 48

static long compress(const unsigned char *s, long n)
{
    long *head = malloc(sizeof(long) << HASH_BITS);
    long *prev = malloc(sizeof(long) * (n + 1));
    long i = 0;
    long k;

    memset(head, 0xff, sizeof(long) << HASH_BITS);
    on = 0;
    ctrl_bits = 0;
#define H(p) ((((p)[0] << 8) ^ ((p)[1] << 4) ^ (p)[2]) & ((1 << HASH_BITS) - 1))
    while (i < n) {
        long best = 0, bestd = 0;

        if (i + 2 < n) {
            long c = head[H(s + i)];
            int tries = CHAIN;

            while (c >= 0 && i - c <= 8191 && tries--) {
                long l = 0;
                long max = n - i < 256 ? n - i : 256;

                while (l < max && s[c + l] == s[i + l]) {
                    l++;
                }
                if (l > best) {
                    best = l;
                    bestd = i - c;
                }
                c = prev[c];
            }
        }
        /* copia curta de 2 bytes tambem vale (dist <= 256) */
        if (best < 3 && i + 1 < n) {
            long d;

            for (d = 1; d <= 256 && d <= i; d++) {
                if (s[i - d] == s[i] && s[i - d + 1] == s[i + 1]) {
                    best = 2;
                    bestd = d;
                    break;
                }
            }
        }
        if (best >= 2 && bestd <= 256 && best <= 5) {
            put_bit(0);
            put_bit(0);
            put_bit(((best - 2) >> 1) & 1);
            put_bit((best - 2) & 1);
            put_byte((int)(256 - bestd));
        } else if (best >= 3) {
            long v = (8192 - bestd) << 3;

            put_bit(0);
            put_bit(1);
            if (best <= 9) {
                v |= best - 2;
                put_byte((int)(v & 0xff));
                put_byte((int)(v >> 8));
            } else {
                put_byte((int)(v & 0xff));
                put_byte((int)(v >> 8));
                put_byte((int)(best - 1));
            }
        } else {
            put_bit(1);
            put_byte(s[i]);
            best = 1;
        }
        for (k = 0; k < best; k++) {
            if (i + 2 < n) {
                long h = H(s + i);

                prev[i] = head[h];
                head[h] = i;
            }
            i++;
        }
    }
    put_bit(0);
    put_bit(1);
    put_byte(0);
    put_byte(0);
    free(head);
    free(prev);
    return on;
}

static unsigned char *slurp(const char *path, long *n)
{
    FILE *f = fopen(path, "rb");
    unsigned char *b;

    if (!f) {
        perror(path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    *n = ftell(f);
    fseek(f, 0, SEEK_SET);
    b = malloc(*n + 1);
    if (fread(b, 1, *n, f) != (size_t)*n) {
        exit(1);
    }
    fclose(f);
    return b;
}

int main(int argc, char **argv)
{
    long n, m;
    unsigned char *s;
    FILE *f;

    if (argc != 4) {
        fprintf(stderr, "uso: prscomp c|d entrada saida\n");
        return 2;
    }
    s = slurp(argv[2], &n);
    if (argv[1][0] == 'c') {
        out = malloc(n + n / 8 + 64);
        m = compress(s, n);
        f = fopen(argv[3], "wb");
        fwrite(out, 1, m, f);
        fclose(f);
        printf("%ld -> %ld (%.1f%%)\n", n, m, n ? 100.0 * m / n : 0.0);
    } else {
        int sz = prs_decompressed_size(s, (int)n);
        unsigned char *d = malloc(sz > 0 ? sz : 1);
        int r = prs_unpack(s, (int)n, d, sz);

        if (r < 0) {
            fprintf(stderr, "PRS invalido\n");
            return 1;
        }
        f = fopen(argv[3], "wb");
        fwrite(d, 1, r, f);
        fclose(f);
    }
    return 0;
}
