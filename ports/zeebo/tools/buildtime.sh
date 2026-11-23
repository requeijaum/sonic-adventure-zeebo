#!/bin/bash
# Mede cada fase de um `make mod`: troca o SHELL do make por um wrapper que
# anota a duracao de cada linha de receita. Uso (na raiz do repo):
#   ports/zeebo/tools/buildtime.sh [argumentos do make, ex.: DEBUG_TITLE=0]
# Escreve a lista ordenada (maior primeiro) e o total.
set -u
cd "$(dirname "$0")/../../.."
export LC_ALL=C
LOG=${TMPDIR:-/tmp}/buildtime.log
WRAP=${TMPDIR:-/tmp}/buildtime-shell.sh
cat > "$WRAP" <<'E'
#!/bin/bash
s=$(date +%s%N); /bin/bash "$@"; rc=$?; e=$(date +%s%N)
printf '%7d ms  %s\n' $(( (e - s) / 1000000 )) "$(printf '%s' "${2:0:100}" | tr '\n' ' ')" >> "$BUILDTIME_LOG"
exit $rc
E
chmod +x "$WRAP"
rm -f "$LOG"
export BUILDTIME_LOG="$LOG"
t0=$(date +%s)
make -C ports/zeebo -f game.mk mod SHELL="$WRAP" "$@" 2>&1 | grep -E 'rror|ERRO|Created'
echo "TOTAL $(( $(date +%s) - t0 )) s"
sort -rn "$LOG" | head -12
