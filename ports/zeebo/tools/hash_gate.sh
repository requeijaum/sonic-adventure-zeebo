#!/bin/bash
# Gate de otimizacao: dois builds guardados (tools/keep_build.sh) com a mesma entrada
# tem de dar o mesmo "estado hash" (hash rolante do que o jogo mandou desenhar, a cada
# 64 quadros) e o mesmo numero de quadros. Uso: hash_gate.sh NOME_A NOME_B [zeebx]
set -eu
B=$(dirname "$0")/../../../build/zeebo/keep
_E=$(cd "$(dirname "$0")" && pwd)/
[ -f "$_E/env.sh" ] && . "$_E/env.sh" || . "$_E/env.sh.example"
Z=${3:-$ZEEBX_COPYTEX}
K=${KEYS:-14000:down,14500:down,15000:down,20000:b1}
sum() {
    timeout 300 "$Z" run "$B/$1/sa1g.mod" --seconds="${SECONDS_RUN:-100}" --keys="$K" 2>&1 |
        grep -aE 'estado hash|frames presented' | sed 's/^ *//'
}
A=$(sum "$1"); C=$(sum "$2")
if [ "$A" = "$C" ] && [ -n "$A" ]; then
    echo "hash_gate: IGUAL ($(echo "$A" | grep -c 'estado hash') pontos)"
else
    echo "hash_gate: DIFERENTE"; diff <(echo "$A") <(echo "$C") | head -6; exit 1
fi
