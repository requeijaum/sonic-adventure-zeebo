#include "zrebase.h"

/* arena de RAM do Dreamcast (ver zrebase.h) */
unsigned char zdc_ram[ZDC_RAM_SIZE];

/* toca o começo e o fim da arena: prova que o bss de 16 MB está mapeado */
int zdc_selftest(void)
{
    zdc_ram[0] = 0x5a;
    zdc_ram[ZDC_RAM_SIZE - 1] = 0xa5;
    return zdc_ram[0] == 0x5a && zdc_ram[ZDC_RAM_SIZE - 1] == 0xa5 &&
           ZDC(0x8ccffffc) == (unsigned long)&zdc_ram[0x00cffffc - ZDC_RAM_BASE];
}

/* a tabela de falas do campo (struct field_messages, adv00/crowd.c) fica no
 * endereço fixo do DC (0x0cb48000) dentro da arena: o arquivo ss_mes_*.bin é
 * lido inteiro ali e seus ponteiros internos são do DC (zgdfs.c os reloca).
 * O 1ST_READ não a define; os programas de campo a leem por nome. */
__asm__(".globl field_messages\n"
        ".set field_messages, zdc_ram + 0x248000\n");
