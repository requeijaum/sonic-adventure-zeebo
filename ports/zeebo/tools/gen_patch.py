#!/usr/bin/env python3
"""Cópias remendadas de três fontes do decomp para ARM32.

Uso: gen_patch.py <raiz do decomp> <saida>
Cada remendo troca um trecho exato; se o trecho sumir (upstream mudou), falha.
O repositório guarda só este gerador, não o texto do upstream.

- src/collision/polygon.c: 9 guardas NONMATCHING só valem no console; usa-se
  marks.console() do próprio decomp para tomar o lado enviado do jogo.
- src/system/frame.c: as esperas de vsync chamam native_vsync() (SITES do decomp).
- src/save/card.c: o alias de light_ry/light_rz era asm estendido no escopo do
  arquivo com a sintaxe do SH-4; vira asm básico com .set de ARM.
- build/engine/native/asm.c: log de depuração em native_jsr e f__njDrawPolygon.
- sdk/include/shinobi.h: syCacheICI chamava um endereço P2 do SH-4; vira no-op.
- engine/native/mesh.c: P32 marca ponteiro de 32 bits; em ARM já é.
"""
import os
import sys

PATCHES = {
    'src/save/card.c': [(
        '__asm__("\\t.global\\t_light_ry\\n"\n'
        '        "\\t.type\\t_light_ry, @object\\n"\n'
        '        "\\t.size\\t_light_ry, 4\\n"\n'
        '        "\\t.set\\t_light_ry, %a0\\n"\n'
        '        "\\t.global\\t_light_rz\\n"\n'
        '        "\\t.type\\t_light_rz, @object\\n"\n'
        '        "\\t.size\\t_light_rz, 4\\n"\n'
        '        "\\t.set\\t_light_rz, %a1\\n"\n'
        '        :\n'
        '        : "i"(&light_ry), "i"(&light_rz));\n',
        '/* light_ry fica em 0x5a0 de sources_file (static assert acima), light_rz 4\n'
        ' * bytes depois; ARM não prefixa o nome com sublinhado */\n'
        '__asm__("\\t.global\\tlight_ry\\n"\n'
        '        "\\t.set\\tlight_ry, sources_file+0x5a0\\n"\n'
        '        "\\t.global\\tlight_rz\\n"\n'
        '        "\\t.set\\tlight_rz, sources_file+0x5a4\\n");\n')],
    'sdk/ninja/njSystem.c': [
        ('    while (blanks == _nj_blank_count_) {\n    }\n',
         '    while (blanks == _nj_blank_count_) {\n        native_vsync();\n    }\n'),
        ('    while (!(SYNC_STATUS & SYNC_VERTICAL)) {\n    }\n    while (SYNC_STATUS & SYNC_VERTICAL) {\n    }\n',
         '    native_vsync();\n')],
    'build/engine/native/asm.c': [
        ('#include "sh4.h"\n', '#include "sh4.h"\nextern void zlog(const char *, const char *, int);\n'),
        ('void native_jsr(uint32_t target)\n{\n',
         'void native_jsr(uint32_t target)\n{\n'
         '    zlog("jsr", "alvo", (int)target);\n'
         '    zlog("jsr", "lr", (int)__builtin_return_address(0));\n'),
        ('static void f__njDrawPolygon(void)\n{\n',
         'static void f__njDrawPolygon(void)\n{\n'
         '    zlog("jsr", "ENTRA f__njDrawPolygon lr", (int)__builtin_return_address(0));\n')],
    'sdk/include/shinobi.h': [(
        '#define syCacheICI() ((void (*)(void))(0xa0000000 | (Uint32)syCacheICI_P2_))()',
        '#define syCacheICI() ((void)0)')],
    'engine/native/mesh.c': [(
        '#define P32 __ptr32 __uptr\n', '#define P32\n')],
}


def main():
    root, out = sys.argv[1], sys.argv[2]
    for f, edits in {'src/collision/polygon.c': [], 'src/system/frame.c': [], 'sdk/ninja/njSystem.c': [], **PATCHES}.items():
        text = open(os.path.join(root, f), encoding='utf-8').read()
        if f == 'src/system/frame.c':
            # os esperas de vsync do jogo giram em frame_countdown: o decomp as
            # troca por native_vsync() (SITES de engine/tools/marks.py)
            sys.path.insert(0, os.path.join(root, 'engine', 'tools'))
            import marks
            edits = list(edits) + list(marks.SITES[f])
        if f == 'src/collision/polygon.c':
            # os guardas só-console, do lado enviado do jogo, como o slice nativo
            sys.path.insert(0, os.path.join(root, 'engine', 'tools'))
            import marks
            text = marks.console(f, text)
        for old, new in edits:
            if old not in text:
                sys.exit('remendo de %s não casa: o upstream mudou' % f)
            text = text.replace(old, new, 1)
        dst = os.path.join(out, f)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        open(dst, 'w', encoding='utf-8').write(text)
        print(f)


if __name__ == '__main__':
    main()
