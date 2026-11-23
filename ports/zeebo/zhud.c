/* Overlay de desempenho (FPSHUD=1; padrao desligado): FPS, ms de game code por
 * quadro e RAM livre, no canto inferior direito. Serve para amostrar o jogo no
 * aparelho fase a fase; no zeebx o numero mede a emulacao, nao o Zeebo.
 *
 * Fonte 3x5 propria, desenhada em quadrados (sem textura): o jogo nem sempre
 * carrega a fonte de debug do Ninja, e este texto nao pode depender do jogo. */
#include "AEEStdLib.h"
#include "zstrip.h"

#ifndef Z_FPSHUD
#define Z_FPSHUD 0
#endif

#if Z_FPSHUD
/* 3 colunas x 5 linhas, bit 14 = canto superior esquerdo */
static const struct {
    char c;
    unsigned short bits;
} glyphs[] = {
    {'0', 0x7B6F}, {'1', 0x2C97}, {'2', 0x73E7}, {'3', 0x73CF},
    {'4', 0x5BC9}, {'5', 0x79CF}, {'6', 0x79EF}, {'7', 0x7249},
    {'8', 0x7BEF}, {'9', 0x7BCF}, {'F', 0x79E4}, {'P', 0x7BE4},
    {'S', 0x79CF}, {'G', 0x796F}, {'M', 0x5FED}, {'.', 0x0002},
};

#define CELL 3
#define HUD_Z 0.99F

static void quad(float x, float y, float w, float h, unsigned argb)
{
    zstrip_begin();
    zstrip_vertex(x, y, HUD_Z, 0.0F, 0.0F, argb);
    zstrip_vertex(x, y + h, HUD_Z, 0.0F, 0.0F, argb);
    zstrip_vertex(x + w, y, HUD_Z, 0.0F, 0.0F, argb);
    zstrip_vertex(x + w, y + h, HUD_Z, 0.0F, 0.0F, argb);
    zstrip_end();
}

static void glyph(char c, float x, float y)
{
    unsigned i, j;

    for (i = 0; i < sizeof glyphs / sizeof glyphs[0]; i++) {
        if (glyphs[i].c != c) {
            continue;
        }
        for (j = 0; j < 15; j++) {
            if (glyphs[i].bits & (1u << (14 - j))) {
                quad(x + (float)(j % 3) * CELL, y + (float)(j / 3) * CELL,
                     CELL, CELL, 0xffffffffu);
            }
        }
        return;
    }
}

/* v em decimos: "34.8" */
static char *put_tenths(char *d, unsigned v)
{
    unsigned t = v / 10;

    if (t >= 100) {
        *d++ = (char)('0' + (t / 100) % 10);
    }
    if (t >= 10) {
        *d++ = (char)('0' + (t / 10) % 10);
    }
    *d++ = (char)('0' + t % 10);
    *d++ = '.';
    *d++ = (char)('0' + v % 10);
    return d;
}
#endif

/* uma vez por quadro apresentado, antes do zstrip_flush.
 * game_total_ms: tempo acumulado dentro do jogo (zgame_step); free_kb: RAM livre */
void zhud_frame(unsigned now_ms, unsigned game_total_ms, unsigned free_kb)
{
#if Z_FPSHUD
    static unsigned since, frames, game0;
    static char text[40];
    char *d;
    int i;

    frames++;
    if (since == 0) {
        since = now_ms;
        game0 = game_total_ms;
        return;
    }
    if (now_ms - since >= 1000) {
        unsigned win = now_ms - since;

        d = text;
        *d++ = 'F'; *d++ = 'P'; *d++ = 'S'; *d++ = ' ';
        d = put_tenths(d, frames * 10000u / win);
        *d++ = ' '; *d++ = 'G'; *d++ = ' ';
        d = put_tenths(d, (game_total_ms - game0) * 10u / frames);
        *d++ = ' '; *d++ = 'M'; *d++ = ' ';
        d = put_tenths(d, free_kb * 10u / 1024u);
        *d = 0;
        since = now_ms;
        frames = 0;
        game0 = game_total_ms;
    }
    if (text[0] == 0) {
        return;
    }
    zstrip_no_texture();
    zstrip_blend(1);
    quad(636.0F - 4.0F * 13.0F * CELL - 130.0F, 440.0F, 4.0F * 13.0F * CELL + 134.0F, 32.0F,
         0xa0000000u);
    for (i = 0; text[i] != 0; i++) {
        if (text[i] != ' ') {
            glyph(text[i], 636.0F - 4.0F * 13.0F * CELL - 128.0F + (float)i * 4.0F * CELL,
                  449.0F);
        }
    }
    zstrip_blend(0);
#else
    (void)now_ms;
    (void)game_total_ms;
    (void)free_kb;
#endif
}
