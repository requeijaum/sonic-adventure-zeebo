#!/usr/bin/env python3
"""Overlays de dados dos programas de modulo (stages, campos, chefes).

Os programas do DC sao lidos no mesmo endereco (MODULE_AREA): so um fica
residente. Aqui o .data de cada programa vira uma secao .ovl_<P> e todas
dividem a mesma regiao (OVERLAY do ld). O .mod principal leva so a regiao (bss);
os dados vao para <P>.ovl no disco, comprimidos em PRS, e zgdfs.c carrega o
arquivo quando o jogo abre <P>.prs/<P>.bin.

  gen_ovl.py script VENDOR_X OUT_A OUT_B GRUPO=prog,prog ...
      gera os dois scripts de link: A (producao, regiao NOLOAD) e B (extracao,
      secoes com conteudo no arquivo).
  gen_ovl.py extract ELF_B OUTDIR PRSCOMP GRUPO=prog,prog ...
      le o ELF do link B e grava OUTDIR/<P>.ovl

Formato do .ovl (u32 little-endian):
  0  'OVL1'  4  ro_ref  8  rw_ref  12 rw_start  16 nrel  20 rel_raw  24 rel_prs
  28 dat_raw 32 dat_prs 36 region_link 40 region_size  44 [PRS(relocs)] [PRS(dados)]
  uma regiao por grupo (modulo, field, extra, tikal): programas do mesmo grupo
  sao lidos no mesmo MODULE_AREA/FIELD_AREA/... e nunca coexistem
  ro_ref/rw_ref: enderecos de link de gdFsOpen (texto) e zovl_region (dados);
  em execucao a diferenca para os enderecos reais vira o delta de cada classe.
  reloc = deslocamento na regiao | (1<<31) se o alvo e dado (>= rw_start).
"""
import os
import struct
import hashlib
import subprocess
import sys
import atexit
import shutil
import tempfile


def parse_groups(args):
    """'modulo=a,b' ... -> [(grupo, [programas])]"""
    out = []
    for a in args:
        g, _, ps = a.partition('=')
        out.append((g, [p for p in ps.split(',') if p]))
    return out


def make_scripts(vendor_x, out_a, out_b, groups):
    text = open(vendor_x, encoding='latin-1').read().replace('\r\n', '\n')
    marker = '  /* End of read-write section */'
    assert marker in text, 'marcador do elf2mod.x nao achado'
    d = os.path.dirname(out_a)
    blk_a = '  . = ALIGN(16);\n  zovl_region = .;\n'
    blk_b = '  . = ALIGN(16);\n  zovl_region = .;\n'
    for g, progs in groups:
        secs = [f'.ovl_{p}' for p in progs]
        big = 0
        for p in progs:
            o = f'{d}/ovl/program-{p}.o'
            if os.path.exists(o):
                e = Elf(o)
                if f'.ovl_{p}' in e.by:
                    big = max(big, e.by[f'.ovl_{p}']['size'])
        big = (big + 63) & ~15
        body = ''.join(f'    {s} {{ *({s}) }}\n' for s in secs)
        # B (extracao): OVERLAY, secoes com conteudo, LMAs em sequencia
        blk_b += (f'  . = ALIGN(16);\n  OVERLAY . :\n  {{\n{body}  }}\n'
                  f'  zovl_{g} = ADDR({secs[0]});\n  zovl_{g}_size = {big};\n'
                  f'  . = zovl_{g} + {big};\n')
        # A (producao): o ld nao aceita NOLOAD em OVERLAY; secoes NOLOAD no mesmo
        # endereco viram NOBITS e a regiao tem o tamanho do maior
        body = ''.join(f'  {s} zovl_{g} (NOLOAD) : {{ *({s}) }}\n' for s in secs)
        blk_a += (f'  . = ALIGN(16);\n  zovl_{g} = .;\n{body}'
                  f'  zovl_{g}_size = {big};\n  . = zovl_{g} + {big};\n')
    for out, blk in ((out_a, blk_a + '  zovl_end = .;\n'), (out_b, blk_b + '  zovl_end = .;\n')):
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
        so = self.sec[st['link']]
        strtab = d[so['off']:so['off'] + so['size']]
        # offset -> nome, de uma vez (o loop com d.index por simbolo levava ~11 s)
        names, pos = {}, 0
        for part in strtab.split(b'\0'):
            names[pos] = part
            pos += len(part) + 1
        raw = d[st['off']:st['off'] + st['size']]
        self.syms = []
        for nm, val, _sz, _info, _other, shndx in struct.iter_unpack('<IIIBBH', raw):
            n = names.get(nm)
            if n is None:  # nome que aponta para o meio de outro (sufixo)
                n = strtab[nm:strtab.index(b'\0', nm)]
            self.syms.append((n.decode(), val, shndx))
        self._byname = {}
        for n, v, _ in self.syms:
            self._byname.setdefault(n, v)  # o primeiro, como antes

    def sym(self, name):
        try:
            return self._byname[name]
        except KeyError:
            raise KeyError(name)

    def data(self, s):
        return self.d[s['off']:s['off'] + s['size']]


