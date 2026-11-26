#!/usr/bin/env python3
"""pvmdump.py ARQ.prs|ARQ.pvm OUTDIR -- descomprime PRS, lê o PVM e grava cada PVRT como PNG + folha de contatos."""
import struct, sys, os
from PIL import Image


def prs_decompress(src):
    out = bytearray(); i = 0; ctl = 0; bits = 0

    def bit():
        nonlocal ctl, bits, i
        if bits == 0:
            ctl = src[i]; i += 1; bits = 8
        b = ctl & 1; ctl >>= 1; bits -= 1
        return b
    while i < len(src):
        if bit():
            out.append(src[i]); i += 1; continue
        if bit():
            o16 = src[i] | (src[i + 1] << 8); i += 2
            if o16 == 0: break
            n = o16 & 7
            off = (o16 >> 3) - 0x2000
            if n == 0:
                n = src[i] + 1; i += 1
            else:
                n += 2
        else:
            n = (bit() << 1) | bit(); n += 2
            off = src[i] - 0x100; i += 1
        for _ in range(n):
            out.append(out[len(out) + off])
    return bytes(out)


def untwiddle_idx(w, h):
    # Morton: x bits em posições ímpares, y em pares (DC)
    def spread(v):
        r = 0
        for b in range(16):
            r |= ((v >> b) & 1) << (2 * b)
        return r
    return lambda x, y: (spread(x) << 1) | spread(y)


def rgb(pf, v):
    if pf == 0:   # ARGB1555
        a = 255 if v & 0x8000 else 0
        return (((v >> 10) & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31, (v & 31) * 255 // 31, a)
    if pf == 1:   # RGB565
        return (((v >> 11) & 31) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31, 255)
    if pf == 2:   # ARGB4444
        return (((v >> 8) & 15) * 17, ((v >> 4) & 15) * 17, (v & 15) * 17, ((v >> 12) & 15) * 17)
    return (255, 0, 255, 255)


def decode_pvrt(blob):
    pf, df = blob[8], blob[9]
    w, h = struct.unpack('<HH', blob[12:16])
    data = blob[16:]
    im = Image.new('RGBA', (w, h)); px = im.load()
    tw = untwiddle_idx(w, h)
    bpp2 = 2
    # pula mipmaps: o maior nível vem por último nos twiddled+mip
    if df in (2, 4):
        # offsets: níveis do menor (1x1) ao maior; pad inicial de 4 bytes de cabeçalho
        sz = 1; off = 0; size = 1
        sizes = []
        while size < w:
            sizes.append(size); size *= 2
        sizes.append(w)
        if df == 2:
            off = 6  # 1x1 (2 bytes) + 4 de cabeçalho/padding
            tot = 0
            for s in sizes[:-1]:
                tot += s * s * 2
            off = tot + 4 if len(sizes) > 1 else 0
            data = data[off:]
        else:
            tot = 0
            for s in sizes[:-1]:
                tot += max(1, (s // 2) * (s // 2))
            data = data[tot + 1 + 2048:] if False else data
    try:
        if df in (1, 2):       # quadrada twiddled
            for y in range(h):
                for x in range(w):
                    k = tw(x, y) * 2
                    v = data[k] | (data[k + 1] << 8)
                    px[x, y] = rgb(pf, v)
        elif df in (3, 4):     # VQ
            cb = 256
            book = data[:cb * 8]
            idx = data[cb * 8:]
            if df == 4:
                # níveis menores antes dos índices do maior: tamanho dos índices do maior = (w/2*h/2)
                n_big = (w // 2) * (h // 2)
                idx = idx[len(idx) - n_big:]
            tw2 = untwiddle_idx(w // 2, h // 2)
            for y in range(h // 2):
                for x in range(w // 2):
                    c = idx[tw2(x, y)]
                    q = [book[c * 8 + k * 2] | (book[c * 8 + k * 2 + 1] << 8) for k in range(4)]
                    px[x * 2, y * 2] = rgb(pf, q[0]); px[x * 2, y * 2 + 1] = rgb(pf, q[1])
                    px[x * 2 + 1, y * 2] = rgb(pf, q[2]); px[x * 2 + 1, y * 2 + 1] = rgb(pf, q[3])
        elif df == 9:          # retangular linear
            for y in range(h):
                for x in range(w):
                    k = (y * w + x) * 2
                    px[x, y] = rgb(pf, data[k] | (data[k + 1] << 8))
        elif df == 13:         # retangular twiddled
            m = min(w, h)
            for by in range(0, h, m):
                for bx in range(0, w, m):
                    base = ((by // m) * (w // m) + (bx // m)) * m * m * 2 if True else 0
                    for y in range(m):
                        for x in range(m):
                            k = base + tw(x, y) * 2
                            px[bx + x, by + y] = rgb(pf, data[k] | (data[k + 1] << 8))
        else:
            return im, f'df={df} nao suportado'
    except IndexError:
        return im, f'truncado df={df}'
    return im, f'pf={pf} df={df}'


def main(path, outdir):
    raw = open(path, 'rb').read()
    if raw[:4] != b'PVMH':
        raw = prs_decompress(raw)
    assert raw[:4] == b'PVMH', raw[:8]
    flags, count = struct.unpack('<HH', raw[8:12])
    pos = 12; names = []
    for _ in range(count):
        idx = struct.unpack('<H', raw[pos:pos + 2])[0]; pos += 2
        name = ''
        if flags & 1:
            name = raw[pos:pos + 28].split(b'\0')[0].decode('latin1'); pos += 28
        if flags & 2: pos += 2
        if flags & 4: pos += 2
        if flags & 8: pos += 4
        names.append(name)
    os.makedirs(outdir, exist_ok=True)
    marks = []
    p = 0
    while True:
        p = raw.find(b'PVRT', p)
        if p < 0: break
        marks.append(p); p += 4
    tiles = []
    for n, p in enumerate(marks):
        size = struct.unpack('<I', raw[p + 4:p + 8])[0]
        blob = raw[p:p + 8 + size]
        im, info = decode_pvrt(blob)
        nm = names[n] if n < len(names) else f'tex{n}'
        im.save(os.path.join(outdir, f'{n:03d}_{nm}.png'))
        tiles.append((n, nm, im, info))
        print(n, nm, im.size, info)
    # folha de contatos
    cell = 128; cols = 8; rows = (len(tiles) + cols - 1) // cols
    sheet = Image.new('RGB', (cols * cell, max(rows, 1) * cell), (40, 40, 40))
    for k, (_, _, im, _) in enumerate(tiles):
        t = im.convert('RGB').resize((cell - 4, cell - 4))
        sheet.paste(t, ((k % cols) * cell + 2, (k // cols) * cell + 2))
    sheet.save(os.path.join(outdir, '_sheet.png'))


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
