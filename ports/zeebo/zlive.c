/* Os modelos 3D do SA1 no sink GLES (zstrip.c).
 *
 * src/draw/model.c (a camada que substitui model.src) entrega cada desenho a
 * sa_present_live; no Dreamcast quem atende é engine/dc/sa1/model.c, que
 * escreve na store queue do PowerVR. Aqui cada ponto do modelo passa pela
 * matriz corrente do Ninja (njCalcPoint), é projetado como o jogo projeta
 * (x' = cx + x * dist * aspect_x / z, z positivo à frente) e vai para o sink
 * como strip de tela, com z = 1/profundidade, igual aos draws 2D.
 *
 * Primeira versão: sem paleta do SA1 (a cor é a difusa do material vezes um
 * Lambert simples com a luz de draw_set_light), sem recorte no near (um
 * triângulo com um ponto atrás do near é descartado), sem culling de face.
 *
 * Também aqui: njEasyDrawObject/njEasyDrawModel (njModelE.c fica fora do
 * build) e sprite_draw_3d (engine/dc/sprite.c usa a store queue). */
#include <kamui.h>
#include <ninja.h>

#include "zstrip.h"

float sqrtf(float);

typedef struct sa_engine sa_engine;
typedef struct sa_draw_live {
    const void *geometry;
    const void *state;
    Uint32 format;
    Uint32 flags;
} sa_draw_live;

void zlog(const char *what, const char *name, int value);
int ztexture_index(const void *surface);

extern NJS_SCREEN _nj_screen_;
extern Float _nj_near_clip_;
extern Float _nj_far_clip_;
extern Float _nj_scr_aspect_y_;
extern NJS_TEXLIST *_nj_current_texlist;
extern Uint32 _nj_control_3d_flag_;
extern Uint32 _nj_constant_attr_and_;
extern Uint32 _nj_constant_attr_or_;
extern NJS_VECTOR sa1_live_light;

/* projeção: o que sa1_live_screen copia */
static Float scale_x = 1.0F, scale_y = 1.0F;

void sa1_live_screen(Float aspect_x, void *scratch)
{
    (void)scratch;
    scale_x = aspect_x * _nj_screen_.dist;
    scale_y = _nj_scr_aspect_y_ * _nj_screen_.dist;
}

void sa1_live_run(void) {}
void sa1_live_forms(Bool on) { (void)on; }
void sa1_live_forget(void) {}

#define MAX_POINTS 2048

/* cada ponto do modelo em tela; ok = à frente do near */
struct zpoint {
    float x, y, ooz;
    int ok;
    float shade;
};
static struct zpoint pts[MAX_POINTS];

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static void project_model(const NJS_MODEL *model)
{
    int i;
    int n = model->nbPoint < MAX_POINTS ? model->nbPoint : MAX_POINTS;
    NJS_VECTOR l = sa1_live_light;
    float ll = l.x * l.x + l.y * l.y + l.z * l.z;
    float invl = ll > 0.0F ? 1.0F / sqrtf(ll) : 0.0F;

    for (i = 0; i < n; i++) {
        NJS_POINT3 v;
        struct zpoint *p = &pts[i];

        njCalcPoint(NULL, &model->points[i], &v);
        p->ok = v.z > _nj_near_clip_;
        if (p->ok) {
            float inv = 1.0F / v.z;

            p->ooz = inv;
            p->x = _nj_screen_.cx + v.x * scale_x * inv;
            p->y = _nj_screen_.cy + v.y * scale_y * inv;
        }
        p->shade = 1.0F;
        if (model->normals != NULL && ll > 0.0F) {
            NJS_VECTOR nv;
            float d;

            njCalcVector(NULL, &model->normals[i], &nv);
            d = -(nv.x * l.x + nv.y * l.y + nv.z * l.z);
            p->shade = 0.45F + 0.55F * clampf(d * invl, 0.0F, 1.0F);
        }
    }
}

