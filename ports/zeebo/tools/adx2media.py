#!/usr/bin/env python3
"""Converte o áudio do disco para o que o DSP do Zeebo toca sozinho.

  tools/adx2media.py [DIR]      (DIR padrão: build/zeebo, o disco montado)

Música e filmes (*.adx) viram MP3 (decodificador de MP3 do DSP, zero CPU do ARM):
    nome.adx  ->  nome.mp3            a faixa inteira
                  nome_lp.mp3         só o laço (do início do laço ao fim), se o ADX tem laço
As vozes (event_adx.afs, ADX mono 22 kHz) viram IMA ADPCM .wav (até 4 objetos de áudio
ao mesmo tempo, sem disputar o único decodificador de MP3 com a música) num contêiner:
    event_adx.zva = 'ZVA1', n (u32 LE), n x (offset, tamanho) u32 LE, dados
O ADX original não muda: sem os arquivos novos o jogo cai no mixer por software.
Precisa do ffmpeg (libmp3lame). Idempotente: pula o que já está mais novo que a fonte.
"""
import concurrent.futures as cf
import glob
import os
import struct
import subprocess
import sys

MP3_BITRATE = "128k"


def adx_info(path):
    with open(path, "rb") as f:
        h = f.read(0x30)
    if len(h) < 0x20 or h[0] != 0x80:
        return None
    rate, total = struct.unpack(">II", h[8:16])
    ls = le = 0
    if h[18] == 3 and len(h) >= 0x2C and struct.unpack(">I", h[0x18:0x1C])[0]:
        ls = struct.unpack(">I", h[0x1C:0x20])[0]
        le = struct.unpack(">I", h[0x24:0x28])[0]
    return rate, total, ls, le


def newer(dst, src):
    return os.path.exists(dst) and os.path.getmtime(dst) >= os.path.getmtime(src)


def encode(src, dst, start=None, end=None):
    af = []
    if start is not None or end is not None:
        a = []
        if start is not None:
            a.append(f"start_sample={start}")
        if end is not None:
            a.append(f"end_sample={end}")
        af = ["-af", "atrim=" + ":".join(a) + ",asetpts=N/SR/TB"]
    cmd = ["ffmpeg", "-v", "error", "-y", "-i", src, *af, "-c:a", "libmp3lame",
           "-b:a", MP3_BITRATE, "-ar", "44100", "-ac", "2", dst]
    subprocess.run(cmd, check=True)


def music(path):
    stem = path[: path.rfind(".")]
    info = adx_info(path)
    if info is None:
        return f"{path}: nao e ADX"
    rate, total, ls, le = info
    out = []
    if not newer(stem + ".mp3", path):
        encode(path, stem + ".mp3")
        out.append("mp3")
    if ls > 0 and le > ls:
        if not newer(stem + "_lp.mp3", path):
            encode(path, stem + "_lp.mp3", start=ls, end=le)
            out.append("lp")
    return f"{os.path.basename(path)}: {' '.join(out) or 'ja pronto'}"


def voice_wav(adx):
    if len(adx) < 32 or adx[0] != 0x80:
        return b""  # entrada vazia ou que nao e ADX: tamanho 0 no contenor
    # em dois passos: do ADX direto para o IMA o ffmpeg recusa 18 das 2088 falas ("frame_size
    # was not respected"); com PCM no meio o codificador recebe quadros do tamanho certo
    pcm = subprocess.run(
        ["ffmpeg", "-v", "error", "-f", "adx", "-i", "-", "-ar", "22050", "-ac", "1",
         "-f", "s16le", "-"], input=adx, capture_output=True)
    if pcm.returncode != 0 or not pcm.stdout:
        return b""
    p = subprocess.run(
        ["ffmpeg", "-v", "error", "-f", "s16le", "-ar", "22050", "-ac", "1", "-i", "-",
         "-c:a", "adpcm_ima_wav", "-f", "wav", "-"], input=pcm.stdout, capture_output=True)
    return p.stdout if p.returncode == 0 else b""


def voices(d):
    afs = os.path.join(d, "event_adx.afs")
    dst = os.path.join(d, "event_adx.zva")
    if not os.path.exists(afs) or newer(dst, afs):
        return "vozes: ja pronto" if os.path.exists(afs) else "vozes: sem event_adx.afs"
    data = open(afs, "rb").read()
    n = struct.unpack("<I", data[4:8])[0]
    ents = [struct.unpack("<II", data[8 + 8 * i: 16 + 8 * i]) for i in range(n)]
    with cf.ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as ex:
        wavs = list(ex.map(lambda e: voice_wav(data[e[0]: e[0] + e[1]]), ents))
    off = 8 + 8 * n
    table = b""
    for w in wavs:
        table += struct.pack("<II", off, len(w))
        off += len(w)
    with open(dst, "wb") as f:
        f.write(b"ZVA1" + struct.pack("<I", n) + table + b"".join(wavs))
    vazias = sum(1 for w in wavs if not w)
    return f"vozes: {n} -> {dst} ({off} bytes, {vazias} vazias/invalidas)"


def main():
    d = sys.argv[1] if len(sys.argv) > 1 else "build/zeebo"
    seen = set()
    files = []
    for p in sorted(glob.glob(os.path.join(d, "*.[aA][dD][xX]"))):
        low = os.path.basename(p).lower()
        if low in seen:
            continue
        seen.add(low)
        files.append(p)
    with cf.ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as ex:
        for line in ex.map(music, files):
            print(line, flush=True)
    print(voices(d))


if __name__ == "__main__":
    main()
