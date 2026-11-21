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
