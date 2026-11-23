#!/bin/bash
# Gate: o ARM11 do Zeebo (MSM7201A) nao tem VFP; uma instrucao VFP no .elf
# mata o app no aparelho. Falha (exit 1) se achar mnemonico VFP/coprocessador
# 10/11 no disassembly.   tools/novfp.sh [arquivo.elf]
set -u
ELF=${1:-$(dirname "$0")/../../../build/zeebo/sa1g.elf}
[ -f "$ELF" ] || { echo "novfp: $ELF nao existe"; exit 2; }
hits=$(arm-none-eabi-objdump -d "$ELF" 2>/dev/null | awk -F'\t' '
  $3 ~ /^(vadd|vsub|vmul|vdiv|vsqrt|vcmp|vcvt|vldr|vstr|vldm|vstm|vpush|vpop|vmov|vmrs|vmsr|vabs|vneg|vnmul|vmla|vmls|fld|fst|fadd|fsub|fmul|fdiv|fmrs|fmsr)/ \
  || $3 ~ /^(mrc|mcr|ldc|stc|cdp)2?[a-z]*$/ && $4 ~ /^p1[01],/ {print}' | head -5)
if [ -n "$hits" ]; then echo "novfp: FALHOU, instrucoes VFP em $ELF:"; echo "$hits"; exit 1; fi
echo "novfp: OK, 0 instrucoes VFP em $ELF"