static Uint32 material_flags(const NJS_MATERIAL *m)
{
    Uint32 f = m->attrflags;

    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR) {
        f = (f & _nj_constant_attr_and_) | _nj_constant_attr_or_;
    }
    return f;
}

/* liga a textura n do texlist corrente (como njSetTextureNum, sem o PVR) */
static int bind_texture(Uint32 n)
{
    NJS_TEXMEMLIST *mem;

    if (_nj_current_texlist == NULL || n >= _nj_current_texlist->nbTexture) {
        return 0;
    }
    mem = (NJS_TEXMEMLIST *)_nj_current_texlist->textures[n].texaddr;
    if (mem == NULL) {
        return 0;
    }
    zstrip_tex_select(ztexture_index(&mem->texinfo.texsurface));
    return 1;
}

static Uint32 shade_color(Uint32 argb, float s)
{
    Uint32 r = (Uint32)(((argb >> 16) & 0xff) * s);
    Uint32 g = (Uint32)(((argb >> 8) & 0xff) * s);
    Uint32 b = (Uint32)((argb & 0xff) * s);

    return (argb & 0xff000000u) | (r << 16) | (g << 8) | b;
}

/* normal de face calculada na hora (os modelos do preview nao trazem
 * normais: model->normals e zero e meshset->normals e NULL) */
static float face_shade(const NJS_MODEL *model, const Sint16 *idx)
{
    const NJS_POINT3 *a = &model->points[idx[0] & 0x7fff];
    const NJS_POINT3 *b = &model->points[idx[1] & 0x7fff];
    const NJS_POINT3 *c = &model->points[idx[2] & 0x7fff];
    NJS_VECTOR e1, e2, n, nv, l = sa1_live_light;
    float ll = l.x * l.x + l.y * l.y + l.z * l.z;
    float invl = ll > 0.0F ? 1.0F / sqrtf(ll) : 0.0F;
    float len, d;

    e1.x = b->x - a->x; e1.y = b->y - a->y; e1.z = b->z - a->z;
    e2.x = c->x - a->x; e2.y = c->y - a->y; e2.z = c->z - a->z;
    n.x = e1.y * e2.z - e1.z * e2.y;
    n.y = e1.z * e2.x - e1.x * e2.z;
    n.z = e1.x * e2.y - e1.y * e2.x;
    len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len < 1e-6F) return 1.0F;
    n.x /= len; n.y /= len; n.z /= len;
    njCalcVector(NULL, &n, &nv);
    d = -(nv.x * l.x + nv.y * l.y + nv.z * l.z);
    return 0.45F + 0.55F * clampf(d * invl, 0.0F, 1.0F);
}

/* um polígono de count índices em ordem de strip */
static void send_strip(const Sint16 *idx, int count, const NJS_COLOR *uv,
                       const NJS_COLOR *vc, Uint32 diffuse, int nbPoint,
                       int textured, float face)
{
    int i;

    for (i = 0; i < count; i++) {
        int k = idx[i] & 0x7fff;

        if (k >= nbPoint || k >= MAX_POINTS || !pts[k].ok) {
            return;
        }
    }
    zstrip_begin();
    for (i = 0; i < count; i++) {
        int k = idx[i] & 0x7fff;
        const struct zpoint *p = &pts[k];
        Uint32 c = vc != NULL ? vc[i].color : diffuse;
        float u = 0.0F, v = 0.0F;

        if (textured && uv != NULL) {
            u = uv[i].tex.u * (1.0F / 256.0F);
            v = uv[i].tex.v * (1.0F / 256.0F);
        }
        zstrip_vertex(p->x, p->y, p->ooz, u, v, shade_color(c, face >= 0.0F ? face : p->shade));
    }
    zstrip_end();
}

