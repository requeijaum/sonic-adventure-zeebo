#!/bin/bash
# Monta o "disco" do jogo como diretório plano de links, para o zeebx achar os
# arquivos pelo nome que o jogo pede (sem ;1, minúsculo; o jogo ignora a caixa).
#   tools/stage_disc.sh <raiz extraída do disco> <destino>
# A raiz vem de um GD-ROM seu (ex.: tools/gdrom_extract.py --extract-all).
# Nada é copiado para o git. Usa hard links (o zeebx v0.6.0 ignora links
# simbólicos no pacote); em outro sistema de arquivos, copia.
set -eu
src=${1:?raiz extraida}
dst=${2:?destino}
mkdir -p "$dst"
n=0; dup=0
while IFS= read -r -d '' f; do
    base=$(basename "$f"); base=${base%;*}
    low=$(echo "$base" | tr 'A-Z' 'a-z')
    if [ -L "$dst/$low" ]; then rm -f "$dst/$low"; fi
    if [ -e "$dst/$low" ]; then dup=$((dup+1)); continue; fi
    ln "$f" "$dst/$low" 2>/dev/null || cp "$f" "$dst/$low"; n=$((n+1))
done < <(find "$src" -type f -print0)
echo "links: $n, nomes repetidos ignorados: $dup"
