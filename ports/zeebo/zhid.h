/* controle do Zeebo pelo IHID (zhid.c) */
#ifndef ZHID_H
#define ZHID_H

#include <AEEShell.h>

#include "zboom.h" /* enum ZHID_* */

void zhid_open(IShell *shell);
void zhid_close(void);
/* botoes (1 << ZHID_*) e analogicos de -128 a 127; 0 sem controle */
unsigned int zhid_poll(int *lx, int *ly, int *rx, int *ry);

#endif
