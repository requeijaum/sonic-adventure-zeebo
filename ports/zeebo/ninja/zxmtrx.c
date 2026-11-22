/* zxmtrx.c — um XMTRX só no .mod do jogo.
 *
 * O applet traz o Ninja de engine/dc/ninja (njm-*.o) sobre o SH4ZAM em
 * software, cujo XMTRX é o estado de shz_xmtrx_sw.c. O jogo traz o asm do
 * SH-4 traduzido (build/engine/native/asm.c), cujo XMTRX é o banco X
 * (native_xmtrx). No link os njm-*.o vencem njRotate*, njUnitMatrix,
 * njSetMatrix, njGetMatrix, njMultiMatrix e njCalcVector; o asm fica com
 * njTranslate, njScale, njPush/PopMatrix e njCalcPoint. Com dois XMTRX, as
 * rotações iam para um e as translações e os pontos saíam do outro: o Sonic
 * da seleção de personagem saía com os ossos fora do lugar.
 *
 * Aqui o estado do SH4ZAM passa a ser o banco X do asm. O layout é o mesmo:
 * XF0..XF15 em colunas (native.h), como shz_xmtrx__t.elem[16]. game.mk
 * enfraquece o shz_xmtrx_state_ da cópia do applet; este é forte. */

float *native_xmtrx(void);

void *shz_xmtrx_state_(void)
{
    return native_xmtrx();
}
