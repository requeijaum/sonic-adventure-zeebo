#!/usr/bin/env python3
"""Integra o jogo (full.o) com os objetos do applet.

Uso: integrate.py <nm> <objcopy> <game.o> <saida.o> <obj do applet> ...

Os símbolos definidos fortes no jogo e num objeto do applet ficam FRACOS no
jogo: o applet (plataforma Zeebo) vence. Duplicatas entre objetos do applet,
ou duplicatas que o jogo deve vencer, não se resolvem aqui (o ld acusa).
Imprime quantos símbolos foram enfraquecidos."""
import subprocess
import sys


def strong_defs(nm, path):
    out = subprocess.run([nm, '--defined-only', '-g', path], capture_output=True, text=True).stdout
    syms = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in 'TDBRC':   # fortes (W/V/w ficam de fora)
            syms.add(parts[2])
    return syms


def main():
    nm, objcopy, game, out = sys.argv[1:5]
    applet = sys.argv[5:]
    mine = set()
    for o in applet:
        mine |= strong_defs(nm, o)
    clash = sorted(strong_defs(nm, game) & mine)
    cmd = [objcopy] + ['--weaken-symbol=' + s for s in clash] + [game, out]
    subprocess.run(cmd, check=True)
    print('enfraquecidos no jogo: %d' % len(clash))
    open(out + '.weak.txt', 'w').write('\n'.join(clash) + '\n')


if __name__ == '__main__':
    main()