def compress(prscomp, blob, tmp, name):
    """PRS do blob, com cache por conteudo: a maioria dos overlays nao muda de
    uma build para a outra, e comprimir os 242 levava ~20 s. A chave inclui o
    hash do proprio prscomp (mudou o compressor, mudou o resultado)."""
    cache = os.path.join(os.path.dirname(os.path.abspath(prscomp)), 'ovlcache')
    os.makedirs(cache, exist_ok=True)
    if not hasattr(compress, 'tool'):
        with open(prscomp, 'rb') as fh:
            compress.tool = hashlib.sha1(fh.read()).hexdigest()[:12]
    key = hashlib.sha1(blob).hexdigest() + '.' + compress.tool
    hit = os.path.join(cache, key)
    if os.path.exists(hit):
        with open(hit, 'rb') as fh:
            return fh.read()
    fi, fo = f'{tmp}/{name}', f'{tmp}/{name}.prs'
    with open(fi, 'wb') as fh:
        fh.write(blob)
    subprocess.run([prscomp, 'c', fi, fo], check=True, stdout=subprocess.DEVNULL)
    with open(fo, 'rb') as fh:
        out = fh.read()
    with open(hit + '.tmp', 'wb') as fh:
        fh.write(out)
    os.replace(hit + '.tmp', hit)
    return out


def extract(elf_b, outdir, prscomp, groups):
    e = Elf(elf_b)
    os.makedirs(outdir, exist_ok=True)
    ro_ref = e.sym('gdFsOpen')
    rw_ref = e.sym('zovl_region')
    gof = {p: g for g, ps in groups for p in ps}
    progs = [p for _, ps in groups for p in ps]
    rw_start = min(s['addr'] for s in e.sec if s['flags'] & 1 and s['flags'] & 2 and s['addr'])
    tmp = tempfile.mkdtemp()
    atexit.register(shutil.rmtree, tmp, True)  # eram ~93 MB deixados a cada build
    total = 0
    progs = [p for p in progs if f'.ovl_{p}' in e.by]
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
            files[tag] = compress(prscomp, blob, tmp, f'{p}.{tag}')
        g = gof[p]
        hdr = struct.pack('<4s10I', b'OVL1', ro_ref, rw_ref, rw_start, len(rels),
                          len(raw_rel), len(files['r']), len(dat), len(files['d']),
                          e.sym(f'zovl_{g}'), e.sym(f'zovl_{g}_size'))
        open(f'{outdir}/{p}.ovl', 'wb').write(hdr + files['r'] + files['d'])
        total += len(hdr) + len(files['r']) + len(files['d'])
        print(f'  {p}.ovl: dados {len(dat)} B, {len(rels)} relocs -> '
              f'{len(hdr) + len(files["r"]) + len(files["d"])} B')
    print(f'gen_ovl: {len(progs)} overlays, {total} bytes no disco, regiao {max(len(e.data(e.by[f".ovl_{p}"])) for p in progs)} B')


if __name__ == '__main__':
    cmd = sys.argv[1]
    if cmd == 'script':
        make_scripts(sys.argv[2], sys.argv[3], sys.argv[4], parse_groups(sys.argv[5:]))
    elif cmd == 'extract':
        extract(sys.argv[2], sys.argv[3], sys.argv[4], parse_groups(sys.argv[5:]))
    else:
        sys.exit(__doc__)
