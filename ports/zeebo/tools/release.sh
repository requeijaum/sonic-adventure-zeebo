#!/bin/bash
# Monta o jogo na pasta de ROMs do zeebx no layout do console, que e o que da nome e capa
# na biblioteca (o .mif nao guarda nome; o titulo e a pasta, a capa e a imagem ao lado do .mod):
#   <DEST>/<Titulo>/mif/<id>.mif
#   <DEST>/<Titulo>/mod/<id>/sa1g.mod, sa1g.png (capa), *.ovl e os arquivos do disco
#
#   tools/release.sh [DEST] [CAPA.png]
#     DEST  padrao: pasta de ROMs do zeebx (roms_dir em ~/.config/zeebx/settings.json)
#     CAPA  padrao: ports/zeebo/assets/cover.png, se existir
# Constroi antes com `AUDIO=1 tools/build.sh` (titulo normal, som ligado). .mod, .mif e .ovl sao
# copias (um rebuild nao muda a release); os arquivos do disco sao hard links (o zeebx 0.6.0
# ignora symlinks), entao a pasta de destino precisa estar no mesmo sistema de arquivos.
set -eu
cd "$(dirname "$0")/../../.."
B=build/zeebo
TITLE=${TITLE:-Sonic Adventure 1 (Zeebo Port)}
ID=${ID:-199258115} # ClassID 0x0be07003 em decimal
DEST=${1:-$(python3 -c "import json,os;print(json.load(open(os.path.expanduser('~/.config/zeebx/settings.json')))['roms_dir'])")}
COVER=${2:-ports/zeebo/assets/cover.png}
[ -f "$B/sa1g.mod" ] && [ -f "$B/sa1g.mif" ] || { echo "FALHOU: sem $B/sa1g.mod/.mif (rode AUDIO=1 tools/build.sh)"; exit 1; }
D="$DEST/$TITLE"
mkdir -p "$D/mif" "$D/mod/$ID"
n=0
for f in "$B"/*; do
    [ -f "$f" ] || continue
    b=$(basename "$f")
    case "$b" in
    *.o | *.c | *.elf | *.lock | *.flags | *.bmp | *.png | *.html | *.htm | *.psd | ninja_selftest | sa1vmu.bin | sa1z* | sa1g_*.mod | sa1g.mif) continue ;;
    sa1g.mod | *.ovl) rm -f "$D/mod/$ID/$b"; cp -f "$f" "$D/mod/$ID/$b" ;;
    *) ln -f "$f" "$D/mod/$ID/$b" 2>/dev/null || cp -f "$f" "$D/mod/$ID/$b" ;;
    esac
    n=$((n + 1))
done
cp -f "$B/sa1g.mif" "$D/mif/$ID.mif"
[ -f "$COVER" ] && cp -f "$COVER" "$D/mod/$ID/sa1g.png"
echo "release: $D ($n arquivos, id $ID)"
