# njmodel_ref

Compara os loops de ponto do `sdk/ninja/njModel.c` (1 a 10): o asm SH-4
traduzido por `engine/tools/asm2c.py` (referência) contra o C de
`ports/zeebo/ninja/njmodel_arm.c`.

    python3 engine/tools/asm2c.py /tmp/ref/loops.c sdk/ninja/njModel.c --explore
    # ref.c inclui loops.c; mine.c leva calc_point/LOOP do njmodel_arm.c
    gcc -g -O0 -w -no-pie -Wl,-Ttext-segment=0x10000000 -I engine/native \
        -I engine/numeric -I engine/include ref.c engine/numeric/math.c -o ref -lm
    gcc -O0 -w mine.c -o mine -lm
    for l in 1 2 3 4 5 6 7 8 9 10; do ./ref $l; ./mine $l; done

O asm usa endereços de 32 bits: `ref.c` mapeia o buffer abaixo de 4 GB e um
alias em 0xE0000000 (store queue). Diferenças de até 0,001 px são o
arredondamento do FSRRA/FIPR.
