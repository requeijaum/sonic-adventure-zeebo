#!/bin/bash
# Build serializado: duas sessoes rodando `make` na mesma arvore corrompem os
# .o intermediarios (ja houve full.o vazio por isso). Este wrapper pega um
# flock e espera a outra terminar.
#   ports/zeebo/tools/build.sh [argumentos do make de game.mk, ex.: DEBUG_TITLE=0]
set -eu
cd "$(dirname "$0")/../../.."
mkdir -p build/zeebo
exec 9>build/zeebo/.build.lock
if ! flock -n 9; then
    echo "build.sh: outro build esta rodando; esperando..." >&2
    flock -w 1800 9
fi
make -C ports/zeebo
make -C ports/zeebo -f game.mk mod "$@"
