#include "zrebase.h"

/* arena de RAM do Dreamcast (ver zrebase.h) */
unsigned char zdc_ram[ZDC_RAM_SIZE];

/* toca o começo e o fim da arena: prova que o bss de 16 MB está mapeado */
int zdc_selftest(void)
{
    zdc_ram[0] = 0x5a;
    zdc_ram[ZDC_RAM_SIZE - 1] = 0xa5;
    return zdc_ram[0] == 0x5a && zdc_ram[ZDC_RAM_SIZE - 1] == 0xa5 &&
           ZDC(0x8ceffffc) == (unsigned long)&zdc_ram[ZDC_RAM_SIZE - 4] &&
           ZDC(0x0cb48000) == (unsigned long)&zdc_ram[0xbc000] &&
           ZDC(0x0c950000) == (unsigned long)&zdc_ram[0x40000] &&
           /* ponteiros de fim de area (um depois do ultimo byte) sao enderecos validos do
            * jogo: WORK_AREA_END - tamanho le o PVM no fim da area (TITLE.PRS). Mapeados no
            * poco, a leitura de 294 KB caia abaixo da arena e corrompia o .bss vizinho */
           ZDC(0x0cf00000) == (unsigned long)&zdc_ram[ZDC_RAM_SIZE] &&
           ZDC(0x0cf00000) - 0x47e72ul >= ZDC(0x0cadc000) &&
           ZDC(0x0c940000) == (unsigned long)&zdc_ram[0x40000] &&
           ZDC(0x0cadc000) == (unsigned long)&zdc_ram[0x50000];
}

/* a tabela de falas do campo (struct field_messages, adv00/crowd.c) fica no
 * endereço fixo do DC (0x0cb48000) dentro da arena: o arquivo ss_mes_*.bin é
 * lido inteiro ali e seus ponteiros internos são do DC (zgdfs.c os reloca).
 * O 1ST_READ não a define; os programas de campo a leem por nome. */
__asm__(".globl field_messages\n"
        ".set field_messages, zdc_ram + 0xbc000\n"); /* ZDC_OFF(0x0cb48000), conferido no selftest */

/* shtpath1/2.PRS (Sky Chase), ja descomprimido em EXTRA_AREA: a primeira palavra
 * e cada cena {kind, path, begin, end} trazem ponteiros absolutos do DC. Anda as
 * cenas como create_actors (ate path == 0) e os troca pelo endereco ARM. Pode
 * rodar de novo: so mexe no que ainda aponta para a arena do DC. */
static int zsht_dc(unsigned long v)
{
    unsigned long a = v & 0x1fffffffu;

    return ((v >> 29) == 0 || (v >> 29) == 4) && a >= ZDC_RAM_BASE + 0x0c000000u &&
           a < 0x0d000000u;
}

void zshooting_fix(void *file)
{
    unsigned long *w = (unsigned long *)file;
    unsigned long *s;
    int k;

    if (zsht_dc(w[0])) {
        w[0] = ZDC(w[0]);
    }
    for (s = (unsigned long *)w[0]; s[1] != 0; s += 4) {
        for (k = 1; k < 4; k++) {
            if (zsht_dc(s[k])) {
                s[k] = ZDC(s[k]);
            }
        }
    }
}
