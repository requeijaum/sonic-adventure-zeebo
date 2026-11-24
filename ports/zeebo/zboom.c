/* Boomerang -> SA1. Mapa:
 *   cruz            -> cruz e analogico (cima/baixo/esq./dir. a fundo)
 *   volante (girar) -> analogico X (o Boomerang vai deitado, de frente para o
 *                      jogador, girado como um volante: o angulo da gravidade
 *                      entre X e Y; Crash Nitro Kart usa assim)
 *   botao 1 / 2     -> A / B
 *   HOME (toque)    -> Start
 *   HOME + 1 / 2    -> X / Y
 *   HOME + esq/dir  -> gatilhos L / R (camera)
 *   HOME + baixo    -> define a posicao atual como o centro do volante
 * Os sinais do volante e a escala do acelerometro vem do zeebx; no aparelho
 * real podem diferir (ZBOOM_STEER_SIGN). */
#include "zboom.h"

#ifndef ZBOOM_STEER_SIGN
#define ZBOOM_STEER_SIGN 1
#endif

#define DEAD  24  /* ~6 graus */
#define FULL 105  /* ~35 graus */

static int iabs(int v)
{
    return v < 0 ? -v : v;
}

/* gravidade no plano X/Y -> x / (|x| + |y|) * 256: cresce com o angulo, sem
 * trigonometria (o port e soft-float) */
static int wheel(int x, int y)
{
    int d = iabs(x) + iabs(y);

    return d < 20 ? 0 : (x * 256) / d; /* abaixo de ~1/3 g no plano: sem sinal */
}

void zboom_reset(zboom_t *b)
{
    b->neutral = 0;
    b->player = -1;
    b->home_held = 0;
    b->home_used = 0;
    b->start_left = 0;
}

unsigned int zboom_update(zboom_t *b, const unsigned int *w, int *lx, int *ly)
{
    unsigned int out = 0;
    unsigned int loose = w[4] & 0x7f; /* logica invertida: 0 = apertado */
    int type = (int)((w[5] >> 1) & 0x7);
    int player = (int)(w[5] & 1);
    int bx, by, x, y, s;
    int b1, b2, home, left, down, right, up, chord;

    *lx = *ly = 0;
    if (type == 0) {
        return 0; /* receptor sem Boomerang nesse jogador */
    }
    if (b->player < 0) {
        b->player = player; /* fica com o primeiro que apareceu */
    }
    if (player != b->player) {
        return 0;
    }
    b1    = !(loose & 1);
    b2    = !(loose & 2);
    home  = !(loose & 4);
    left  = !(loose & 8);
    down  = !(loose & 16);
    right = !(loose & 32);
    up    = !(loose & 64);

    /* acelerometro: um byte por eixo, centro 0x80, 60 por g. O jogo desfaz o
     * giro de 20 graus do sensor dentro do braco (cos .94, sen .342) */
    bx = (int)(w[1] & 0xff) - 128;
    by = (int)(w[2] & 0xff) - 128;
    x = (962 * bx + 350 * by) / 1024;
    y = (-350 * bx + 962 * by) / 1024;

    chord = 0;
    if (home) {
        if (!b->home_held) {
            b->home_used = 0;
        }
        if (down) {
            b->neutral = wheel(x, y); /* HOME + baixo: recentra */
            chord = 1;
        } else if (left) {
            out |= 1u << ZHID_ZL;
            chord = 1;
        } else if (right) {
            out |= 1u << ZHID_ZR;
            chord = 1;
        } else if (b1) {
            out |= 1u << ZHID_B3;
            chord = 1;
        } else if (b2) {
            out |= 1u << ZHID_B4;
            chord = 1;
        }
        if (chord) {
            b->home_used = 1;
        }
    } else {
        if (b->home_held && !b->home_used) {
            b->start_left = 4; /* toque curto: Start por 4 leituras */
        }
        if (b1) {
            out |= 1u << ZHID_B1;
        }
        if (b2) {
            out |= 1u << ZHID_B2;
        }
        if (up) {
            out |= 1u << ZHID_UP;
            *ly = -128;
        }
        if (down) {
            out |= 1u << ZHID_DOWN;
            *ly = 127;
        }
        if (left) {
            out |= 1u << ZHID_LEFT;
        }
        if (right) {
            out |= 1u << ZHID_RIGHT;
        }
    }
    b->home_held = home;
    if (b->start_left > 0) {
        b->start_left--;
        out |= 1u << ZHID_START;
    }

    /* volante -> analogico X; a cruz esq./dir. vale a fundo e passa na frente */
    s = (wheel(x, y) - b->neutral) * ZBOOM_STEER_SIGN;
    if (iabs(s) < DEAD) {
        s = 0;
    } else {
        s = (s > 0 ? s - DEAD : s + DEAD) * 127 / (FULL - DEAD);
    }
    if (s > 127) {
        s = 127;
    } else if (s < -128) {
        s = -128;
    }
    *lx = s;
    if (!home) {
        if (left) {
            *lx = -128;
        } else if (right) {
            *lx = 127;
        }
    }
    return out;
}
