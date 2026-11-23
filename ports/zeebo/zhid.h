/* controle do Zeebo pelo IHID (zhid.c) */
#ifndef ZHID_H
#define ZHID_H

#include <AEEShell.h>

enum {
    ZHID_UP, ZHID_DOWN, ZHID_LEFT, ZHID_RIGHT, ZHID_START, ZHID_BACK,
    ZHID_B1, ZHID_B2, ZHID_B3, ZHID_B4, ZHID_ZL, ZHID_L2, ZHID_ZR, ZHID_R2
};

void zhid_open(IShell *shell);
void zhid_close(void);
/* botoes (1 << ZHID_*) e analogicos de -128 a 127; 0 sem controle */
unsigned int zhid_poll(int *lx, int *ly, int *rx, int *ry);

#endif
