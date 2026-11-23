#!/usr/bin/env python3
"""Overlays de dados dos programas de modulo (stages, campos, chefes).

Os programas do DC sao lidos no mesmo endereco (MODULE_AREA): so um fica
residente. Aqui o .data de cada programa vira uma secao .ovl_<P> e todas
dividem a mesma regiao (OVERLAY do ld). O .mod principal leva so a regiao (bss);
os dados vao para <P>.ovl no disco, comprimidos em PRS, e zgdfs.c carrega o
arquivo quando o jogo abre <P>.prs/<P>.bin.

  gen_ovl.py script VENDOR_X OUT_A OUT_B SECAO...
      gera os dois scripts de link: A (producao, regiao NOLOAD) e B (extracao,
      secoes com conteudo no arquivo).
  gen_ovl.py extract ELF_B OUTDIR PRSCOMP PROGRAMA...
      le o ELF do link B e grava OUTDIR/<P>.ovl

Formato do .ovl (u32 little-endian):
  0  'OVL1'  4  ro_ref  8  rw_ref  12 rw_start  16 nrel  20 rel_raw  24 rel_prs
  28 dat_raw 32 dat_prs  36 [PRS(relocs)] [PRS(dados)]
  ro_ref/rw_ref: enderecos de link de gdFsOpen (texto) e zovl_region (dados);
  em execucao a diferenca para os enderecos reais vira o delta de cada classe.
  reloc = deslocamento na regiao | (1<<31) se o alvo e dado (>= rw_start).
"""
import os
import struct
import subprocess
import sys
import tempfile


def make_scripts(vendor_x, out_a, out_b, secs):
    text = open(vendor_x, encoding='latin-1').read().replace('\r\n', '\n')
    marker = '  /* End of read-write section */'
    assert marker in text, 'marcador do elf2mod.x nao achado'
    body = ''.join(f'    {s} {{ *({s}) }}\n' for s in secs)
    for out, noload in ((out_a, ' (NOLOAD)'), (out_b, '')):
        blk = (f'  . = ALIGN(16);\n'
               f'  OVERLAY .{noload} : NOCROSSREFS_PLACEHOLDER\n  {{\n{body}  }}\n'
               f'  zovl_region = ADDR({secs[0]});\n'
               f'  . = ALIGN(32 / 8);\n')
        blk = blk.replace(' NOCROSSREFS_PLACEHOLDER', '')
        open(out, 'w', encoding='latin-1').write(text.replace(marker, blk + marker, 1))


class Elf:
    def __init__(self, path):
        d = self.d = open(path, 'rb').read()
        assert d[:4] == b'\x7fELF' and d[4] == 1 and d[5] == 1, 'ELF32 LE esperado'
        (self.shoff,) = struct.unpack_from('<I', d, 32)
        shentsize, shnum, shstrndx = struct.unpack_from('<HHH', d, 46)
        self.sec = []
        for i in range(shnum):
            o = self.shoff + i * shentsize
            name, typ, flags, addr, off, size, link, info, align, entsize = \
                struct.unpack_from('<IIIIIIIIII', d, o)
            self.sec.append(dict(name=name, type=typ, flags=flags, addr=addr, off=off,
                                 size=size, link=link, info=info, entsize=entsize))
        stro = self.sec[shstrndx]['off']
        for s in self.sec:
            e = d.index(b'\0', stro + s['name'])
            s['n'] = d[stro + s['name']:e].decode()
        self.by = {s['n']: s for s in self.sec}
        st = self.by['.symtab']
        sn = self.sec[st['link']]['off']
        self.syms = []
        for i in range(st['size'] // 16):
            nm, val, sz, info, other, shndx = struct.unpack_from('<IIIBBH', d, st['off'] + i * 16)
            e = d.index(b'\0', sn + nm)
            self.syms.append((d[sn + nm:e].decode(), val, shndx))

    def sym(self, name):
        for n, v, _ in self.syms:
            if n == name:
                return v
        raise KeyError(name)

    def data(self, s):
        return self.d[s['off']:s['off'] + s['size']]


def extract(elf_b, outdir, prscomp, progs):
    e = Elf(elf_b)
    os.makedirs(outdir, exist_ok=True)
    ro_ref = e.sym('gdFsOpen')
    rw_ref = e.sym('zovl_region')
    rw_start = min(s['addr'] for s in e.sec if s['flags'] & 1 and s['flags'] & 2 and s['addr'])
    tmp = tempfile.mkdtemp()
    total = 0
    for p in progs:
        s = e.by[f'.ovl_{p}']
        rs = e.by.get(f'.rel.ovl_{p}')
        dat = bytearray(e.data(s))
        rels = []
        if rs is not None:
            for i in range(rs['size'] // 8):
                off, info = struct.unpack_from('<II', e.d, rs['off'] + i * 8)
                typ, symi = info & 0xff, info >> 8
                if typ != 2:  # R_ARM_ABS32
                    sys.exit(f'{p}: reloc tipo {typ} em dados (so ABS32 e esperado)')
                if e.syms[symi][2] == 0:  # indefinido (--unresolved-symbols=ignore-all)
                    continue
                o = off - s['addr']
                (t,) = struct.unpack_from('<I', dat, o)
                rels.append(o | ((1 << 31) if t >= rw_start else 0))
        raw_rel = b''.join(struct.pack('<I', r) for r in rels)
        files = {}
        for tag, blob in (('r', raw_rel), ('d', bytes(dat))):
            fi, fo = f'{tmp}/{p}.{tag}', f'{tmp}/{p}.{tag}.prs'
            open(fi, 'wb').write(blob)
            subprocess.run([prscomp, 'c', fi, fo], check=True, stdout=subprocess.DEVNULL)
            files[tag] = open(fo, 'rb').read()
        hdr = struct.pack('<4s8I', b'OVL1', ro_ref, rw_ref, rw_start, len(rels),
                          len(raw_rel), len(files['r']), len(dat), len(files['d']))
        open(f'{outdir}/{p}.ovl', 'wb').write(hdr + files['r'] + files['d'])
        total += len(hdr) + len(files['r']) + len(files['d'])
        print(f'  {p}.ovl: dados {len(dat)} B, {len(rels)} relocs -> '
              f'{len(hdr) + len(files["r"]) + len(files["d"])} B')
    print(f'gen_ovl: {len(progs)} overlays, {total} bytes no disco, regiao {max(len(e.data(e.by[f".ovl_{p}"])) for p in progs)} B')


if __name__ == '__main__':
    cmd = sys.argv[1]
    if cmd == 'script':
        make_scripts(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5:])
    elif cmd == 'extract':
        extract(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5:])
    else:
        sys.exit(__doc__)
