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
- sdk/include/shinobi.h: syCacheICI chamava um endereço P2 do SH-4; vira no-op.
- engine/native/mesh.c: P32 marca ponteiro de 32 bits; em ARM já é.
"""
import os
import re
import sys

# os arquivos que game.mk compila com -DDREAMCAST (ver o roadmap, opção C)
DC_SET = ['sdk/ninja/njDraw.c', 'sdk/ninja/njDrawLine2D.c',
          'sdk/ninja/njDrawLine3D.c', 'sdk/ninja/njDrawPolygon2D.c', 'sdk/ninja/njDebug.c',
          'engine/dc/sprite.c']

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
    'src/system/timer.c': [
        ('#ifdef NONMATCHING\n#define TSTR (*(volatile Uint8 *)0xffd80004)\n'
         '#define TCOR1 (*(volatile Uint32 *)0xffd80014)\n'
         '#define TCNT1 (*(volatile Uint32 *)0xffd80018)\n'
         '#define TCR1 (*(volatile Uint16 *)0xffd8001c)\n#else',
         '#ifdef NONMATCHING\n/* zeebo: sem TMU; o contador 1 vem de syTmrGetCount (zsys.c) */\n'
         'extern Uint8 ztmu_tstr;\nextern Uint32 ztmu_tcor1;\nextern Uint16 ztmu_tcr1;\n'
         'Uint32 ztmu_tcnt1(void);\n'
         '#define TSTR ztmu_tstr\n#define TCOR1 ztmu_tcor1\n'
         '#define TCNT1 (*ztmu_tcnt1_ref())\n#define TCR1 ztmu_tcr1\n'
         'Uint32 *ztmu_tcnt1_ref(void);\n#else')],
    'src/main.c': [
        # ADVERTISE linkado no .mod (game.mk PROGRAMS): o binário SH-4 lido do
        # disco fica em MODULE_AREA, e a tabela de entrada ARM vai por cima
        ('        loader_read_prs("advertise.prs", ADVERTISE);\n',
         '        loader_read_prs("advertise.prs", ADVERTISE);\n'
         '        *ADVERTISE = advertise_entries;\n'),
        # idem para SUMMARY (resumo da história e tutorial), nas três leituras
        ('        loader_read_prs("summary.prs", SUMMARY);\n',
         '        loader_read_prs("summary.prs", SUMMARY);\n'
         '        *SUMMARY = summary_entries;\n')],
    'sdk/ninja/njSystem.c': [
        ('    while (blanks == _nj_blank_count_) {\n    }\n',
         '    while (blanks == _nj_blank_count_) {\n        native_vsync();\n    }\n'),
        ('    while (!(SYNC_STATUS & SYNC_VERTICAL)) {\n    }\n    while (SYNC_STATUS & SYNC_VERTICAL) {\n    }\n',
         '    native_vsync();\n')],
    'sdk/include/shinobi.h': [(
        '#define syCacheICI() ((void (*)(void))(0xa0000000 | (Uint32)syCacheICI_P2_))()',
        '#define syCacheICI() ((void)0)')],
    'engine/native/mesh.c': [(
        '#define P32 __ptr32 __uptr\n', '#define P32\n')],
}


def main():
    root, out = sys.argv[1], sys.argv[2]
    # arquivos de desenho no sabor Dreamcast (game.mk DC_SET): os rótulos
    # __asm__("_nome") são do SHC (prefixo _); no ARM o símbolo C não tem
    for f in DC_SET:
        dst = os.path.join(out, f)
        src_text = open(os.path.join(root, f)).read()
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        open(dst, 'w').write(re.sub(r'__asm__\("_([A-Za-z_])', r'__asm__("\1', src_text))
    for f, edits in {'src/collision/polygon.c': [], 'src/system/frame.c': [], 'sdk/ninja/njSystem.c': [], 'src/main.c': [], 'src/system/timer.c': [], **PATCHES}.items():
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
            # um trecho que aparece várias vezes (as leituras do SUMMARY) troca em todas
            text = text.replace(old, new)
        dst = os.path.join(out, f)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        open(dst, 'w', encoding='utf-8').write(text)
        print(f)


if __name__ == '__main__':
    main()
