#!/bin/bash
# Para cada linha do menu de debug (src/progress/title.c title_debug_stages),
# reconstroi o .mod com DEBUG_TITLE=N, roda no zeebx e guarda o ultimo quadro e
# as linhas de log que importam em build/zeebo/matrix/. Resumo em summary.txt.
#   tools/stage_matrix.sh 3 6 9 ...        (sem argumentos: as 29 linhas padrao)
# Depois rode `make -f game.mk mod` sem DEBUG_TITLE para voltar ao boot normal.
set -u
cd "$(dirname "$0")/.."
ROOT=$(cd ../.. && pwd)
OUTD=$ROOT/build/zeebo/matrix
mkdir -p "$OUTD"
ROWS=("$@")
[ ${#ROWS[@]} -eq 0 ] && ROWS=(3 6 9 12 15 18 21 24 28 32 35 36 37 38 39 40 46 52 58 62 65 66 67 68 69 70 71 72 0)
for r in "${ROWS[@]}"; do
    rm -f "$ROOT/build/zeebo/game/obj/src/progress/title.o" "$ROOT/build/zeebo/game/obj/src/main.o"
    b=$(make -f game.mk mod DEBUG_TITLE=$r 2>&1 | grep -E 'objetos')
    SECONDS_RUN=${SECONDS_RUN:-75} KEYS=${KEYS:-14000:b1,17000:b1} \
        LOGPAT='TRAP|trap |programa ausente|vbind|HOT|fim do jogo|modo' LOGN=80 \
        KEEP="$OUTD/row$r.png" tools/frame_hash.sh > "$OUTD/row$r.log" 2>&1
    h=$(grep -E 'hash' "$OUTD/row$r.log" | tail -1)
    echo "row $r | $b | $h | traps=$(grep -ciE 'trap ' "$OUTD/row$r.log") ausente=$(grep -c 'programa ausente' "$OUTD/row$r.log")" | tee -a "$OUTD/summary.txt"
done
