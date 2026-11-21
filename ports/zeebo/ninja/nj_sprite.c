/* nj_sprite.c — estado do Ninja que o caminho de sprite lê, e a cor do strip.
 *
 * No jogo, `__nj_screen_`, os clips e `_nj_constant_material_` vêm dos objetos
 * do SDK (njSystem). No .mod do Z2 ainda não há jogo linkado, então o
 * bring-up define os mesmos símbolos com os valores padrão do Ninja para
 * 640x480. `njSetTexture`/`njSetkmTextureNum` só registram: o sink GLES1
 * liga a textura de teste. */

#include <stdint.h>

#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "zstrip.h"
#include "assets/pvm.h"

/* NJS_SCREEN: dist, w, h, cx, cy */
NJS_SCREEN __nj_screen_ = {320.0f, 640.0f, 480.0f, 320.0f, 240.0f};
Float __nj_far_clip_ = 0.0f;
NJS_POINT2 __nj_2d_clip_[2] = {{0.0f, 0.0f}, {640.0f, 480.0f}};
Float __nj_scr_aspect_y_ = 1.0f;
Float __nj_near_clip_ = 1.0f;
NJS_ARGB _nj_constant_material_ = {1.0f, 1.0f, 1.0f, 1.0f};

static uint32_t sink_color = 0xffffffffu;

uint32_t sprite_sink_color(void)
{
    return sink_color;
}

/* a cor que sprite_begin passa a njStartVertexTex*Intens: NJS_ARGB (a,r,g,b) */
void zsprite_set_color(const Uint32 *color)
{
    const Float *f = (const Float *)color;
    uint32_t a = (uint32_t)(f[0] * 255.0f + 0.5f);
    uint32_t r = (uint32_t)(f[1] * 255.0f + 0.5f);
    uint32_t g = (uint32_t)(f[2] * 255.0f + 0.5f);
    uint32_t b = (uint32_t)(f[3] * 255.0f + 0.5f);

    sink_color = (a << 24) | (r << 16) | (g << 8) | b;
}

/* texlist corrente: texaddr de cada NJS_TEXNAME guarda o índice global do
 * sink (zstrip_tex_new), como o Ninja guarda o endereço da memlist */
static NJS_TEXLIST *cur_texlist;

Sint32 njSetTexture(NJS_TEXLIST *texlist)
{
    cur_texlist = texlist;
    return 0;
}

Sint32 njSetTextureNum(Uint32 n)
{
    if (cur_texlist == 0 || n >= cur_texlist->nbTexture) {
        zstrip_tex_select(-1);
        return -1;
    }
    zstrip_tex_select((int)cur_texlist->textures[n].texaddr);
    return 0;
}

Sint32 njSetkmTextureNum(Uint32 n)
{
    return njSetTextureNum(n);
}

/* PVM em memória -> uma textura GL por entrada; o texlist do jogo pode
 * vir com nbTexture/textures já preenchidos (só texaddr é escrito) */
static unsigned char tex_scratch[256 * 256 * 4];
static Pvm tex_pvm;
/* o Ninja não passa o tamanho do PVM; quem conhece o buffer o informa */
static int pvm_len_hint = 0x7fffffff;

Sint32 njLoadTexturePvmMemory(Uint8 *addr, NJS_TEXLIST *texlist)
{
    int count;
    int loaded = 0;

    if (addr == 0 || texlist == 0 || texlist->textures == 0) {
        return -1;
    }
    count = pvm_parse(addr, pvm_len_hint, &tex_pvm);
    if (count <= 0) {
        return -1;
    }
    if (texlist->nbTexture == 0 || texlist->nbTexture > (Uint32)count) {
        texlist->nbTexture = (Uint32)count;
    }
    for (Uint32 i = 0; i < texlist->nbTexture; i++) {
        const PvmTexture *t = &tex_pvm.tex[i];
        int idx = -1;

        if (pvr_decode_rgba8888(&t->pvr, t->data, t->data_len, tex_scratch,
                                (int)sizeof tex_scratch) > 0) {
            idx = zstrip_tex_new((int)t->pvr.width, (int)t->pvr.height,
                                 tex_scratch);
        }
        texlist->textures[i].texaddr = (Uint32)idx;
        loaded += idx >= 0;
    }
    return loaded > 0 ? 1 : -1;
}

/* texlist da cena de teste: o mesmo formato que o jogo usa */
static NJS_TEXNAME scene_names[16];
NJS_TEXLIST zscene_texlist = {scene_names, 16};

int zninja_load_scene_pvm(unsigned char *pvm, int len)
{
    int ok;

    pvm_len_hint = len;
    ok = njLoadTexturePvmMemory(pvm, &zscene_texlist);
    pvm_len_hint = 0x7fffffff;
    if (ok <= 0) {
        return 0;
    }
    return (int)zscene_texlist.nbTexture;
}

int zninja_scene_tex_count(void)
{
    return (int)zscene_texlist.nbTexture;
}
