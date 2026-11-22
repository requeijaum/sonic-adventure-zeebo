#!/usr/bin/env python3
"""Acelera o pl_mpeg.h baixado (o repositório guarda só este patch).

Uso: patch_plm.py entrada.h saida.h

No ARM do Zeebo o decoder passa ~40% do tempo lendo bits: plm_buffer_read
monta o valor byte a byte e plm_buffer_read_vlc chama plm_buffer_read uma vez
por bit. Aqui:
- plm_buffer_read lê uma janela de 32 bits big-endian quando há 4 bytes à
  frente e count <= 24 (o caminho original fica para o fim do buffer);
- plm_buffer_read_vlc tira cada bit direto do byte quando há bits no buffer.
O resultado é o mesmo bit a bit (conferido no host com tools/plm_check.c).

Também acrescenta plm_video_set_skip_b(): com ele ligado os quadros B não
são decodificados (o tipo sai dos 13 bits do cabeçalho de imagem, lidos sem
mexer no buffer), o quadro anterior volta a ser entregue e o tempo avança.
Nenhum quadro seguinte usa um B como referência. Nos filmes do SA1 isso tira
50 a 68% dos quadros e deixa o decode 1,4 a 2,5 vezes mais rápido."""
import sys

READ_OLD = """int plm_buffer_read(plm_buffer_t *self, int count) {
	if (!plm_buffer_has(self, count)) {
		return 0;
	}
"""
READ_NEW = """int plm_buffer_read(plm_buffer_t *self, int count) {
	/* zeebo: janela de 32 bits quando há 4 bytes à frente */
	{
		size_t bi = self->bit_index;
		size_t by = bi >> 3;
		if (count <= 24 && by + 4 <= self->length) {
			const uint8_t *p = self->bytes + by;
			uint32_t w = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
			             ((uint32_t)p[2] << 8) | (uint32_t)p[3];
			self->bit_index = bi + count;
			return count ? (int)((w << (bi & 7)) >> (32 - count)) : 0;
		}
	}
	if (!plm_buffer_has(self, count)) {
		return 0;
	}
"""

VLC_OLD = """int16_t plm_buffer_read_vlc(plm_buffer_t *self, const plm_vlc_t *table) {
	plm_vlc_t state = {0, 0};
	do {
		state = table[state.index + plm_buffer_read(self, 1)];
	} while (state.index > 0);
	return state.value;
}"""
VLC_NEW = """int16_t plm_buffer_read_vlc(plm_buffer_t *self, const plm_vlc_t *table) {
	plm_vlc_t state = {0, 0};
	do {
		/* zeebo: o bit direto do byte enquanto houver bits no buffer */
		int bit;
		size_t bi = self->bit_index;
		if ((self->length << 3) > bi) {
			bit = (self->bytes[bi >> 3] >> (7 - (bi & 7))) & 1;
			self->bit_index = bi + 1;
		}
		else {
			bit = plm_buffer_read(self, 1);
		}
		state = table[state.index + bit];
	} while (state.index > 0);
	return state.value;
}"""


SKIP_DECL_OLD = """void plm_video_set_no_delay(plm_video_t *self, int no_delay);
"""
SKIP_DECL_NEW = SKIP_DECL_OLD + """
// zeebo: com skip ligado os quadros B nao sao decodificados: o quadro
// anterior volta a ser entregue e o tempo avanca. Nenhum quadro seguinte
// usa um B como referencia, entao a imagem nao desanda.

void plm_video_set_skip_b(plm_video_t *self, int skip);
"""

SKIP_FIELD_OLD = """\tint has_reference_frame;
\tint assume_no_b_frames;
"""
SKIP_FIELD_NEW = """\tint has_reference_frame;
\tint assume_no_b_frames;
\tint skip_b;
\tplm_frame_t *last_frame;
"""

SKIP_FUNC_OLD = """void plm_video_set_no_delay(plm_video_t *self, int no_delay) {
\tself->assume_no_b_frames = no_delay;
}
"""
SKIP_FUNC_NEW = SKIP_FUNC_OLD + """
void plm_video_set_skip_b(plm_video_t *self, int skip) {
\tself->skip_b = skip;
}
"""

DECODE_OLD = """\t\tplm_buffer_discard_read_bytes(self->buffer);
\t\t
\t\tplm_video_decode_picture(self);
"""
DECODE_NEW = """\t\t// zeebo: um B pode ser pulado sem decodificar; antes e preciso ler
\t\t// o tipo do quadro (13 bits) e devolver o ponteiro de bits
\t\tif (self->skip_b && self->last_frame) {
\t\t\t// os 13 bits saem do proprio buffer, sem ler: uma leitura aqui
\t\t\t// poderia recarregar e mover os bytes, perdendo o indice
\t\t\tsize_t saved = self->buffer->bit_index;
\t\t\tsize_t by = saved >> 3, sh = saved & 7;
\t\t\tint skip_type = -1;
\t\t\tif (saved + 13 <= (self->buffer->length << 3)) {
\t\t\t\tuint32_t w =
\t\t\t\t\t((uint32_t)self->buffer->bytes[by] << 16) |
\t\t\t\t\t((uint32_t)self->buffer->bytes[by + 1] << 8) |
\t\t\t\t\tself->buffer->bytes[by + 2];
\t\t\t\t// temporal_reference (10 bits) e picture_coding_type (3)
\t\t\t\tskip_type = (int)((w >> (11 - sh)) & 7);
\t\t\t}
\t\t\tif (skip_type == PLM_VIDEO_PICTURE_TYPE_B) {
\t\t\t\tself->start_code = plm_buffer_find_start_code(
\t\t\t\t\tself->buffer, PLM_START_PICTURE
\t\t\t\t);
\t\t\t\tframe = self->last_frame;
\t\t\t\tbreak;
\t\t\t}
\t\t}

\t\tplm_buffer_discard_read_bytes(self->buffer);
\t\t
\t\tplm_video_decode_picture(self);
"""

TAIL_OLD = """\tframe->time = self->time;
\tself->frames_decoded++;
\tself->time = (double)self->frames_decoded / self->framerate;
\t
\treturn frame;
}
"""
TAIL_NEW = """\tframe->time = self->time;
\tself->frames_decoded++;
\tself->time = (double)self->frames_decoded / self->framerate;
\tself->last_frame = frame;
\t
\treturn frame;
}
"""

REWIND_OLD = """\tself->has_reference_frame = FALSE;
\tself->start_code = -1;
}
"""
REWIND_NEW = """\tself->has_reference_frame = FALSE;
\tself->start_code = -1;
\tself->last_frame = NULL;
}
"""

PATCHES = (
    (READ_OLD, READ_NEW), (VLC_OLD, VLC_NEW),
    (REWIND_OLD, REWIND_NEW),
    (SKIP_DECL_OLD, SKIP_DECL_NEW), (SKIP_FIELD_OLD, SKIP_FIELD_NEW),
    (SKIP_FUNC_OLD, SKIP_FUNC_NEW), (DECODE_OLD, DECODE_NEW),
    (TAIL_OLD, TAIL_NEW),
)


def main():
    src, dst = sys.argv[1], sys.argv[2]
    text = open(src, encoding='utf-8').read()
    for old, new in PATCHES:
        assert text.count(old) == 1, 'pl_mpeg.h mudou: ' + old.splitlines()[0]
        text = text.replace(old, new)
    open(dst, 'w', encoding='utf-8').write(text)


if __name__ == '__main__':
    main()
