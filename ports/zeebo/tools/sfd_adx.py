#!/usr/bin/env python3
"""Extrai o ADX de um filme Sofdec (.sfd) para <nome>.adx.

O .sfd é um MPEG-1 Program Stream; o fluxo 0xC0 é o ADX inteiro, com o
cabeçalho (o mesmo que zplm_extract_audio junta no aparelho). Feito na
montagem do disco, o jogo não precisa ler o filme inteiro antes de tocar.
    tools/sfd_adx.py arquivo.sfd [...]
"""
import sys


def extract(data):
    out = bytearray()
    i, n = 0, len(data)
    while i + 4 <= n:
        if data[i:i + 3] != b"\x00\x00\x01":
            i += 1
            continue
        sid = data[i + 3]
        if sid == 0xB9:  # fim do programa
            break
        if sid == 0xBA:  # pack: MPEG-1 tem 12 bytes, MPEG-2 14 + enchimento
            if data[i + 4] >> 6 == 1:
                i += 14 + (data[i + 13] & 7)
            else:
                i += 12
            continue
        if sid < 0xBB:
            i += 4
            continue
        plen = (data[i + 4] << 8) | data[i + 5]
        body, end = i + 6, i + 6 + plen
        if sid == 0xC0:
            j = body
            while j < end and data[j] == 0xFF:  # enchimento
                j += 1
            if data[j] >> 6 == 1:  # STD buffer
                j += 2
            t = data[j] >> 4
            j += 5 if t == 2 else 10 if t == 3 else 1
            out += data[j:end]
        i = end
    return bytes(out)


def main():
    for path in sys.argv[1:]:
        with open(path, "rb") as f:
            adx = extract(f.read())
        if len(adx) < 4 or adx[0] != 0x80:
            print(f"{path}: sem ADX", file=sys.stderr)
            continue
        dst = path[: path.rfind(".")] + ".adx"
        with open(dst, "wb") as f:
            f.write(adx)
        print(f"{dst}: {len(adx)} bytes")


if __name__ == "__main__":
    main()
