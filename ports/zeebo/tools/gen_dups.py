#!/usr/bin/env python3
"""Programas linkados no mesmo .mod que definem o mesmo simbolo de dados.

No DC cada programa e lido no mesmo endereco (MODULE_AREA), entao dois programas
podem ter dados 'no mesmo lugar' (dat_0c9e28a0). Num .mod unico isso e
definicao dupla. Aqui cada programa passa a definir P__S (objcopy
--redefine-sym) e o jogo le S, que fica num buffer do tamanho maximo; ao abrir
o programa (zgdfs.c chama zprog_enter) P__S e copiado para S. So dados (D/B/R);
funcao duplicada e erro.

uso: gen_dups.py OUT NM OBJCOPY PROGRAM...  (OUT/program-P.o -> OUT/dup/program-P.o,
OUT/gen/zdups.c)"""
import collections
import os
import shutil
import subprocess
import sys

out, nm, objcopy, progs = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:]
SKIP = ('_data_start', '_data_end', '_bss_start', '_bss_end')
defs = collections.defaultdict(dict)  # simbolo -> {programa: (tipo, tamanho)}
for p in progs:
    txt = subprocess.run([nm, '-S', '-g', '--defined-only', f'{out}/program-{p}.o'],
                         capture_output=True, text=True, check=True).stdout
    for line in txt.splitlines():
        f = line.split()
        if len(f) == 4:
            size, typ, name = int(f[1], 16), f[2], f[3]
        elif len(f) == 3:
            size, typ, name = 0, f[1], f[2]
        else:
            continue
        if name.endswith(SKIP):
            continue
        defs[name][p] = (typ, size)

dups = {n: v for n, v in defs.items() if len(v) > 1}
bad = [n for n, v in dups.items() if any(t not in 'DdBbRr' for t, _ in v.values())]
if bad:
    sys.exit('gen_dups: funcao definida em mais de um programa: ' + ' '.join(bad))

os.makedirs(f'{out}/dup', exist_ok=True)
os.makedirs(f'{out}/gen', exist_ok=True)
per = collections.defaultdict(list)
for n, v in dups.items():
    for p in v:
        per[p].append(n)
for p in progs:
    src, dst = f'{out}/program-{p}.o', f'{out}/dup/program-{p}.o'
    if per[p]:
        subprocess.run([objcopy] + [f'--redefine-sym={n}={p}__{n}' for n in per[p]] + [src, dst],
                       check=True)
    else:
        shutil.copyfile(src, dst)

c = ['/* gerado por tools/gen_dups.py */', 'extern void *memcpy(void *, const void *, unsigned long);',
     'struct zdup { const char *prog; const void *src; void *dst; unsigned long size; };']
for n, v in sorted(dups.items()):
    size = max(s for _, s in v.values()) or 8
    c.append(f'char {n}[{size}] __attribute__((aligned(8)));')
    for p in v:
        c.append(f'extern char {p}__{n}[];')
c.append('static const struct zdup zdups[] = {')
for n, v in sorted(dups.items()):
    for p, (_, s) in sorted(v.items()):
        c.append(f'    {{"{p}", {p}__{n}, {n}, {s or 8}}},')
c.append('    {0, 0, 0, 0},')
c.append('};')
c.append('static int zeq(const char *a, const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}')
c.append('void zprog_enter(const char *low)')
c.append('{ int i; for (i = 0; zdups[i].prog; i++) if (zeq(zdups[i].prog, low)) memcpy(zdups[i].dst, zdups[i].src, zdups[i].size); }')
open(f'{out}/gen/zdups.c', 'w').write('\n'.join(c) + '\n')
print(f'gen_dups: {len(dups)} simbolos duplicados em {len(per)} programas')
