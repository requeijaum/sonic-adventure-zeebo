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
FLAGS="STATECACHE=${STATECACHE:-} HOTOPT=${HOTOPT:-} DYNRES=${DYNRES:-} FPSHUD=${FPSHUD:-} PROF=${PROF:-} AUDIO=${AUDIO:-} OPT=${OPT:--O2 -fno-tree-vectorize} HOTFILES=${HOTFILES:-} TEX16=${TEX16:-} MENUFPS30=${MENUFPS30:-}"
if [ "$(cat build/zeebo/.flags 2>/dev/null)" != "$FLAGS" ]; then
    echo "build.sh: flags mudaram ($FLAGS); recompilando os .c que as leem" >&2
    touch ports/zeebo/zstrip.c ports/zeebo/zkamui.c ports/zeebo/app.c ports/zeebo/zhud.c \
        ports/zeebo/ztexture.c ports/zeebo/zthreads.c ports/zeebo/zfile.c ports/zeebo/zsys.c \
        ports/zeebo/zgdfs.c ports/zeebo/zrebase.c ports/zeebo/zaudio.c ports/zeebo/zlive.c \
        ports/zeebo/platform/brew_egl.c
    echo "$FLAGS" > build/zeebo/.flags
fi
# Mudar o zrebase.h (o mapa da arena, ZDC) muda constantes de todo objeto. Ja sobraram objetos
# com o mapa antigo apos uma mudanca (o jogo caiu longe da causa: leitura 32 KB abaixo da arena);
# a causa nao foi isolada (os .d citam o cabecalho). Objeto mais velho que ele = para.
if [ -z "${ZREBASE_OK:-}" ]; then
    OLD=$(find build/zeebo/game/obj -name '*.o' ! -newer ports/zeebo/zrebase.h 2>/dev/null | head -1)
    if [ -n "$OLD" ]; then
        echo "build.sh: $OLD e mais velho que ports/zeebo/zrebase.h; o mapa ZDC mudou e o build incremental" >&2
        echo "  deixaria objetos com o mapa antigo. Rebuild limpo (apagar build/zeebo/game/obj/**/*.o, exige" >&2
        echo "  aprovacao do usuario) ou ZREBASE_OK=1 para ignorar." >&2
        exit 3
    fi
fi
# DEBUG_TITLE=N entra por argumento do make e so afeta title.o e main.o, que o make nao refaz
# ao mudar o valor. build/zeebo/.debug_title guarda o valor da ultima build que terminou bem:
# mudou => refaz esses dois; hash_ref.sh o le para recusar um .mod que nao seja DEBUG_TITLE=0.
DT=
for a in "$@"; do case "$a" in DEBUG_TITLE=*) DT=${a#DEBUG_TITLE=} ;; esac; done
STAMP=build/zeebo/.debug_title
if [ -f "$STAMP" ] && [ "$(cat "$STAMP")" != "DEBUG_TITLE=$DT" ]; then
    echo "build.sh: DEBUG_TITLE mudou ($(cat "$STAMP") -> DEBUG_TITLE=$DT); refazendo title.o e main.o" >&2
    touch src/progress/title.c src/main.c
fi
make -C ports/zeebo
make -C ports/zeebo -f game.mk mod "$@"
echo "DEBUG_TITLE=$DT" > "$STAMP"
