#!/bin/bash
# FPS e tempo de jogo por fase, no zeebx do Ryzen. Roda as linhas do menu de
# debug (remote_matrix.sh) e resume as linhas "vprof" de cada log numa tabela
# (build/zeebo/matrix_r/fps.md). O numero mede a EMULACAO: serve para comparar
# fases entre si, nao como FPS do aparelho (para isso: FPSHUD=1 no build).
#   tools/fps_matrix.sh [-j N] [-s SEGUNDOS] LINHA...
set -eu
cd "$(dirname "$0")"
OUTD=../../../build/zeebo/matrix_r
rm -f "$OUTD"/summary.txt
LOGPAT='vprof|mem|TRAP' LOGN=400 ./remote_matrix.sh "$@" >/dev/null
python3 - "$OUTD" "$@" <<'PY'
import re, sys, os
outd = sys.argv[1]
args = [a for a in sys.argv[2:] if a.isdigit()]
rows = []
for r in args:
    p = os.path.join(outd, 'row%s.log' % r)
    if not os.path.exists(p):
        continue
    sums = {'quadros': 0, 'janela ms': 0, 'jogo': 0}
    min_free = None
    windows = 0
    trap = False
    for l in open(p, errors='replace'):
        if 'TRAP' in l and 'trap ' not in l:
            trap = True
        m = re.search(r'sa1z: (vprof|mem) (.+?) 0x([0-9a-f]+)(?:\s+\((\d+)x\))?\s*$', l)
        if not m:
            continue
        kind, key, val, n = m.group(1), m.group(2), int(m.group(3), 16), int(m.group(4) or 1)
        if kind == 'vprof' and key in ('quadros', 'janela ms', 'jogo'):
            sums[key] += val * n
            if key == 'quadros':
                windows += n
        if kind == 'mem' and key == 'livre KB':
            min_free = val if min_free is None else min(min_free, val)
    if sums['janela ms'] == 0:
        rows.append((r, None))
        continue
    fps = sums['quadros'] * 1000.0 / sums['janela ms']
    game = sums['jogo'] / max(sums['quadros'], 1)
    rows.append((r, (fps, game, windows, min_free, trap)))
out = ['| linha | FPS (zeebx) | ms de jogo/quadro | janelas de 2 s | RAM livre mín. KB | TRAP |',
       '|---|---|---|---|---|---|']
for r, v in rows:
    if v is None:
        out.append('| %s | sem dados | | | | |' % r)
    else:
        out.append('| %s | %.1f | %.1f | %d | %s | %s |' % (r, v[0], v[1], v[2], v[3], 'sim' if v[4] else ''))
open(os.path.join(outd, 'fps.md'), 'w').write('\n'.join(out) + '\n')
print('\n'.join(out))
PY
