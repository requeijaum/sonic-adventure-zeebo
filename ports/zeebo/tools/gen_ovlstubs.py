#!/usr/bin/env python3
"""gen_ovlstubs.py OUT NM OBJCOPY PROGRAMA... — codigo dos programas em overlay.

O codigo de cada programa (.text) vira a secao .ovlc_<P>, numa regiao de codigo que o
zovl_load enche ao abrir o programa. O .mod (RO) chama as funcoes que o programa exporta por
`bl`, mas um bl do RO para o RW (a regiao) o elf2mod nao aceita (o BREW pode por RO e RW
em bases diferentes: o salto relativo quebraria). Aqui cada funcao global de um programa
ganha, no .mod, um stub `F: ldr pc, [pc, #-4]; .word F__ovlf` (um literal ABS32, que a lista de
relocacao do .mod trata por classe), e o programa passa a definir F__ovlf.

Escreve OUT/gen/zovlstubs.S e OUT/ovl/program-<P>.o a partir de OUT/dup/program-<P>.o."""
import os
import subprocess
import sys

out, nm, objcopy = sys.argv[1:4]
progs = sys.argv[4:]
os.makedirs(f'{out}/ovl', exist_ok=True)
os.makedirs(f'{out}/gen', exist_ok=True)
stubs = ['\t.arm\n\t.text\n\t.align 2\n']
total = 0
for p in progs:
    src = f'{out}/dup/program-{p}.o'
    names = []
    for line in subprocess.run([nm, '-g', '--defined-only', src], capture_output=True,
                               text=True, check=True).stdout.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in ('T', 'W'):
            names.append(parts[2])
    rs = f'{out}/ovl/program-{p}.redef'
    with open(rs, 'w') as fh:
        for n in names:
            fh.write(f'{n} {n}__ovlf\n')
    cmd = [objcopy, f'--rename-section', f'.data=.ovl_{p}', '--rename-section', f'.text=.ovlc_{p}']
    if names:
        cmd.append(f'--redefine-syms={rs}')
    subprocess.run(cmd + [src, f'{out}/ovl/program-{p}.o'], check=True)
    for n in names:
        stubs.append(f'\t.globl {n}\n\t.type {n}, %function\n{n}:\n\tldr pc, [pc, #-4]\n'
                     f'\t.word {n}__ovlf\n')
    total += len(names)
with open(f'{out}/gen/zovlstubs.S', 'w') as fh:
    fh.write('/* gerado por tools/gen_ovlstubs.py */\n' + ''.join(stubs))
print(f'gen_ovlstubs: {total} stubs em {len(progs)} programas')
