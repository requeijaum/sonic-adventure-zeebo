/* Boomerang (controle de movimento do Zeebo) -> botoes e analogico do SA1.
 * C puro, sem BREW: da para testar no PC (tests/boom_test.c). O formato do
 * pacote do receptor foi lido do zeebx (machine/hid.rs, feito a partir dos
 * jogos Boomerang Sports e Crash Nitro Kart), nao de um aparelho real. */
#ifndef ZBOOM_H
#define ZBOOM_H

enum {
    ZHID_UP, ZHID_DOWN, ZHID_LEFT, ZHID_RIGHT, ZHID_START, ZHID_BACK,
    ZHID_B1, ZHID_B2, ZHID_B3, ZHID_B4, ZHID_ZL, ZHID_L2, ZHID_ZR, ZHID_R2
};

#define ZBOOM_VENDOR  0x15a2
#define ZBOOM_PRODUCT 0x0003
#define ZBOOM_WORDS   25 /* palavras do AEEHIDPositionInfo (bRelativeAxes + 24) */

typedef struct {
    int neutral;    /* leitura do volante em repouso (ver zboom_wheel) */
    int player;     /* qual dos dois Boomerangs do receptor (-1: ainda nao) */
    int home_held;
    int home_used;
    int start_left; /* leituras restantes do pulso de Start */
} zboom_t;

void zboom_reset(zboom_t *b);
/* w: as 25 palavras do AEEHIDPositionInfo. Devolve 1 << ZHID_* e o analogico
 * esquerdo (-128..127); 0 e analogico zerado se o pacote nao for de um
 * Boomerang ligado. */
unsigned int zboom_update(zboom_t *b, const unsigned int *w, int *lx, int *ly);

#endif
