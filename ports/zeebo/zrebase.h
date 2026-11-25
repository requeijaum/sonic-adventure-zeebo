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
/* Mapa por trechos (medido e conferido contra as constantes ZDC(...) do jogo, 2026-11-25).
 * O jogo so enderecava 0x0c900000..0x0d000000 (7 MB), mas desde que o binario SH-4 dos programas
 * deixou de ser descomprimido (zprs_skip_next, zgdfs.c) nada vive em 0x0c940000..0x0cadc000 (as
 * areas MODULE/FIELD dos programas) nem acima de 0x0cf00000 (fim de WORK_AREA):
 *   0x0c900000..0x0c940000  -> arena +0x000000 (256 KB: cabeca de advertise/summary)
 *   0x0c940000..0x0cadc000  -> poco de 64 KB (acessos perdidos caem aqui, sem tocar o resto)
 *   0x0cadc000..0x0cf00000  -> arena +0x050000 (4,1 MB: TAILS/TIKAL/SONIC_BOARD, field_messages,
 *                              camera/set, EXTRA/EVENT, DEMO, MOTION, MODEL, STREAM, WORK_AREA)
 *   0x0cf00000..0x0d000000  -> poco
 * O fim de WORK_AREA (0x0cf00000, inclusive) mapeia para o fim do seg2: o jogo calcula
 * WORK_AREA_END - tamanho (PVM lido no fim da area). O poco fica em +0x040000. ZDC() segue constante de link (inicializadores estaticos).
 * O array fica em .bss: o elf2mod grava o bss como zeros no .mod (4,45 MB aqui, eram 7,34). */
#define ZDC_RAM_SIZE 0x00474000u
extern unsigned char zdc_ram[ZDC_RAM_SIZE];
#define ZDC_A(a) ((unsigned long)(a) & 0x00ffffffu)
#define ZDC_OFF(a)                                                                            \
    (ZDC_A(a) < 0x00900000u                                                                   \
         ? 0x00040000u                                                                        \
         : (ZDC_A(a) < 0x00940000u                                                            \
                ? ZDC_A(a) - 0x00900000u                                                      \
                : (ZDC_A(a) < 0x00adc000u                                                     \
                       ? 0x00040000u + ((ZDC_A(a) - 0x00940000u) & 0xffffu)                   \
                       : (ZDC_A(a) <= 0x00f00000u                                             \
                              ? 0x00050000u + (ZDC_A(a) - 0x00adc000u)                        \
                              : 0x00040000u + ((ZDC_A(a) - 0x00f00000u) & 0xffffu)))))
#define ZDC(a) ((unsigned long)(zdc_ram + ZDC_OFF(a)))

#endif
