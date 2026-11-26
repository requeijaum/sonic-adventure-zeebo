#!/usr/bin/env python3
"""dd_summary.py [DD] -- resume os logs de dd_matrix.sh: por linha do menu e por pct, as medias das janelas de 2 s
depois que a fase comecou (janelas com 'tdist entradas' > 0, ou as 60% finais). Saida em markdown."""
import os, re, sys, statistics

DD = sys.argv[1] if len(sys.argv) > 1 else os.path.expanduser('~/.hermes/cache/scratch/dd')
LOGS = os.path.join(DD, 'logs')
PAT = re.compile(r'sa1z: (vprof|tperf|tdist) (.+?)(?: 0x([0-9a-f]+)| = (\d+))(?:\s+\((\d+)x\))?\s*$')

KEYS = {
    ('tperf', 'jogo (% da janela)'): 'busy',
    ('tperf', 'quadro medio (ms)'): 'ms',
    ('tperf', 'quadros de 3 vsyncs (37-55 ms) na janela'): 'v3',
    ('tperf', 'quadros de 4+ vsyncs (>55 ms) na janela'): 'v4',
    ('vprof', 'jogo'): 'jogo',
    ('vprof', 'zlive'): 'zlive',
    ('vprof', 'desenhos'): 'draws',
    ('vprof', 'quadros'): 'frames',
    ('tdist', 'entradas do terreno por quadro x100'): 'land',
}


def series(path):
    out = {}
    for line in open(path, errors='replace'):
        m = PAT.search(line)
        if not m:
            continue
        kind, key, hx, dec, rep = m.groups()
        k = KEYS.get((kind, key.strip()))
        if not k:
            continue
        v = int(hx, 16) if hx is not None else int(dec)
        out.setdefault(k, []).extend([v] * int(rep or 1))
    return out


def stage_slice(s):
    n = min(len(s.get('land', [])), len(s.get('busy', [])), len(s.get('draws', [])))
    if n == 0:
        return None
    land = s['land'][:n]
    idx = [i for i, v in enumerate(land) if v > 0]
    if len(idx) < 3:
        idx = list(range(int(n * 0.4), n))
    return {k: [v[i] for i in idx if i < len(v)] for k, v in s.items()}


def mean(x):
    return sum(x) / len(x) if x else float('nan')


rows = {}
for f in sorted(os.listdir(LOGS)):
    m = re.match(r'r(\d+)_p(\d+)\.log$', f)
    if not m:
        continue
    r, p = int(m.group(1)), int(m.group(2))
    s = series(os.path.join(LOGS, f))
    sl = stage_slice(s)
    rows.setdefault(r, {})[p] = (sl, len(s.get('land', [])))

pcts = sorted({p for d in rows.values() for p in d}, reverse=True)
print('| linha | pct | janelas | terreno/quadro | busy %% | ms/quadro | quadros 3+ vsync | desenhos | zlive us |')
print('|---|---|---|---|---|---|---|---|---|')
for r in sorted(rows):
    base = None
    for p in pcts:
        if p not in rows[r]:
            continue
        sl, nwin = rows[r][p]
        if not sl:
            print(f'| {r} | {p} | 0 | sem dados | | | | | |')
            continue
        n = len(sl.get('busy', []))
        v3 = [a + b for a, b in zip(sl.get('v3', []), sl.get('v4', []))]
        print('| %d | %d | %d | %.1f | %.1f | %.1f | %.1f | %.0f | %.0f |' % (
            r, p, n, mean(sl.get('land', [])) / 100.0, mean(sl.get('busy', [])), mean(sl.get('ms', [])),
            mean(v3), mean(sl.get('draws', [])), mean(sl.get('zlive', []))))
