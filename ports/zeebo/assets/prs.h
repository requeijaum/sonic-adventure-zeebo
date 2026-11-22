/*
 * sa1-zeebo — descompressao PRS (Sega LZ77), freestanding.
 *
 * PRS e o formato de compressao usado pelos assets do Sonic Adventure
 * (PRS/LZ77, LSB-first). Sem libc: sem malloc, sem stdlib; o chamador
 * fornece o destino. Erros retornam negativo.
 */
#ifndef SA1_PRS_H
#define SA1_PRS_H

/* Tamanho descomprimido de um buffer PRS, ou negativo em erro. */
int prs_decompressed_size(const unsigned char *src, int src_len);

/* Descomprime src em dst (capacidade dst_cap).
 * Retorna o tamanho descomprimido (>=0) ou negativo em erro
 * (dados invalidos / destino pequeno). */
int prs_unpack(const unsigned char *src, int src_len,
                   unsigned char *dst, int dst_cap);

#endif /* SA1_PRS_H */
