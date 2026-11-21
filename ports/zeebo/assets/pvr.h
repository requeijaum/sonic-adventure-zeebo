/*
 * sa1-zeebo — parser de containers de textura PVM/PVRT (Sega/Dreamcast).
 * Estrutura PVRT e decoder para formatos suportados (twiddled/rectangle/VQ).
 */
#ifndef SA1_PVR_H
#define SA1_PVR_H

typedef struct {
    unsigned int pixel_format; /* byte baixo do campo fmt (0=ARGB1555,1=RGB565,2=ARGB4444,3=YUV422) */
    unsigned int data_format;  /* byte seguinte: image data type Sega (0x01..0x12) */
    unsigned int width;
    unsigned int height;
    unsigned int data_offset;  /* offset do pixel data dentro do buffer */
    unsigned int size;         /* campo size do PVRT */
} PvrTexture;

/* Varre o buffer por blocos PVRT. Retorna a contagem (>=0), preenchendo
 * ate `max` entradas. Exige chunks completos (size >= 8), pulando seu payload.
 * Se `out` for NULL, so conta; max negativo ou out com max <= 0 retorna 0. */
int pvr_scan(const unsigned char *buf, int len, PvrTexture *out, int max);

/* Indice linear (y*w+x) do texel na posicao de memoria `p` de uma textura
 * twiddled w x h. Port de get_untwiddled_index (Crayon-Utilities). */
unsigned int pvr_untwiddle_index(unsigned int w, unsigned int h, unsigned int p);

/* Decodifica para RGBA8888.
 * Formatos suportados: data type 0x01/0x02/0x12 twiddled, 0x03/0x04 VQ,
 * 0x10/0x11 small VQ, 0x09 rectangle; pixel_format 0=ARGB1555,1=RGB565,
 * 2=ARGB4444,3=YUV422 (somente rectangle).
 * Dimensoes: 1..4096; twiddle/VQ exigem potencias de dois; VQ >= 2x2.
 * Mipmapped types retornam apenas o nível-base; VQ mipmapped exige quadrado.
 * Nao suportado: paleta 0x05/0x06/0x07/0x08, rectangle twiddled 0x0D,
 * YUV422 twiddled, stride 0x0B.
 * Retorna w*h*4 (bytes escritos), ou <0 se formato/dimensoes/indices invalidos
 * ou buffers pequenos. data_len e dst_cap devem ser positivos. */
int pvr_decode_rgba8888(const PvrTexture *t, const unsigned char *data,
                        int data_len, unsigned char *dst, int dst_cap);

#endif /* SA1_PVR_H */
