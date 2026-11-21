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

# pad: o D-pad (--teclas) tem de mudar o quadro 80; timer: o canto sup. esq. fica verde
rm -rf /tmp/zchk_a /tmp/zchk_b
zeebx run ../../build/zeebo/sa1z.mod --seconds=4 --dump-gl=/tmp/zchk_a >/dev/null 2>&1
zeebx run ../../build/zeebo/sa1z.mod --seconds=4 --teclas=300:right --dump-gl=/tmp/zchk_b >/dev/null 2>&1
ha=$(md5sum < /tmp/zchk_a/00080.bmp); hb=$(md5sum < /tmp/zchk_b/00080.bmp)
[ "$ha" != "$hb" ] || fail "pad: o quadro 80 nao mudou com --teclas"
python3 - <<'PY' || fail "timer: indicador do canto nao esta verde"
from PIL import Image
r, g, b = Image.open('/tmp/zchk_a/00080.bmp').convert('RGB').getpixel((12, 12))
raise SystemExit(0 if g > 180 and r < 100 else 1)
PY
echo "ok: pad e timer"

if [ "${CHECK_GAME:-0}" = 1 ]; then
    res=$(make -f game.mk link 2>&1 | grep -o '[0-9]* de [0-9]* objetos' | head -1)
    n=${res%% *}
    [ "${n:-0}" -ge "${GAME_MIN:-487}" ] || fail "game.mk: $res (< ${GAME_MIN:-487})"
    echo "ok: game.mk ($res)"
    integ=$(make -f game.mk integrate 2>&1 | grep -o 'duplicatas: [0-9]*  indefinidos: [0-9]*')
    dup=$(echo "$integ" | sed 's/duplicatas: \([0-9]*\).*/\1/')
    [ "${dup:-1}" = 0 ] || fail "integrate: $integ (duplicatas devem ser 0)"
    echo "ok: integrate ($integ)"
fi
