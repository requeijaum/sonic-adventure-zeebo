#!/usr/bin/env python3
"""Importa o save principal de um VMU (.bin de 128 KB, RetroArch/Flycast) para o
cartao emulado da porta (sa1vmu.bin, formato de zshinobi.c: 'VMU1' + 24 entradas
de 28 bytes + blocos de 512).

O decomp (JP) procura SONICADV_SYS; o dump US chama o mesmo arquivo de
SONICADV_INT. O conteudo e igual (3 saves de 0x4a0 bytes, CRC-16 0x8408 conferido
aqui), so o nome muda. Chao/records (SONICADV_P00/S00/H0x) sao copiados se existirem.

uso: vmu_import.py VMU.bin [-o DESTINO/sa1vmu.bin] [--force]
"""
import argparse
import os
import struct
import sys
import time

sys.path.insert(0, os.path.expanduser("~/projects/dreamcast-vmu-sync/src"))
from vmu_parse import VMUImage  # noqa: E402

BLOCK = 512
BLOCKS = 200
FILES = 24
SAVE_SIZE = 0x4A0
SAVE_COUNT = 3
HEADER_BYTES = 128 + 2 * BLOCK  # cabecalho VMS + 2 icones
RENAME = {"SONICADV_INT": "SONICADV_SYS"}
KEEP = ("SONICADV_SYS", "SONICADV_P00", "SONICADV_S00")


def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0x8408 if crc & 1 else crc >> 1
    return crc ^ 0xFFFF


def check_main(raw):
    body = raw[HEADER_BYTES:]
    ok = 0
    for i in range(SAVE_COUNT):
        s = body[i * SAVE_SIZE:(i + 1) * SAVE_SIZE]
        ok += len(s) == SAVE_SIZE and int.from_bytes(s[:4], "little") & 0xFFFF == crc16(s[4:])
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("vmu")
    ap.add_argument("-o", "--out", default="sa1vmu.bin")
    ap.add_argument("--force", action="store_true", help="sobrescreve o destino (guarda .bak)")
    a = ap.parse_args()

    img = VMUImage.load(a.vmu)
    picked = []
    for f in img.files:
        name = RENAME.get(f.filename, f.filename)
        if name in KEEP:
            picked.append((name, f))
    if not any(n == "SONICADV_SYS" for n, _ in picked):
        sys.exit("SONICADV_INT/SONICADV_SYS nao encontrado no VMU")

    table, data, at = b"", b"", 0
    for name, f in picked:
        raw = bytes(f.data)
        raw += b"\0" * (-len(raw) % BLOCK)
        n = len(raw) // BLOCK
        if name == "SONICADV_SYS":
            good = check_main(raw)
            print(f"{name}: {n} blocos, saves com CRC valido: {good}/{SAVE_COUNT}")
            if good == 0:
                sys.exit("nenhum save valida pelo CRC do decomp: formato diferente")
        now = time.localtime()
        stamp = struct.pack("<HBBBBBB", now.tm_year, now.tm_mon, now.tm_mday, now.tm_hour,
                            now.tm_min, now.tm_sec, (now.tm_wday + 1) % 7)
        table += struct.pack("<16sHH8s", name.encode(), at, n, stamp)
        data += raw
        at += n
    if at > BLOCKS:
        sys.exit("nao cabe em 200 blocos")
    table += b"\0" * (28 * (FILES - len(picked)))
    out = b"VMU1" + table + data

    if os.path.exists(a.out):
        if not a.force:
            sys.exit(f"{a.out} ja existe (use --force; guarda {a.out}.bak)")
        os.replace(a.out, a.out + ".bak")
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    with open(a.out, "wb") as fh:
        fh.write(out)
    print(f"gravado {a.out}: {len(out)} bytes, {at} blocos")


if __name__ == "__main__":
    main()
