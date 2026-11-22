#!/usr/bin/env python3
"""Acelera o pl_mpeg.h baixado (o repositório guarda só este patch).

Uso: patch_plm.py entrada.h saida.h

No ARM do Zeebo o decoder passa ~40% do tempo lendo bits: plm_buffer_read
monta o valor byte a byte e plm_buffer_read_vlc chama plm_buffer_read uma vez
por bit. Aqui:
- plm_buffer_read lê uma janela de 32 bits big-endian quando há 4 bytes à
  frente e count <= 24 (o caminho original fica para o fim do buffer);
- plm_buffer_read_vlc tira cada bit direto do byte quando há bits no buffer.
O resultado é o mesmo bit a bit (conferido no host com tools/plm_check.c)."""
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


def main():
    src, dst = sys.argv[1], sys.argv[2]
    text = open(src, encoding='utf-8').read()
    for old, new in ((READ_OLD, READ_NEW), (VLC_OLD, VLC_NEW)):
        assert text.count(old) == 1, 'pl_mpeg.h mudou: ' + old.splitlines()[0]
        text = text.replace(old, new)
    open(dst, 'w', encoding='utf-8').write(text)


if __name__ == '__main__':
    main()
