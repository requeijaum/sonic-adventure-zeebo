/* zsprite.c — njDrawSprite2D real do decomp sobre o sink GLES1.
 *
 * Prova o caminho 2D de sprite (título/HUD/menus): engine/dc/njSprite.c e
 * sa1/sprite.c + a sombra de sa1/sprite.h que troca só o envio à store queue.
 * A textura vem do texlist (njSetTexture) + texid (njSetTextureNum). */

#define GL_OES_VERSION_1_1
#include "gles/gl.h"

#include <ninja.h>

#include "zstrip.h"

static NJS_TEXANIM sprite_frame = {
    64, 64,   /* sx, sy */
    32, 32,   /* cx, cy */
    0,  0,    /* u1, v1 */
    256, 256, /* u2, v2 */
    0, 0      /* texid, attr */
};

static NJS_SPRITE sprite;
extern NJS_TEXLIST zscene_texlist;

void zsprite_init(void)
{
    sprite.p.x = 500.0f;
    sprite.p.y = 120.0f;
    sprite.p.z = 0.0f;
    sprite.sx = 1.0f;
    sprite.sy = 1.0f;
    sprite.ang = 0;
    sprite.tlist = &zscene_texlist;
    sprite.tanim = &sprite_frame;
}

void zsprite_frame(uint32_t tick)
{
    zsys_frame();
    sprite.p.x = 470.0f + (float)((tick * 2) % 130) + (float)zsys_pad_x();
    sprite.p.y = 120.0f + (float)zsys_pad_y();
    sprite.ang = (Angle)(tick * 350);
    /* troca a textura do frame pelo texid, como o jogo faz */
    if (zscene_texlist.nbTexture > 0) {
        sprite_frame.texid = (tick / 30) % zscene_texlist.nbTexture;
    }
    njDrawSprite2D(&sprite, 0, 0.95f,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    /* indicador do timer: verde = syTmr* anda em tempo real, vermelho = não */
    zstrip_no_texture();
    zstrip_blend(0);
    zstrip_begin();
    {
        uint32_t c = zsys_timer_ok() ? 0xff30e070u : 0xffe03030u;

        zstrip_vertex(4.0f, 4.0f, 0.97f, 0.0f, 0.0f, c);
        zstrip_vertex(20.0f, 4.0f, 0.97f, 0.0f, 0.0f, c);
        zstrip_vertex(4.0f, 20.0f, 0.97f, 0.0f, 0.0f, c);
        zstrip_vertex(20.0f, 20.0f, 0.97f, 0.0f, 0.0f, c);
    }
    zstrip_end();
}
