#!/bin/bash
# Abre o jogo (sa1g.mod) numa janela do zeebx para jogar.
#   ports/zeebo/play.sh            janela, até fechar (Esc)
#   ports/zeebo/play.sh --build    refaz applet + jogo antes
# Teclas: Z = A/Start, X ou Espaço = B, setas = direcional, Esc = fecha.
# O disco tem de estar em build/zeebo (tools/stage_disc.sh). O log vai para
# ~/.cache/sa1z-check/janela.log.
set -eu
cd "$(dirname "$0")"
ZEEBX=${ZEEBX:-$HOME/projects/zeebx-v060/target/release/zeebx}
if [ "${1:-}" = --build ]; then
    make >/dev/null
    make -f game.mk mod | tail -1
fi
[ -f ../../build/zeebo/sa1g.mod ] || { echo "sem build/zeebo/sa1g.mod: rode com --build"; exit 1; }
[ -e ../../build/zeebo/masaru.drv ] || { echo "sem o disco em build/zeebo: tools/stage_disc.sh"; exit 1; }
mkdir -p "$HOME/.cache/sa1z-check"
exec "$ZEEBX" run ../../build/zeebo/sa1g.mod --window --seconds=36000 \
    > "$HOME/.cache/sa1z-check/janela.log" 2>&1
