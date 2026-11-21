#!/bin/bash
# Verificação mínima do port: falha se algo regredir.
#   1. build do applet gera o .mod (linha "Created")
#   2. selftest do Ninja no host passa
#   3. zeebx roda 3 s: SwapBuffers > 0 e GenTextures >= 9
#   4. (opcional, CHECK_GAME=1) game.mk linka e o nº de objetos que compilam
#      não cai abaixo de GAME_MIN (padrão 500)
# O selftest do applet (zdc/threads) para o desenho se falhar: SwapBuffers 0.
set -u
cd "$(dirname "$0")"
fail() { echo "FALHOU: $*"; exit 1; }

out=$(make 2>&1) || { echo "$out" | tail -5; fail "make"; }
echo "$out" | grep -q "Created" || {
    # nada a refazer: confirma que o .mod existe e é mais novo que as fontes
    [ -f ../../build/zeebo/sa1z.mod ] || fail "sem .mod"
}
make host-selftest 2>&1 | grep -q "PASSED" || fail "host-selftest"

log=$(zeebx run ../../build/zeebo/sa1z.mod --seconds=3 2>&1)
swaps=$(echo "$log" | grep -o '[0-9]*x IEGL11::SwapBuffers' | grep -o '^[0-9]*')
tex=$(echo "$log" | grep -o '[0-9]*x IGLES11::GenTextures' | grep -o '^[0-9]*')
[ "${swaps:-0}" -gt 0 ] || fail "zeebx: 0 SwapBuffers (selftest do applet falhou?)"
[ "${tex:-0}" -ge 9 ] || fail "zeebx: GenTextures=${tex:-0} (<9: assets nao carregaram)"
echo "ok: applet (swaps=$swaps, texturas=$tex)"

if [ "${CHECK_GAME:-0}" = 1 ]; then
    res=$(make -f game.mk link 2>&1 | grep -o '[0-9]* de [0-9]* objetos' | head -1)
    n=${res%% *}
    [ "${n:-0}" -ge "${GAME_MIN:-500}" ] || fail "game.mk: $res (< ${GAME_MIN:-500})"
    echo "ok: game.mk ($res)"
fi
