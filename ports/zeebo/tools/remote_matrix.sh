#!/bin/bash
# Roda linhas do menu de debug em PARALELO no Ryzen (zeebo-xbuild), com um unico
# .mod (build com DEBUG_TITLE=0): as teclas "down" levam o cursor ate a linha N.
#   tools/remote_matrix.sh [-j N] [-s SEGUNDOS] LINHA...
# Resultado: build/zeebo/matrix_r/rowN.png, rowN.log e summary.txt.
# Pre-requisitos no Ryzen: /root/zeebx (estatico), /root/pkg (build/zeebo sem
# game/ e applet/), python3-pil. Envia o .mod e frame_hash.sh a cada chamada.
set -u
cd "$(dirname "$0")/.."
ROOT=$(cd ../.. && pwd)
H=${REMOTE:-zeebo-xbuild}
J=3; SECS=80
while getopts j:s: o; do case $o in j) J=$OPTARG;; s) SECS=$OPTARG;; esac; done
shift $((OPTIND-1))
ROWS=("$@")
[ ${#ROWS[@]} -eq 0 ] && { echo "uso: $0 [-j N] [-s S] LINHA..."; exit 2; }
OUTD=$ROOT/build/zeebo/matrix_r
mkdir -p "$OUTD"
rsync -aq "$ROOT/build/zeebo/sa1g.mod" "$ROOT/build/zeebo/sa1g.mif" "$H:/root/pkg/"
rsync -aq tools/frame_hash.sh "$H:/root/frame_hash.sh"
ssh "$H" 'mkdir -p /root/out /root/ports/zeebo/tools /root/.cache/sa1z-check; cp /root/frame_hash.sh /root/ports/zeebo/tools/frame_hash.sh; rm -f /root/out/*'
ssh "$H" "printf '%s\n' ${ROWS[*]} | xargs -P $J -I{} bash -c '
r={}; keys=\"\"; t=14000
for ((i=0;i<r;i++)); do keys=\"\${keys}\${t}:down,\"; t=\$((t+500)); done
keys=\"\${keys}\$((t+5000)):b1\"
ZEEBX=/root/zeebx MOD=/root/pkg/sa1g.mod SECONDS_RUN=$SECS KEYS=\$keys MEM_KB=2800000 \
  LOGPAT=\"TRAP|trap |programa ausente|vbind|fim do jogo|modo\" LOGN=80 KEEP=/root/out/row\$r.png \
  /root/ports/zeebo/tools/frame_hash.sh > /root/out/row\$r.log 2>&1
echo \"row \$r | \$(grep -h hash /root/out/row\$r.log | tail -1) | traps=\$(grep -ciE \"trap \" /root/out/row\$r.log) ausente=\$(grep -c \"programa ausente\" /root/out/row\$r.log)\"
'" | tee -a "$OUTD/summary.txt"
rsync -aq "$H:/root/out/" "$OUTD/"
