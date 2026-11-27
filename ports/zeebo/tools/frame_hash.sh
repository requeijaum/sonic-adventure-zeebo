#!/bin/bash
# Roda o jogo (sa1g.mod) sem janela, com limites, e imprime o hash do último
# quadro (ou do quadro pedido). Os dumps são apagados no fim.
#
#   tools/frame_hash.sh                     cena padrão: seleção de personagem
#   SECONDS_RUN=20 KEYS=... tools/frame_hash.sh
#   FRAME=00300 tools/frame_hash.sh         hash de um quadro específico
#   KEEP=/caminho/q.png tools/frame_hash.sh guarda o quadro como PNG
#   LOGPAT='^live' tools/frame_hash.sh     mostra as linhas do log que casam
#
# Limites (o run longo com 563 BMPs coincidiu com um travamento da máquina em
# 2026-11-22): nice 19, timeout, memória virtual (MEM_KB) e limpeza dos dumps.
set -u
cd "$(dirname "$0")/.."
_E=$(cd "$(dirname "$0")" && pwd)/
[ -f "$_E/env.sh" ] && . "$_E/env.sh" || . "$_E/env.sh.example"
ZEEBX=${ZEEBX:-$ZEEBX_V060}
MOD=${MOD:-../../build/zeebo/sa1g.mod}
SECONDS_RUN=${SECONDS_RUN:-62}
# do logo até a seleção de personagem (a sequência usada na investigação do 3D)
KEYS=${KEYS:-30000:b1,34000:b1,38000:b1,42000:b1,46000:b1,49000:left,51000:b1,55000:b1}
MEM_KB=${MEM_KB:-3000000}
TMPD=$(mktemp -d "${XDG_CACHE_HOME:-$HOME/.cache}/sa1z-check/fh.XXXXXX") || exit 1
trap 'rm -rf "$TMPD"' EXIT

[ -f "$MOD" ] || { echo "FALHOU: sem $MOD"; exit 1; }
(
    ulimit -v "$MEM_KB"
    exec timeout $((SECONDS_RUN + 120)) nice -n 19 "$ZEEBX" run "$MOD" \
        --seconds="$SECONDS_RUN" --keys="$KEYS" --dump-gl="$TMPD/gl"
) > "$TMPD/log" 2>&1
rc=$?
[ -n "${LOGPAT:-}" ] && grep -aE "$LOGPAT" "$TMPD/log" | tail -"${LOGN:-60}"

if [ -n "${FRAME:-}" ]; then
    f="$TMPD/gl/$FRAME.bmp"
else
    # o último dump pode estar vazio se o run foi interrompido
    f=$(find "$TMPD/gl" -name '*.bmp' -size +0 2>/dev/null | sort | tail -1)
fi
[ -n "$f" ] && [ -s "$f" ] || { echo "FALHOU: sem quadro (rc=$rc)"; tail -5 "$TMPD/log"; exit 1; }
n=$(ls "$TMPD/gl" | wc -l)
h=$(sha256sum < "$f" | cut -c1-12)
echo "quadro $(basename "$f" .bmp) de $n  hash $h  rc=$rc"
if [ -n "${KEEP:-}" ]; then
    python3 -c "from PIL import Image; import sys; Image.open(sys.argv[1]).save(sys.argv[2])" "$f" "$KEEP"
    echo "guardado em $KEEP"
fi
# KEEP_DIR=/dir [KEEP_EVERY=60]: guarda um quadro a cada KEEP_EVERY como PNG
if [ -n "${KEEP_DIR:-}" ]; then
    mkdir -p "$KEEP_DIR"
    python3 - "$TMPD/gl" "$KEEP_DIR" "${KEEP_EVERY:-60}" <<'EOF'
import os, sys
from PIL import Image
src, dst, every = sys.argv[1], sys.argv[2], int(sys.argv[3])
fs = sorted(f for f in os.listdir(src) if f.endswith('.bmp') and os.path.getsize(os.path.join(src, f)))
for i, f in enumerate(fs):
    if i % every == 0 or i == len(fs) - 1:
        Image.open(os.path.join(src, f)).save(os.path.join(dst, f[:-4] + '.png'))
print(f"{len(os.listdir(dst))} PNGs em {dst}")
EOF
fi