static void draw_meshset(const NJS_MODEL *model, const NJS_MESHSET *ms)
{
    const NJS_MATERIAL *mat = &model->mats[ms->type_matId & 0x3fff];
    Uint32 flags = material_flags(mat);
    int type = ms->type_matId >> 14;
    const Sint16 *m = ms->meshes;
    const NJS_COLOR *uv = ms->vertuv;
    const NJS_COLOR *vc = ms->vertcolor;
    int textured = (flags & NJD_FLAG_USE_TEXTURE) && bind_texture(mat->attr_texId & 0xffff);
    int alpha = (flags & NJD_FLAG_USE_ALPHA) != 0;
    int i;

    if (textured) {
        zstrip_use_texture();
    } else {
        zstrip_no_texture();
    }
    zstrip_blend(alpha);
    if (alpha) {
        zstrip_blend_mode((flags >> 29) & 7, (flags >> 26) & 7);
    }
    for (i = 0; i < ms->nbMesh; i++) {
        int count;
        Sint16 tmp[3];

        switch (type) {
        case 0: /* triângulos */
            count = 3;
            send_strip(m, 3, uv, vc, mat->diffuse.color, model->nbPoint, textured, face_shade(model, m));
            break;
        case 1: /* quads: já em ordem de strip (engine/dc/sa1/model.c manda
                 * 0 1 2 3 direto, polygon_sizes[1] = 4) */
            count = 4;
            send_strip(m, 4, uv, vc, mat->diffuse.color, model->nbPoint, textured, face_shade(model, m));
            break;
        default: /* N e strip: a palavra de cabeça traz a contagem */
            count = m[0] & 0x3fff;
            m++;
            send_strip(m, count, uv, vc, mat->diffuse.color, model->nbPoint, textured, face_shade(model, m));
            break;
        }
        (void)tmp;
        m += count;
        if (uv) uv += count;
        if (vc) vc += count;
    }
}

static int model_in_view(const NJS_MODEL *model)
{
    NJS_POINT3 c;
    float r = model->r;

    njCalcPoint(NULL, (NJS_POINT3 *)&model->center, &c);
    return c.z > _nj_near_clip_ - r && c.z < _nj_far_clip_ + r;
}

static void draw_basic(const NJS_MODEL *model, int cull)
{
    int i;

    if (model == NULL || model->points == NULL || model->meshsets == NULL) {
        return;
    }
    if (cull && !model_in_view(model)) {
        return;
    }
    project_model(model);
    for (i = 0; i < model->nbMeshset; i++) {
        draw_meshset(model, &model->meshsets[i]);
    }
    zstrip_blend(0);
}

int sa_present_live(sa_engine *engine, const sa_draw_live *draw)
{
    Uint32 how;
    static int seen;

    (void)engine;
    if (!seen) {
        seen = 1;
        zlog("live", "primeiro modelo", (int)draw->geometry);
    }
    how = *(const Uint32 *)draw->state;
    /* SA1_LIVE_NESTED 0, SA1_LIVE_MODEL 1, SA1_LIVE_LAND 2 */
    draw_basic((const NJS_MODEL *)draw->geometry, how == 1);
    return 0;
}

/* --- Ninja "easy" (njModelE.c fica fora do build) --- */

void njEasyDrawModel(NJS_MODEL *model)
{
    draw_basic(model, 1);
}

static void easy_walk(NJS_OBJECT *o)
{
    for (; o != NULL; o = o->sibling) {
        Uint32 f = o->evalflags;

        njPushMatrix(NULL);
        if (!(f & NJD_EVAL_UNIT_POS)) {
            njTranslate(NULL, o->pos[0], o->pos[1], o->pos[2]);
        }
        if (!(f & NJD_EVAL_UNIT_ANG)) {
            if (f & NJD_EVAL_ZXY_ANG) {
                njRotateZ(NULL, o->ang[2]);
                njRotateX(NULL, o->ang[0]);
                njRotateY(NULL, o->ang[1]);
            } else {
                njRotateXYZ(NULL, o->ang[0], o->ang[1], o->ang[2]);
            }
        }
        if (!(f & NJD_EVAL_UNIT_SCL)) {
            njScale(NULL, o->scl[0], o->scl[1], o->scl[2]);
        }
        if (!(f & NJD_EVAL_HIDE) && o->model != NULL) {
            draw_basic(o->model, 1);
        }
        if (!(f & NJD_EVAL_BREAK)) {
            easy_walk(o->child);
        }
        njPopMatrix(1);
    }
}

