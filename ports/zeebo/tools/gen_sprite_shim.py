#!/usr/bin/env python3
"""Gera o shim de sa1/sprite.h para o port Zeebo.

Copia o header do decomp e troca UMA função: sprite_send_corner, que
escreveria na store queue do PowerVR, passa a entregar o vértice ao sink
GLES1 (zstrip_vertex). O resto (cantos, ângulo, clip 2D) fica intacto.

Assim o repositório guarda só este gerador, não o texto do upstream.
"""
import sys


def main() -> int:
    src, dst = sys.argv[1], sys.argv[2]
    text = open(src, encoding='utf-8').read()
    text = text.replace(
        '#include "sa_sa1.h"',
        '#include "sa_sa1.h"\n\n#include "zstrip.h"\n\n'
        '/* preenchido pelo começo do strip (njkm_zeebo.c) */\n'
        'uint32_t sprite_sink_color(void);', 1)
    start = text.index('ALWAYS_INLINE struct sprite_vertex *\nsprite_send_corner')
    endmark = 'return (struct sprite_vertex *)at + 1;\n}'
    end = text.index(endmark, start) + len(endmark)
    replacement = '''ALWAYS_INLINE struct sprite_vertex *
sprite_send_corner(struct sprite_vertex *sq, const struct sprite_corner *corner,
                   Uint32 control)
{
    (void)sq;
    (void)control;
    zstrip_vertex(corner->x, corner->y, corner->rhw, corner->u, corner->v,
                  sprite_sink_color());
    return sq;
}'''
    open(dst, 'w', encoding='utf-8').write(text[:start] + replacement + text[end:])
    return 0


if __name__ == '__main__':
    sys.exit(main())
