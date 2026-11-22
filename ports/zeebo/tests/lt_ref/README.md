# lt_ref

Fuzz das calculadoras de luz de `ports/zeebo/ninja/lt_arm.c` contra o asm SH-4
de `sdk/ninja/njLtSrcs.c` traduzido por `engine/tools/asm2c.py`
(`asm2c.py lt.c sdk/ninja/njLtSrcs.c --explore`). `fuzz.c` inclui `lt.c` e o
`lt_arm.c` (com `lt_rgb_XX` renomeadas para `my_lt_rgb_XX`) e compara 20000
luzes aleatórias por calculadora. Compilar com `-m64 -no-pie
-Wl,-Ttext-segment=0x10000000` (o asm usa endereços de 32 bits).
Resultado: as 23 calculadoras e `lt_power_body` batem (erro < 1e-6).
