#!/bin/bash
# dd_matrix.sh "PCTS" LINHA...  -- mede a distancia de desenho (drawdist.cfg) em linhas do menu de debug.
# Para cada pct cria $DD/p<pct>/mod/199258115/ (o zeebx acha o applet pelo nome da pasta) (symlinks dos arquivos do jogo, o .mod de build/zeebo e o drawdist.cfg),
# roda a linha no zeebx (session, sem janela) e guarda o log em $DD/logs/r<linha>_p<pct>.log.
#   dd_matrix.sh "100 75 50 25" 3 6 9 ...
# Variaveis: JOBS (paralelo, 3), SECS (segundos depois do inicio da fase, 60), MOD (padrao build/zeebo/sa1g.mod)
set -u
PCTS=($1); shift
ROWS=("$@")
JOBS=${JOBS:-3}; SECS=${SECS:-60}
SRC="$HOME/zeebo-roms-fullset/Sonic Adventure 1 (Zeebo Port)/mod/199258115"
MOD=${MOD:-$HOME/projects/sonic-adventure-zeebo/build/zeebo/sa1g.mod}
Z=$HOME/projects/zeebx-emu-copytex/target/release/zeebx
DD=${DD:-$HOME/.hermes/cache/scratch/dd}
mkdir -p "$DD/logs"
for p in "${PCTS[@]}"; do
    d="$DD/p$p/mod/199258115"
    if [ ! -f "$d/.ok" ] || [ "$MOD" -nt "$d/sa1n.mod" ]; then
        rm -rf "$DD/p$p"; mkdir -p "$d"; ln -s "$SRC/../../mif" "$DD/p$p/mif"
        for f in "$SRC"/*; do
            b=$(basename "$f")
            case "$b" in sa1n.mod|drawdist.cfg) continue;; esac
            case "$b" in *.ovl) cp -p "$(dirname "$MOD")/$b" "$d/$b" 2>/dev/null || ln "$f" "$d/$b";; *) ln "$f" "$d/$b" 2>/dev/null || cp -a "$f" "$d/$b";; esac
        done
        cp -p "$MOD" "$d/sa1n.mod"
        touch "$d/.ok"
    fi
    echo "$p" > "$d/drawdist.cfg"
done
run_one() {
    r=$1; p=$2
    d="$DD/p$p/mod/199258115"; log="$DD/logs/r${r}_p${p}.log"
    keys="6000:zl:3200,6000:zr:3200,6000:up:3200"; t=11000
    for i in $(seq 1 $r); do keys="$keys,$t:down:120"; t=$((t+300)); done
    go=$((t+1800))
    keys="$keys,$go:b1:300,$((go+26000)):right:6000,$((go+34000)):b1:200"
    timeout 400 "$Z" session "$d/sa1n.mod" --seconds=$((go/1000+SECS)) --ports=zpad --keys="$keys" --serial="$log" >/dev/null 2>&1
    echo "row $r pct $p faults=$(grep -acE 'did not return|fault at|TRAP' "$log") lines=$(wc -l < "$log")"
}
export -f run_one; export DD Z SECS
jobs_list=()
for r in "${ROWS[@]}"; do for p in "${PCTS[@]}"; do jobs_list+=("$r $p"); done; done
printf '%s\n' "${jobs_list[@]}" | xargs -P "$JOBS" -L1 bash -c 'run_one $0 $1'
