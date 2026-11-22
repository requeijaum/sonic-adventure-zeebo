#!/bin/bash
# Verificação mínima do port: falha se algo regredir.
#   1. build do applet gera o .mod (linha "Created")
#   2. selftest do Ninja no host passa
#   3. zeebx roda 3 s: SwapBuffers > 0 e GenTextures >= 9
#   4. (opcional, CHECK_GAME=1) game.mk linka e o nº de objetos que compilam
#      não cai abaixo de GAME_MIN; integrate sem duplicatas; o jogo chega ao
#      menu principal com o botão 1 (game_mode 11, ava_vmssel.PRS: o cartao emulado e visto) e >= 300 quadros em 30 s
# O selftest do applet (zdc/threads) para o desenho se falhar: SwapBuffers 0.
set -u
cd "$(dirname "$0")"
# zeebx v0.6.0 (compilado da tag upstream); ZEEBX=... troca
ZEEBX=${ZEEBX:-$HOME/projects/zeebx-v060/target/release/zeebx}
[ -x "$ZEEBX" ] || ZEEBX=zeebx
# dumps fora do /tmp (tmpfs pequeno)
TMPD=${XDG_CACHE_HOME:-$HOME/.cache}/sa1z-check
mkdir -p "$TMPD"
fail() { echo "FALHOU: $*"; exit 1; }

out=$(make 2>&1) || { echo "$out" | tail -5; fail "make"; }
echo "$out" | grep -q "Created" || {
    # nada a refazer: confirma que o .mod existe e é mais novo que as fontes
    [ -f ../../build/zeebo/sa1z.mod ] || fail "sem .mod"
}
make host-selftest 2>&1 | grep -q "PASSED" || fail "host-selftest"

log=$("$ZEEBX" run ../../build/zeebo/sa1z.mod --seconds=3 2>&1)
swaps=$(echo "$log" | grep -o '[0-9]*x IEGL11::SwapBuffers' | grep -o '^[0-9]*')
tex=$(echo "$log" | grep -o '[0-9]*x IGLES11::GenTextures' | grep -o '^[0-9]*')
[ "${swaps:-0}" -gt 0 ] || fail "zeebx: 0 SwapBuffers (selftest do applet falhou?)"
[ "${tex:-0}" -ge 9 ] || fail "zeebx: GenTextures=${tex:-0} (<9: assets nao carregaram)"
echo "ok: applet (swaps=$swaps, texturas=$tex)"

# pad: o D-pad (--teclas) tem de mudar o quadro 80; timer: o canto sup. esq. fica verde
rm -rf "$TMPD/a" "$TMPD/b"
# v0.6.0: --key-names (antes --teclas)
KEYS=--key-names; "$ZEEBX" 2>&1 | grep -q -- --key-names || KEYS=--teclas
"$ZEEBX" run ../../build/zeebo/sa1z.mod --seconds=4 --dump-gl="$TMPD/a" >/dev/null 2>&1
"$ZEEBX" run ../../build/zeebo/sa1z.mod --seconds=4 $KEYS=300:right --dump-gl="$TMPD/b" >/dev/null 2>&1
ha=$(md5sum < "$TMPD/a/00080.bmp"); hb=$(md5sum < "$TMPD/b/00080.bmp")
[ "$ha" != "$hb" ] || fail "pad: o quadro 80 nao mudou com --teclas"
TMPD="$TMPD" python3 - <<'PY' || fail "timer: indicador do canto nao esta verde"
from PIL import Image
r, g, b = Image.open(__import__('os').environ['TMPD'] + '/a/00080.bmp').convert('RGB').getpixel((12, 12))
raise SystemExit(0 if g > 180 and r < 100 else 1)
PY
rm -rf "$TMPD/a" "$TMPD/b"
echo "ok: pad e timer"

if [ "${CHECK_GAME:-0}" = 1 ]; then
    res=$(make -f game.mk link 2>&1 | grep -o '[0-9]* de [0-9]* objetos' | head -1)
    n=${res%% *}
    [ "${n:-0}" -ge "${GAME_MIN:-489}" ] || fail "game.mk: $res (< ${GAME_MIN:-489})"
    echo "ok: game.mk ($res)"
    integ=$(make -f game.mk integrate 2>&1 | grep -o 'duplicatas: [0-9]*  indefinidos: [0-9]*')
    dup=$(echo "$integ" | sed 's/duplicatas: \([0-9]*\).*/\1/')
    [ "${dup:-1}" = 0 ] || fail "integrate: $integ (duplicatas devem ser 0)"
    echo "ok: integrate ($integ)"
    # o jogo roda até o menu principal: botão 1 passa
    # o título e entra no menu (game_mode 11, MENU_RUN; texturas do menu)
    make -f game.mk mod >/dev/null 2>&1 || fail "game.mk mod"
    glog=$(timeout 300 "$ZEEBX" run ../../build/zeebo/sa1g.mod --seconds=30 \
        --keys=14000:b1,18000:b1,22000:b1,26000:b1 2>&1)
    gswaps=$(echo "$glog" | grep -o '[0-9]*x IEGL11::SwapBuffers' | grep -o '^[0-9]*')
    echo "$glog" | grep -q "game_mode 0x00000001" || fail "jogo: nao chegou ao LOGO (game_mode 1)"
    echo "$glog" | grep -q "game_mode 0x0000000b" || fail "jogo: nao chegou aos menus (game_mode 11)"
    echo "$glog" | grep -q "gdFsOpen ok ava_vmssel.PRS" || fail "jogo: nao chegou a selecao do cartao (ava_vmssel)"
    [ "${gswaps:-0}" -ge "${GAME_FRAMES_MIN:-300}" ] || fail "jogo: ${gswaps:-0} quadros em 30 s (< ${GAME_FRAMES_MIN:-300})"
    traps=$(echo "$glog" | grep -c "sa1z: trap ")
    echo "ok: jogo (LOGO -> titulo -> menu, quadros=$gswaps, armadilhas atingidas=$traps)"
fi
