/* njkm_zeebo.c — abertura/fecho de strip do Ninja sobre o sink GLES1.
 *
 * No Dreamcast estas funções montam o cabeçalho do strip e escrevem na store
 * queue (sdk/ninja/njkm.c). Aqui elas só abrem/fecham o strip do sink e
 * escolhem textura e blend. O njDraw.c real do decomp chama exatamente
 * estas entradas. */

#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "zstrip.h"

static Uint32 nj_listtype_storage;
static Uint32 *nj_listtype_ptr = &nj_listtype_storage;
Uint32 **_nj_listtype_ = &nj_listtype_ptr;

void zsprite_set_color(const Uint32 *color);

void njGetkmPointer(void)
{
}

void njSetkmPointer(void)
{
}

void njSetkmColorMode(Uint32 mode)
{
    (void)mode;
}

/* com o jogo: a lista de texturas do Ninja (_nj_texmemlist) diz qual
 * registro tem o índice global; a superfície dele guarda a textura GL
 * (ztexture.c). Sem o jogo (cena de teste), o índice é o do sink. */
extern NJS_TEXMEMLIST *_nj_texmemlist __attribute__((weak));
extern Uint32 _nj_texmemsize __attribute__((weak));
extern Uint32 _nj_current_texture __attribute__((weak));
Sint32 njSearchTexList(NJS_TEXMEMLIST *list, Uint32 n, Uint32 globalIndex)
    __attribute__((weak));
int ztexture_index(const void *surface);

static void select_record(Sint32 i, Uint32 globalIndex)
{
    if (i < 0 || (Uint32)i >= _nj_texmemsize) {
        zstrip_tex_select(-1);
        return;
    }
    if (&_nj_current_texture != 0) {
        _nj_current_texture = globalIndex;
    }
    zstrip_tex_select(ztexture_index(&_nj_texmemlist[i].texinfo.texsurface));
}

static int game_textures(void)
{
    return &_nj_texmemlist != 0 && _nj_texmemlist != 0 && njSearchTexList != 0;
}

/* o PVR pega a textura do cabeçalho do strip, que o Ninja escreve com a
 * textura corrente (_nj_current_texture); aqui o strip liga a textura GL dela */
static void bind_current(void)
{
    if (game_textures() && &_nj_current_texture != 0) {
        select_record(njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                      _nj_current_texture),
                      _nj_current_texture);
    }
}

void *njStartVertexNonTexOpaque(void)
{
    zstrip_begin();
    zstrip_no_texture();
    zstrip_blend(0);
    return 0;
}

/* o cabeçalho do strip leva os fatores de blend do contexto corrente */
extern struct tagKMVERTEXCONTEXT *_nj_current_ctx_ __attribute__((weak));

static void blend_current(void)
{
    if (&_nj_current_ctx_ != 0 && _nj_current_ctx_ != 0) {
        zstrip_blend_mode(_nj_current_ctx_->SRCBlendingMode,
                          _nj_current_ctx_->DSTBlendingMode);
    }
}

void *njStartVertexNonTexTrans(void)
{
    blend_current();
    zstrip_begin();
    zstrip_no_texture();
    zstrip_blend(1);
    return 0;
}

void *njStartVertexTexOpaque(void)
{
    zstrip_begin();
    bind_current();
    zstrip_use_texture();
    zstrip_blend(0);
    return 0;
}

void *njStartVertexTexTrans(void)
{
    blend_current();
    zstrip_begin();
    bind_current();
    zstrip_use_texture();
    zstrip_blend(1);
    return 0;
}

void njEndVertex(Uint32 *vertex)
{
    (void)vertex;
    zstrip_end();
}

Sint32 njSetkmTextureNumG(Uint32 globalIndex)
{
    if (game_textures()) {
        select_record(njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex),
                      globalIndex);
    } else {
        zstrip_tex_select((int)globalIndex);
    }
    zstrip_use_texture();
    return 1;
}

Sint32 njSetkmTextureMemList(Uint32 n)
{
    if (game_textures()) {
        select_record((Sint32)n, _nj_texmemlist[n].globalIndex);
        zstrip_use_texture();
        return 1;
    }
    return njSetkmTextureNumG(n);
}

/* sprites: o strip textured com a cor constante do sprite (intensidade 1) */
void *njStartVertexTexOpaqueIntens(Uint32 *color)
{
    zsprite_set_color(color);
    zstrip_begin();
    bind_current();
    zstrip_use_texture();
    zstrip_blend(0);
    return 0;
}

void *njStartVertexTexTransIntens(Uint32 *color)
{
    blend_current();
    zsprite_set_color(color);
    zstrip_begin();
    bind_current();
    zstrip_use_texture();
    zstrip_blend(1);
    return 0;
}
