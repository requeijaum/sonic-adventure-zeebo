#!/usr/bin/env python3
"""gen_traps.py NM READELF SAIDA.c OBJ... — o que nenhum objeto define vira armadilha.

No .mod de teste os nomes sem definição valiam 0: uma chamada a um deles
saltava para o endereço 0 e o zeebx só mostrava "acesso a 0x0". Aqui cada
nome indefinido que algum objeto chama (relocação R_ARM_CALL/JUMP24/
THM_CALL) vira uma função que loga o nome uma vez e devolve 0; os demais
(dados: texlists, tabelas) viram 512 bytes de zeros. Assim o jogo segue e o
log diz o que falta."""
import os
import re
import subprocess
import sys

def elf_called(path):
    """Nomes alvo de relocações de chamada (R_ARM_CALL/JUMP24/THM_CALL) num
    ELF32 LE relocável. Lê as tabelas .rel direto: o `readelf -rW` num objeto
    de 100 MB levava ~6 minutos (a saída em texto é o custo); aqui são
    segundos e o resultado é o mesmo conjunto de nomes."""
    from array import array
    calls = {10, 28, 29}  # THM_CALL, CALL, JUMP24
    names = set()
    with open(path, 'rb') as fh:
        data = fh.read()
    if data[:4] != b'\x7fELF' or data[4] != 1 or data[5] != 1:
        raise SystemExit(f'gen_traps: {path} não é ELF32 little-endian')
    shoff = int.from_bytes(data[32:36], 'little')
    shentsize = int.from_bytes(data[46:48], 'little')
    shnum = int.from_bytes(data[48:50], 'little')
    secs = []
    for i in range(shnum):
        b = shoff + i * shentsize
        secs.append({
            'type': int.from_bytes(data[b + 4:b + 8], 'little'),
            'off': int.from_bytes(data[b + 16:b + 20], 'little'),
            'size': int.from_bytes(data[b + 20:b + 24], 'little'),
            'link': int.from_bytes(data[b + 24:b + 28], 'little'),
            'entsize': int.from_bytes(data[b + 36:b + 40], 'little'),
        })
    for sec in secs:
        if sec['type'] not in (9, 4):  # SHT_REL (SHT_RELA não ocorre em ARM)
            continue
        if sec['type'] == 4:
            raise SystemExit(f'gen_traps: {path} tem .rela, não suportado')
        symtab = secs[sec['link']]
        strtab = secs[symtab['link']]
        words = array('I')
        words.frombytes(data[sec['off']:sec['off'] + sec['size']])
        syms = set()
        for k in range(1, len(words), 2):
            info = words[k]
            if (info & 0xff) in calls:
                syms.add(info >> 8)
        for idx in syms:
            sb = symtab['off'] + idx * 16
            st_name = int.from_bytes(data[sb:sb + 4], 'little')
            start = strtab['off'] + st_name
            names.add(data[start:data.index(b'\0', start)].decode('latin1'))
    return names


nm, readelf, out, objs = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:]
defined, undefined = set(), set()
for line in subprocess.run([nm, '-A', *objs], capture_output=True, text=True).stdout.splitlines():
    f = line.rsplit(':', 1)[-1].split()
    if len(f) == 2 and f[0] in 'Uw':
        undefined.add(f[1])
    elif len(f) == 3 and f[1] not in 'Uw':
        defined.add(f[2])
missing = sorted(n for n in undefined - defined if re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', n)
                 and not n.startswith('__aeabi') and not n.startswith('__'))
called = set()
for o in objs:
    called |= elf_called(o)
funcs = [n for n in missing if n in called]
data = [n for n in missing if n not in called]
with open(out, 'w') as fh:
    fh.write('/* gerado por tools/gen_traps.py: nomes que nenhum objeto define */\n')
    fh.write('void zlog(const char *what, const char *name, int value);\n\n')
    for n in funcs:
        fh.write(f'int {n}(void) {{ static int seen; if (!seen) {{ seen = 1; '
                 f'zlog("trap", "{n}", (int)__builtin_return_address(0)); }} return 0; }}\n')
    # TRAP_FILL=udf (depuração): os dados viram instruções UDF, para que um
    # salto num deles pare ali e o zeebx mostre o endereço (nm diz o nome)
    udf = os.environ.get('TRAP_FILL') == 'udf'
    for n in data:
        if udf:
            fh.write(f'unsigned int {n}[128] __attribute__((aligned(32))) = '
                     f'{{[0 ... 127] = 0xe7f000f0u}};\n')
        else:
            fh.write(f'unsigned char {n}[512] __attribute__((aligned(32)));\n')
print(f'armadilhas: {len(funcs)} funções, {len(data)} dados')
