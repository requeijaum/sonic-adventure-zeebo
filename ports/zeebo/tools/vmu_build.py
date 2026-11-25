#!/usr/bin/env python3
"""Monta cartoes VMU (sa1vmu*.bin) para a porta do SA1 a partir de .VMS do Bucanero (com o .vmi
do lado, que traz o nome do arquivo) ou de uma imagem de VMU de 128 KB.

Formato do cartao (ports/zeebo/zshinobi.c): 'VMU1' (4) + 24 entradas de 28 bytes
(nome[16], first u16, blocos u16, tempo[8] = u16 ano + 6 x u8) + dados em blocos de 512.
Arquivos por drive: A1 = sa1vmu.bin; os outros sa1vmu_<a-d><1|2>.bin.

uso:
  vmu_build.py build SPEC.json [--out-dir DIR] [--force]
  vmu_build.py list CARTAO.bin
SPEC.json:
  {"bucanero": "DIR com os .vmi/.VMS",
   "cards": [{"drive": "b1", "title": "...",
              "save":   {"vms": "v34162.VMS"} | {"vmu": "x.A1.bin", "file": "SONICADV_INT"},
              "extras": [{"vms": "v17434.VMS"}, ...]}]}
"""
import argparse
import json
import os
import struct
import sys
import time

BLOCK, BLOCKS, FILES = 512, 200, 24
MAGIC = b"VMU1"
SAVE_SIZE, SAVE_COUNT, SAVE_HEADER = 0x4A0, 3, 128 + 2 * 512
KEY, STEP, M32 = 0x5A455441, 0x4E414E41, 0xFFFFFFFF
HEX = set("0123456789ABCDEF")
DRIVES = [f"{p}{u}" for p in "abcd" for u in "12"]
RENAME = {"SONICADV_INT": "SONICADV_SYS"}


class BuildError(Exception):
    pass


def card_file_name(drive):
    drive = drive.lower()
    if drive not in DRIVES:
        raise BuildError(f"drive invalido: {drive} (use {', '.join(DRIVES)})")
    return "sa1vmu.bin" if drive == "a1" else f"sa1vmu_{drive}.bin"


def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0x8408 if crc & 1 else crc >> 1
    return crc ^ 0xFFFF


def main_save_slots_ok(raw):
    """quantos dos 3 slots do SONICADV_SYS passam no CRC de save_checksum (save.c:310)."""
    body = raw[SAVE_HEADER:]
    ok = 0
    for i in range(SAVE_COUNT):
        s = body[i * SAVE_SIZE:(i + 1) * SAVE_SIZE]
        ok += len(s) == SAVE_SIZE and int.from_bytes(s[:4], "little") & 0xFFFF == crc16(s[4:])
    return ok


