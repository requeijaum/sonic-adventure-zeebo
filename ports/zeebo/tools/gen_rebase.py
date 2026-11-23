#!/usr/bin/env python3
"""Reescreve os endereços fixos de RAM do Dreamcast (0x0cXXXXXX / 0x8cXXXXXX)
do código do jogo para ZDC(...), que aponta para uma arena de 16 MB.

Uso: gen_rebase.py <raiz do decomp> <saida> arquivo.c [arquivo.h ...]
Só mexe em código (comentários ficam intactos) e só nos arquivos pedidos.
O repositório guarda só este gerador, não o texto do upstream."""
import re
import sys

LIT = re.compile(r'\b0x(?:0c|8c)[0-9a-fA-F]{6}\b')
TOKEN = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|' + LIT.pattern, re.S)


def rewrite(text):
    count = 0

    def sub(m):
        nonlocal count
        t = m.group(0)
        if LIT.fullmatch(t):
            count += 1
            return 'ZDC(%s)' % t
        return t
    return TOKEN.sub(sub, text), count


EVENT_OLD = '        event_module = EVENT_MODULE;\n'
EVENT_NEW = ('        /* zeebo: a cabeça ARM do evento sobre o binário SH-4 */\n'
             '        zevent_head(number, EVENT_MODULE);\n' + EVENT_OLD)


def event_hook(path):
    """event_load lê o evento do disco (SH-4) em EVENT_MODULE; o código
    ARM do evento está no .mod, e zevent_head (zprogs.c) põe a tabela dele
    por cima, como gen_patch.py faz com advertise e summary."""
    text = open(path, encoding='utf-8').read()
    if 'zevent_head' in text:
        return
    assert text.count(EVENT_OLD) == 1, path
    text = text.replace(EVENT_OLD, EVENT_NEW)
    text = text.replace('#include', 'void zevent_head(int number, void *module);\n#include', 1)
    open(path, 'w', encoding='utf-8').write(text)


SHT_OLD = '    shooting_scenes = *shooting_scene_file;\n'
SHT_NEW = ('    /* zeebo: ponteiros do DC de shtpath*.PRS viram enderecos ARM */\n'
           '    zshooting_fix(shooting_scene_file);\n' + SHT_OLD)


def shooting_hook(path):
    """shtpath1/2.PRS tem ponteiros absolutos do DC (a primeira palavra e cada\n    cena: path, begin, end); zshooting_fix (zrebase.c) os reloca depois da\n    leitura."""
    text = open(path, encoding='utf-8').read()
    if 'zshooting_fix' in text:
        return
    assert text.count(SHT_OLD) == 1, path
    text = text.replace(SHT_OLD, SHT_NEW)
    text = text.replace('#include', 'void zshooting_fix(void *file);\n#include', 1)
    open(path, 'w', encoding='utf-8').write(text)


def main():
    if sys.argv[1] == '--shooting-hook':
        shooting_hook(sys.argv[2])
        return
    if sys.argv[1] == '--event-hook':
        event_hook(sys.argv[2])
        return
    root, out, files = sys.argv[1], sys.argv[2], sys.argv[3:]
    import os
    total = 0
    for f in files:
        text = open(os.path.join(root, f), encoding='utf-8').read()
        new, n = rewrite(text)
        dst = os.path.join(out, f)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        open(dst, 'w', encoding='utf-8').write(new)
        total += n
        print('%s: %d' % (f, n))
    print('total: %d' % total)


if __name__ == '__main__':
    main()
