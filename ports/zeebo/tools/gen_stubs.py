#!/usr/bin/env python3
"""Gera stubs C a partir dos protótipos dos headers do SDK do decomp.

Uso: gen_stubs.py <saida.c> <nomes.txt> <header> [<header> ...]

Para cada nome, acha o protótipo `ret nome(args);` nos headers e emite uma
definição que ignora os argumentos e devolve 0 (ou nada, se void). Funções
que devolvem um handle por parâmetro (`SDMEMBLK *h`, `SDMIDI *h`, ...) recebem
um objeto estático não nulo. O repositório guarda só este gerador: o texto dos
headers do upstream não é copiado.
"""
import re
import sys

HANDLE_OUT = re.compile(r'\b(SDMEMBLK|SDMIDI|SDPSTM|SDHANDLE)\s*\*\s*(\w+)')


def find_proto(name, text):
    m = re.search(r'(?m)^[ \t]*([A-Za-z_][\w \t\*]*?)\b' + re.escape(name) + r'\s*\(([^;{]*?)\)\s*;', text)
    if not m:
        return None
    ret = ' '.join(m.group(1).split())
    args = ' '.join(m.group(2).split())
    return ret, args


def param_names(args):
    names = []
    for part in args.split(','):
        part = part.strip()
        if not part or part == 'void' or part == '...':
            continue
        m = re.search(r'(\w+)(\[\d*\])?$', part)
        if m and not re.fullmatch(r'(void|char|int|float|Float|Uint8|Sint8|Uint16|Sint16|Uint32|Sint32|Int)', m.group(1)):
            names.append(m.group(1))
    return names


def main():
    out, names_file, headers = sys.argv[1], sys.argv[2], sys.argv[3:]
    text = '\n'.join(open(h, encoding='utf-8', errors='replace').read() for h in headers)
    names = [n.strip() for n in open(names_file) if n.strip()]
    found, missing = [], []
    import os
    incs = ['#include <%s>' % os.path.basename(h) for h in headers
            if os.path.basename(h) not in ('shinobi.h',)]
    body = ['/* gerado por tools/gen_stubs.py: não editar */',
            '#include <kamui.h>', '#include <shinobi.h>'] + incs + ['',
            'static unsigned long zstub_handle[8];', 'void zlog(const char *what, const char *name, int value);', '']
    for n in names:
        p = find_proto(n, text)
        if not p:
            missing.append(n)
            continue
        ret, args = p
        lines = ['    zlog("stub", "%s", (int)__builtin_return_address(0));' % n]
        for h in HANDLE_OUT.finditer(args):
            lines.append('    if (%s != 0) {\n        *%s = (void *)zstub_handle;\n    }' % (h.group(2), h.group(2)))
        for pn in param_names(args):
            lines.append('    (void)%s;' % pn)
        if ret.split()[-1] != 'void' or '*' in ret:
            lines.append('    return 0;')
        body.append('%s %s(%s)\n{\n%s\n}\n' % (ret, n, args, '\n'.join(lines)))
        found.append(n)
    open(out, 'w', encoding='utf-8').write('\n'.join(body))
    print('stubs: %d  sem protótipo: %s' % (len(found), ' '.join(missing)))


if __name__ == '__main__':
    main()
