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
# O gate so vale com um sa1g.mod DEBUG_TITLE=0 (outra build da outra cena e da "DIFERENTE").
# tools/build.sh grava build/zeebo/.debug_title ao fim de uma build boa. Sem o arquivo, ou com
# o .mod mais novo que ele (feito por fora do build.sh), nao da para saber: avisa e segue;
# HASH_REF_STRICT=1 transforma o aviso em erro.
guard() {
    local s=$R/build/zeebo/.debug_title m=$R/build/zeebo/sa1g.mod
    if [ ! -f "$s" ] || [ "$m" -nt "$s" ]; then
        echo "hash_ref: nao da para confirmar que o sa1g.mod e DEBUG_TITLE=0 (sem carimbo valido; rebuild com tools/build.sh DEBUG_TITLE=0)" >&2
        [ -z "${HASH_REF_STRICT:-}" ] || exit 4
    elif [ "$(cat "$s")" != "DEBUG_TITLE=0" ]; then
        echo "hash_ref: o sa1g.mod foi feito com $(cat "$s"); o gate exige DEBUG_TITLE=0. Rebuild: tools/build.sh DEBUG_TITLE=0 (depois o build normal)." >&2
        exit 4
    fi
}
cur() {
    local d out
    d=$(mktemp -d)
    out=$(cd "$d" && timeout 300 "$Z" run "$R/build/zeebo/sa1g.mod" --seconds=100 --keys="$K" 2>&1 |
        grep -aE 'estado hash|frames presented' | sed 's/^ *//') || true
    rm -rf "$d"
    printf '%s\n' "$out"
}
case "${1:-check}" in
update) guard; cur > "$D/estado_hash.ref"; wc -l "$D/estado_hash.ref" ;;
check)
    guard
    if diff <(cur) "$D/estado_hash.ref" > /dev/null; then echo "hash_ref: IGUAL"; else echo "hash_ref: DIFERENTE" >&2; exit 1; fi ;;
*) echo "uso: hash_ref.sh [check|update]" >&2; exit 2 ;;
esac