void njEasyDrawObject(NJS_OBJECT *object)
{
    easy_walk(object);
}

void njSetEasyLight(Float x, Float y, Float z) { (void)x; (void)y; (void)z; }
void njSetEasyLightIntensity(Float i, Float a) { (void)i; (void)a; }
void njSetEasyLightColor(Float r, Float g, Float b) { (void)r; (void)g; (void)b; }

/* --- sprites 3D (engine/dc/sprite.c escreve na store queue) --- */

static Float sprite_depth;

void sprite_set_depth(Float depth) { sprite_depth = depth; }
Float sprite_get_depth(void) { return sprite_depth; }

void sprite_draw_3d(NJS_SPRITE *sprite, Int entry, Uint32 attr)
{
    NJS_POINT3 v;
    const NJS_TEXANIM *t;
    float inv, sx, sy, x0, y0, x1, y1, u1, v1, u2, v2;
    Uint32 c = 0xffffffffu;

    if (sprite == NULL || sprite->tanim == NULL || event_skipping) {
        return;
    }
    t = &sprite->tanim[entry];
    njCalcPoint(NULL, &sprite->p, &v);
    v.z += sprite_depth;
    if (!(v.z > _nj_near_clip_)) {
        return;
    }
    inv = 1.0F / v.z;
    sx = sprite->sx * scale_x * inv;
    sy = sprite->sy * scale_y * inv;
    x0 = _nj_screen_.cx + v.x * scale_x * inv - t->cx * sx;
    y0 = _nj_screen_.cy + v.y * scale_y * inv - t->cy * sy;
    x1 = x0 + t->sx * sx;
    y1 = y0 + t->sy * sy;
    u1 = t->u1 * (1.0F / 256.0F);
    v1 = t->v1 * (1.0F / 256.0F);
    u2 = t->u2 * (1.0F / 256.0F);
    v2 = t->v2 * (1.0F / 256.0F);
    if (attr & NJD_SPRITE_HFLIP) { float s = u1; u1 = u2; u2 = s; }
    if (attr & NJD_SPRITE_VFLIP) { float s = v1; v1 = v2; v2 = s; }
    if (attr & NJD_SPRITE_COLOR) {
        extern NJS_ARGB _nj_constant_material_;
        NJS_ARGB *m = &_nj_constant_material_;

        c = ((Uint32)(clampf(m->a, 0, 1) * 255) << 24) |
            ((Uint32)(clampf(m->r, 0, 1) * 255) << 16) |
            ((Uint32)(clampf(m->g, 0, 1) * 255) << 8) |
            (Uint32)(clampf(m->b, 0, 1) * 255);
    }
    {
        NJS_TEXLIST *saved = _nj_current_texlist;

        _nj_current_texlist = sprite->tlist;
        if (bind_texture((Uint32)t->texid)) {
            zstrip_use_texture();
        } else {
            zstrip_no_texture();
        }
        _nj_current_texlist = saved;
    }
    zstrip_blend((attr & NJD_SPRITE_ALPHA) != 0);
    zstrip_begin();
    zstrip_vertex(x0, y0, inv, u1, v1, c);
    zstrip_vertex(x1, y0, inv, u2, v1, c);
    zstrip_vertex(x0, y1, inv, u1, v2, c);
    zstrip_vertex(x1, y1, inv, u2, v2, c);
    zstrip_end();
    zstrip_blend(0);
}
