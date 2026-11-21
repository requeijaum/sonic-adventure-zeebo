#!/usr/bin/env python3
"""gen_traps.py NM READELF SAIDA.c OBJ... — o que nenhum objeto define vira armadilha.

No .mod de teste os nomes sem definição valiam 0: uma chamada a um deles
saltava para o endereço 0 e o zeebx só mostrava "acesso a 0x0". Aqui cada
nome indefinido que algum objeto chama (relocação R_ARM_CALL/JUMP24/
THM_CALL) vira uma função que loga o nome uma vez e devolve 0; os demais
(dados: texlists, tabelas) viram 512 bytes de zeros. Assim o jogo segue e o
log diz o que falta."""
import re
import subprocess
import sys

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
    rel = subprocess.run([readelf, '-rW', o], capture_output=True, text=True).stdout
    for line in rel.splitlines():
        if 'R_ARM_CALL' in line or 'R_ARM_JUMP24' in line or 'R_ARM_THM_CALL' in line:
            called.add(line.split()[-1].split('@')[0])
funcs = [n for n in missing if n in called]
data = [n for n in missing if n not in called]
with open(out, 'w') as fh:
    fh.write('/* gerado por tools/gen_traps.py: nomes que nenhum objeto define */\n')
    fh.write('void zlog(const char *what, const char *name, int value);\n\n')
    for n in funcs:
        fh.write(f'int {n}(void) {{ static int seen; if (!seen) {{ seen = 1; '
                 f'zlog("trap", "{n}", (int)__builtin_return_address(0)); }} return 0; }}\n')
    for n in data:
        fh.write(f'unsigned char {n}[512] __attribute__((aligned(32)));\n')
print(f'armadilhas: {len(funcs)} funções, {len(data)} dados')
