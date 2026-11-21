/* Arena de RAM do Dreamcast (16 MB) para os endereços fixos do jogo.
 * ZDC(0x0cXXXXXX) e ZDC(0x8cXXXXXX) viram arena + deslocamento. */
#ifndef SA1Z_ZREBASE_H
#define SA1Z_ZREBASE_H

/* Só 0x0c900000..0x0d000000 (7 MB) é endereçado pelo código do jogo: a
 * menor área fixa é MODULE_AREA (0x0c900000) e a RAM acaba em 0x0d000000.
 * O array fica em .bss: o endereço é constante de link, então ZDC() vale em
 * inicializadores estáticos (stream.c, recorder.h, shooting.c). Custo: o
 * elf2mod grava o bss como zeros, ~7 MB a mais no .mod (16 MB dariam um .mod
 * de 18,5 MB, medido). Alternativa: MALLOC + reescrever esses 3 inicializadores. */
#define ZDC_RAM_BASE 0x00900000u
#define ZDC_RAM_SIZE 0x00700000u
extern unsigned char zdc_ram[ZDC_RAM_SIZE];
#define ZDC(a) ((unsigned long)(zdc_ram + (((unsigned long)(a) & 0x00ffffffu) - ZDC_RAM_BASE)))

#endif
