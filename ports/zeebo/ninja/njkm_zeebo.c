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

void *njStartVertexNonTexOpaque(void)
{
    zstrip_begin();
    zstrip_no_texture();
    zstrip_blend(0);
    return 0;
}

void *njStartVertexNonTexTrans(void)
{
    zstrip_begin();
    zstrip_no_texture();
    zstrip_blend(1);
    return 0;
}

void *njStartVertexTexOpaque(void)
{
    zstrip_begin();
    zstrip_use_texture();
    zstrip_blend(0);
    return 0;
}

void *njStartVertexTexTrans(void)
{
    zstrip_begin();
    zstrip_use_texture();
    zstrip_blend(1);
    return 0;
}

void njEndVertex(Uint32 *vertex)
{
    (void)vertex;
    zstrip_end();
}

/* o índice global é o do sink (zstrip_tex_new) */
Sint32 njSetkmTextureNumG(Uint32 globalIndex)
{
    zstrip_tex_select((int)globalIndex);
    zstrip_use_texture();
    return 0;
}

Sint32 njSetkmTextureMemList(Uint32 n)
{
    return njSetkmTextureNumG(n);
}

/* sprites: o strip textured com a cor constante do sprite (intensidade 1) */
void *njStartVertexTexOpaqueIntens(Uint32 *color)
{
    zsprite_set_color(color);
    zstrip_begin();
    zstrip_use_texture();
    zstrip_blend(0);
    return 0;
}

void *njStartVertexTexTransIntens(Uint32 *color)
{
    zsprite_set_color(color);
    zstrip_begin();
    zstrip_use_texture();
    zstrip_blend(1);
    return 0;
}
