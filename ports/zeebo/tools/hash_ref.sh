#!/bin/bash
# Gate de desenho contra uma referencia versionada. `check` roda o sa1g.mod atual e
# compara os "estado hash" (hash rolante do que o jogo manda ao GL, a cada 64 quadros)
# com tools/estado_hash.ref; `update` regrava a referencia. Entrada fixa (KEYS) e
# --seconds=100: a sequencia e deterministica. Nao cobre logica fora do desenho.
set -eu
D=$(cd "$(dirname "$0")" && pwd)
R=$D/../../..
Z=${ZEEBX:-$HOME/projects/zeebx-emu-copytex/target/release/zeebx}
K=${KEYS:-14000:down,14500:down,15000:down,20000:b1}
cur() {
    (cd "$(mktemp -d)" && timeout 300 "$Z" run "$R/build/zeebo/sa1g.mod" --seconds=100 --keys="$K" 2>&1 |
        grep -aE 'estado hash|frames presented' | sed 's/^ *//')
}
case "${1:-check}" in
update) cur > "$D/estado_hash.ref"; wc -l "$D/estado_hash.ref" ;;
check)
    if diff <(cur) "$D/estado_hash.ref" > /dev/null; then echo "hash_ref: IGUAL"; else echo "hash_ref: DIFERENTE" >&2; exit 1; fi ;;
*) echo "uso: hash_ref.sh [check|update]" >&2; exit 2 ;;
esac
