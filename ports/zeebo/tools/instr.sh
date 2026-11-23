#!/bin/bash
# instr.sh [MOD] — milhões de instruções ARM numa rodada de quadros fixos do
# zeebx (determinístico: serve para comparar otimizações sem depender do tempo)
# SECONDS_RUN (padrão 40) KEYS (padrão: menu de debug -> fase)
MOD=${1:-$HOME/projects/sonic-adventure-zeebo/build/zeebo/sa1g.mod}
ZEEBX=${ZEEBX:-$HOME/projects/zeebx-v060/target/release/zeebx}
( ulimit -v 3000000
  timeout 900 nice -n 19 "$ZEEBX" run "$MOD" --seconds="${SECONDS_RUN:-40}" \
      --keys="${KEYS:-14000:b1,17000:b1}" 2>&1 | grep -a 'million instructions' )