def event_sum_ok(raw):
    """download_read (download.c:729): corta 640 bytes, decifra, ~soma dos bytes signed de [64, n)."""
    length = (len(raw) + BLOCK - 1) // BLOCK * BLOCK - 640
    if length < 64:
        return False
    body = raw[640:640 + length].ljust(length, b"\0")
    dec = b"".join(struct.pack("<I", (struct.unpack_from("<I", body, i * 4)[0] ^ ((i * STEP) & M32) ^ KEY) & M32)
                   for i in range(length // 4))
    saved = struct.unpack_from("<I", dec, 0x2C)[0]
    total = sum(b - 256 if b > 127 else b for b in dec[64:length])
    return (~total) & M32 == saved


def read_vmi_name(vms_path):
    """nome do arquivo (12 bytes em 0x58 do .vmi), procurando o .vmi de mesmo nome em qualquer caixa."""
    folder, base = os.path.split(vms_path)
    stem = os.path.splitext(base)[0].lower()
    for f in os.listdir(folder or "."):
        if os.path.splitext(f)[0].lower() == stem and f.lower().endswith(".vmi"):
            b = open(os.path.join(folder, f), "rb").read()
            if len(b) < 0x68:
                raise BuildError(f"{f}: .vmi curto demais")
            return b[0x58:0x64].split(b"\0")[0].decode("ascii")
    raise BuildError(f"sem .vmi ao lado de {vms_path}")


def read_vmu_image(path, name):
    """le o arquivo `name` de uma imagem de VMU de 128 KB (raiz no bloco 255, FAT no 254)."""
    img = open(path, "rb").read()
    if len(img) != 256 * BLOCK:
        raise BuildError(f"{path}: nao e uma imagem de VMU de 128 KB")
    root = img[255 * BLOCK:256 * BLOCK]
    fat_loc, _fat_size, dir_loc, dir_size = struct.unpack_from("<HHHH", root, 0x46)
    fat = struct.unpack_from("<256H", img, fat_loc * BLOCK)
    for d in range(dir_size):
        blk = img[(dir_loc - d) * BLOCK:(dir_loc - d + 1) * BLOCK]
        for e in range(16):
            ent = blk[e * 32:(e + 1) * 32]
            if ent[0] == 0:
                continue
            fname = ent[4:16].split(b"\0")[0].decode("ascii", "replace")
            if fname != name:
                continue
            first = struct.unpack_from("<H", ent, 2)[0]
            size = struct.unpack_from("<H", ent, 24)[0]
            out, b = b"", first
            for _ in range(size):
                out += img[b * BLOCK:(b + 1) * BLOCK]
                b = fat[b]
            return out
    raise BuildError(f"{name} nao esta em {path}")


def load_source(src, base):
    """-> (nome no cartao, bytes)"""
    if "vms" in src:
        path = os.path.join(os.path.expanduser(base), src["vms"])
        name = read_vmi_name(path)
        data = open(path, "rb").read()
    elif "vmu" in src:
        name = src["file"]
        data = read_vmu_image(os.path.expanduser(src["vmu"]), name)
    else:
        raise BuildError(f"fonte sem vms/vmu: {src}")
    return RENAME.get(name, name), data


def validate(name, data, skip_checks):
    if skip_checks:
        return
    if name == "SONICADV_SYS":
        if main_save_slots_ok(data) == 0:
            raise BuildError("SONICADV_SYS: nenhum slot passa no CRC do decomp")
    elif name.startswith("SONICADV_") and len(name) == 12 and set(name[9:]) <= HEX:
        if not event_sum_ok(data):
            raise BuildError(f"{name}: soma de verificacao do download invalida")


def build_card(files, now=None):
    """files = [(nome, bytes)] -> bytes do cartao. Falha se passar de 24 arquivos ou 200 blocos."""
    if len(files) > FILES:
        raise BuildError(f"{len(files)} arquivos (maximo {FILES})")
    names = [n for n, _ in files]
    if len(set(names)) != len(names):
        raise BuildError("nome repetido no cartao: " + ", ".join(sorted({n for n in names if names.count(n) > 1})))
    t = time.localtime(now)
    stamp = struct.pack("<HBBBBBB", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, (t.tm_wday + 1) % 7)
    table, data, at = b"", b"", 0
    for name, raw in files:
        if len(name) > 12:
            raise BuildError(f"nome longo demais: {name}")
        raw = raw + b"\0" * (-len(raw) % BLOCK)
        n = len(raw) // BLOCK
        table += struct.pack("<16sHH8s", name.encode("ascii"), at, n, stamp)
        data += raw
        at += n
    if at > BLOCKS:
        raise BuildError(f"{at} blocos (maximo {BLOCKS})")
    table += b"\0" * (28 * (FILES - len(files)))
    return MAGIC + table + data


def parse_card(blob):
    """-> [(nome, first, blocos, bytes)]"""
    if blob[:4] != MAGIC:
        raise BuildError("magic 'VMU1' ausente")
    out = []
    for i in range(FILES):
        name, first, n, _ = struct.unpack_from("<16sHH8s", blob, 4 + i * 28)
        name = name.split(b"\0")[0].decode("ascii")
        if not name:
            continue
        if first + n > BLOCKS:
            raise BuildError(f"{name}: fora dos {BLOCKS} blocos")
        base = 4 + FILES * 28 + first * BLOCK
        out.append((name, first, n, blob[base:base + n * BLOCK]))
    return out


def cmd_build(a):
    spec = json.load(open(a.spec))
    base = spec.get("bucanero", ".")
    out_dir = os.path.expanduser(a.out_dir)
    os.makedirs(out_dir, exist_ok=True)
    seen = set()
    for card in spec["cards"]:
        fname = card_file_name(card["drive"])
        if fname in seen:
            raise BuildError(f"drive repetido: {card['drive']}")
        seen.add(fname)
        files = [load_source(card["save"], base)] + [load_source(x, base) for x in card.get("extras", [])]
        for n, d in files:
            validate(n, d, a.skip_checks)
        if not any(n == "SONICADV_SYS" for n, _ in files):
            raise BuildError(f"{card['drive']}: sem SONICADV_SYS/INT")
        blob = build_card(files)
        dest = os.path.join(out_dir, fname)
        if os.path.exists(dest):
            if not a.force:
                raise BuildError(f"{dest} ja existe (use --force; guarda .bak)")
            os.replace(dest, dest + ".bak")
        with open(dest, "wb") as fh:
            fh.write(blob)
        used = sum(len(d) + (-len(d) % BLOCK) for _, d in files) // BLOCK
        print(f"{dest}: {card.get('title', '')} | {', '.join(n for n, _ in files)} | {used}/{BLOCKS} blocos")


def cmd_list(a):
    blob = open(a.card, "rb").read()
    used = 0
    for name, first, n, raw in parse_card(blob):
        extra = ""
        if name == "SONICADV_SYS":
            extra = f" slots CRC ok: {main_save_slots_ok(raw)}/3"
        elif name.startswith("SONICADV_") and len(name) == 12 and set(name[9:]) <= HEX:
            extra = " soma download: " + ("ok" if event_sum_ok(raw) else "RUIM")
        print(f"{name:14} bloco {first:3} x {n:3}{extra}")
        used += n
    print(f"{used}/{BLOCKS} blocos usados")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build")
    b.add_argument("spec")
    b.add_argument("--out-dir", default="~/vmu-bucanero/out")
    b.add_argument("--force", action="store_true")
    b.add_argument("--skip-checks", action="store_true")
    b.set_defaults(fn=cmd_build)
    l = sub.add_parser("list")
    l.add_argument("card")
    l.set_defaults(fn=cmd_list)
    a = ap.parse_args(argv)
    try:
        a.fn(a)
    except BuildError as e:
        sys.exit(f"erro: {e}")


if __name__ == "__main__":
    main()
