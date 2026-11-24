#!/bin/bash
# Guarda o build atual para comparar depois: o .mod MAIS os .ovl que o acompanham.
# Os .ovl (dados dos programas) sao reescritos a cada build em build/zeebo e
# so servem ao .mod que os gerou: um .mod antigo com o .ovl de um build novo
# reloca ponteiros no lugar errado em silencio (nomes de arquivo viram lixo e o
# jogo para em "TRAP OPEN"). Nunca copie so o sa1g.mod.
#   tools/keep_build.sh NOME      -> build/zeebo/keep/NOME/{sa1g.mod,*.ovl}
# rodar:  zeebx run build/zeebo/keep/NOME/sa1g.mod ...
set -eu
[ $# -eq 1 ] || { echo "uso: $0 NOME"; exit 2; }
B=$(dirname "$0")/../../../build/zeebo
D=$B/keep/$1
mkdir -p "$D"
cp "$B/sa1g.mod" "$D/sa1g.mod"
cp "$B"/*.ovl "$D"/
# o jogo le os dados do disco da pasta do .mod: o resto vai por hardlink (o zeebx
# recusa link simbolico que sai da pasta); nao ocupa espaco e nada e apagado
for f in "$B"/*; do
    [ -f "$f" ] || continue
    case "$f" in
    *.mod|*.ovl|*.o|*.elf|*.bmp|*.log|*.err|*.map) continue ;;
    esac
    ln -f "$f" "$D/$(basename "$f")" 2>/dev/null || cp -p "$f" "$D/$(basename "$f")"
done
echo "guardado: $D ($(ls "$D" | wc -l) arquivos)"
