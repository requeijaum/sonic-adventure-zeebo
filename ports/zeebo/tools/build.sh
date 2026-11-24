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
# O make nao rastreia flags passadas por ambiente: um build com STATECACHE=0 deixava o
# zstrip.o antigo no build seguinte sem flag (ja aconteceu). Guarda o conjunto usado e,
# se mudou, forca a recompilacao dos .c do applet que leem essas flags.
FLAGS="STATECACHE=${STATECACHE:-} HOTOPT=${HOTOPT:-} DYNRES=${DYNRES:-} FPSHUD=${FPSHUD:-} PROF=${PROF:-} AUDIO=${AUDIO:-} OPT=${OPT:--O2 -fno-tree-vectorize} HOTFILES=${HOTFILES:-} TEX16=${TEX16:-}"
if [ "$(cat build/zeebo/.flags 2>/dev/null)" != "$FLAGS" ]; then
    echo "build.sh: flags mudaram ($FLAGS); recompilando os .c que as leem" >&2
    touch ports/zeebo/zstrip.c ports/zeebo/zkamui.c ports/zeebo/app.c ports/zeebo/zhud.c \
        ports/zeebo/ztexture.c ports/zeebo/zthreads.c ports/zeebo/zfile.c ports/zeebo/zsys.c \
        ports/zeebo/zgdfs.c ports/zeebo/zrebase.c ports/zeebo/zaudio.c ports/zeebo/zlive.c \
        ports/zeebo/platform/brew_egl.c
    echo "$FLAGS" > build/zeebo/.flags
fi
make -C ports/zeebo
make -C ports/zeebo -f game.mk mod "$@"
