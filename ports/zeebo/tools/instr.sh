#!/bin/bash
# instr.sh [MOD] — milhões de instruções ARM numa rodada de quadros fixos do
# zeebx (determinístico: serve para comparar otimizações sem depender do tempo)
# SECONDS_RUN (padrão 40) KEYS (padrão: menu de debug -> fase)
D=$(cd "$(dirname "$0")" && pwd)
_E=$(cd "$(dirname "$0")" && pwd)/
[ -f "$_E/env.sh" ] && . "$_E/env.sh" || . "$_E/env.sh.example"
MOD=${1:-$D/../../../build/zeebo/sa1g.mod}
ZEEBX=${ZEEBX:-$ZEEBX_V060}
( ulimit -v 3000000
  timeout 900 nice -n 19 "$ZEEBX" run "$MOD" --seconds="${SECONDS_RUN:-40}" \
      --keys="${KEYS:-14000:b1,17000:b1}" 2>&1 | grep -a 'million instructions' )
