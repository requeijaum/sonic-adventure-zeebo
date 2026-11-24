/* gcc -I.. -o /tmp/boom_test boom_test.c ../zboom.c && /tmp/boom_test */
#include <stdio.h>
#include <string.h>
#include "zboom.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FALHOU %d: %s\n", __LINE__, #c); fails++; } } while (0)

/* o mesmo pacote que o zeebx monta (machine/hid.rs, boomerang_packet) */
static void pkt(unsigned *w, int player, int type, unsigned loose, int ax, int ay, int az)
{
    memset(w, 0, ZBOOM_WORDS * 4);
    w[1] = (unsigned)(128 + ax); w[2] = (unsigned)(128 + ay); w[3] = (unsigned)(128 + az);
    w[4] = loose & 0x7f;
    w[5] = ((unsigned)type << 1) | (unsigned)player;
}
/* gravidade (x,y) em g, ja com a rotacao de 20 graus do sensor aplicada */
static void grav(unsigned *w, int player, unsigned loose, double x, double y)
{
    double raw_x = 0.94 * x - 0.342 * y, raw_y = 0.342 * x + 0.94 * y;
    pkt(w, player, 1, loose, (int)(raw_x * 60 + (raw_x < 0 ? -.5 : .5)),
        (int)(raw_y * 60 + (raw_y < 0 ? -.5 : .5)), 0);
}

int main(void)
{
    zboom_t b; unsigned w[ZBOOM_WORDS]; int lx, ly; unsigned o;
    const unsigned NONE = 0x7f;

    zboom_reset(&b);
    pkt(w, 0, 0, NONE, 0, 60, 0);                        /* desconectado */
    CHECK(zboom_update(&b, w, &lx, &ly) == 0 && lx == 0);

    grav(w, 0, NONE, 0.0, 1.0);                          /* volante reto */
    o = zboom_update(&b, w, &lx, &ly); CHECK(o == 0 && lx == 0 && ly == 0);
    grav(w, 0, NONE, 0.5, 0.866);                        /* 30 graus a direita */
    zboom_update(&b, w, &lx, &ly); CHECK(lx > 60);
    grav(w, 0, NONE, -0.5, 0.866);                       /* 30 graus a esquerda */
    zboom_update(&b, w, &lx, &ly); CHECK(lx < -60);
    grav(w, 0, NONE, 0.087, 0.996);                      /* 5 graus: zona morta */
    zboom_update(&b, w, &lx, &ly); CHECK(lx == 0);
    grav(w, 0, NONE, 0.7, 0.7);                          /* 45 graus: a fundo */
    zboom_update(&b, w, &lx, &ly); CHECK(lx == 127);
    grav(w, 1, NONE, 0.7, 0.7);                          /* o outro jogador: ignora */
    CHECK(zboom_update(&b, w, &lx, &ly) == 0 && lx == 0);

    grav(w, 0, NONE & ~1u, 0, 1);                        /* botao 1 -> A */
    CHECK(zboom_update(&b, w, &lx, &ly) == (1u << ZHID_B1));
    grav(w, 0, NONE & ~2u, 0, 1);                        /* botao 2 -> B */
    CHECK(zboom_update(&b, w, &lx, &ly) == (1u << ZHID_B2));
    grav(w, 0, NONE & ~64u, 0, 1);                       /* cima */
    o = zboom_update(&b, w, &lx, &ly); CHECK((o & (1u << ZHID_UP)) && ly == -128);
    grav(w, 0, NONE & ~32u, 0, 1);                       /* direita */
    o = zboom_update(&b, w, &lx, &ly); CHECK((o & (1u << ZHID_RIGHT)) && lx == 127);

    /* HOME: toque -> Start por 4 leituras; com botao -> X, sem Start */
    zboom_reset(&b);
    grav(w, 0, NONE & ~4u, 0, 1);
    CHECK(zboom_update(&b, w, &lx, &ly) == 0);           /* segurando */
    grav(w, 0, NONE, 0, 1);
    CHECK(zboom_update(&b, w, &lx, &ly) == (1u << ZHID_START));
    CHECK(zboom_update(&b, w, &lx, &ly) == (1u << ZHID_START));
    CHECK(zboom_update(&b, w, &lx, &ly) == (1u << ZHID_START));
    CHECK(zboom_update(&b, w, &lx, &ly) == (1u << ZHID_START));
    CHECK(zboom_update(&b, w, &lx, &ly) == 0);
    grav(w, 0, NONE & ~4u & ~1u, 0, 1);                  /* HOME + 1 -> X */
    CHECK(zboom_update(&b, w, &lx, &ly) == (1u << ZHID_B3));
    grav(w, 0, NONE, 0, 1);
    CHECK(zboom_update(&b, w, &lx, &ly) == 0);           /* solta: sem Start */
    grav(w, 0, NONE & ~4u & ~8u, 0, 1);                  /* HOME + esq. -> L */
    CHECK(zboom_update(&b, w, &lx, &ly) == (1u << ZHID_ZL));

    /* HOME + baixo recentra: 30 graus passa a ser o zero */
    zboom_reset(&b);
    grav(w, 0, NONE & ~4u & ~16u, 0.5, 0.866);
    zboom_update(&b, w, &lx, &ly);
    grav(w, 0, NONE, 0.5, 0.866);
    zboom_update(&b, w, &lx, &ly); CHECK(lx == 0);
    grav(w, 0, NONE, 0.0, 1.0);
    zboom_update(&b, w, &lx, &ly); CHECK(lx < -40);

    printf(fails ? "%d falhas\n" : "boom_test: ok\n", fails);
    return fails != 0;
}
