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
void zlogd(const char *what, const char *name, int value);

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
    float vx, vy, vz; /* em view space, para o recorte no near */
};
static struct zpoint pts[MAX_POINTS];

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

float *native_xmtrx(void);

/* a luz do jogo muda raramente: |l|^2 e 1/|l| uma vez por mudanca, nao por modelo/face */
static void light_get(NJS_VECTOR *l, float *ll, float *invl)
{
    static unsigned bx, by, bz;
    static float s_ll, s_inv;
    static int have;
    const unsigned *b = (const unsigned *)&sa1_live_light;

    if (!have || b[0] != bx || b[1] != by || b[2] != bz) {
        have = 1;
        bx = b[0];
        by = b[1];
        bz = b[2];
        s_ll = sa1_live_light.x * sa1_live_light.x + sa1_live_light.y * sa1_live_light.y +
               sa1_live_light.z * sa1_live_light.z;
        s_inv = s_ll > 0.0F ? 1.0F / sqrtf(s_ll) : 0.0F;
    }
    *l = sa1_live_light;
    *ll = s_ll;
    *invl = s_inv;
}

/* O desenhador "simple" do Ninja (sdk/ninja/njModelS.c) testa a esfera do modelo com o
 * centro na matriz corrente; no Dreamcast isso e uma funcao em assembly SH-4, que aqui
 * nao existe e virava uma armadilha que devolvia 0 sem escrever a saida: o centro ficava
 * com lixo, o teste de visao descartava o modelo e a cena saia preta (Zero, Twinkle). */
void SimpleCalcModelMatrix(NJS_POINT3 *out, NJS_POINT3 *from)
{
    const float *m = native_xmtrx();
    float x = from->x, y = from->y, z = from->z;

    out->x = m[0] * x + m[4] * y + m[8] * z + m[12];
    out->y = m[1] * x + m[5] * y + m[9] * z + m[13];
    out->z = m[2] * x + m[6] * y + m[10] * z + m[14];
}

static void project_model(const NJS_MODEL *model)
{
    const float *m = native_xmtrx();
    int i;
    int n = model->nbPoint < MAX_POINTS ? model->nbPoint : MAX_POINTS;
    NJS_VECTOR l;
    float ll, invl;

    light_get(&l, &ll, &invl);
    if (model->nbPoint > MAX_POINTS) {
        static int most;

        if (model->nbPoint > most) { /* os strips acima de MAX_POINTS somem */
            most = model->nbPoint;
            zlog("vlive", "modelo com pontos demais", most);
        }
    }
    for (i = 0; i < n; i++) {
        NJS_POINT3 v;
        struct zpoint *p = &pts[i];

        {
            const NJS_POINT3 *q = &model->points[i];

            /* ftrv: a matriz corrente em colunas (native_xmtrx), inline em
             * vez de uma chamada por ponto */
            v.x = m[0] * q->x + m[4] * q->y + m[8] * q->z + m[12];
            v.y = m[1] * q->x + m[5] * q->y + m[9] * q->z + m[13];
            v.z = m[2] * q->x + m[6] * q->y + m[10] * q->z + m[14];
        }
        p->vx = v.x;
        p->vy = v.y;
        p->vz = v.z;
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

            const NJS_VECTOR *n = &model->normals[i];

            nv.x = m[0] * n->x + m[4] * n->y + m[8] * n->z;
            nv.y = m[1] * n->x + m[5] * n->y + m[9] * n->z;
            nv.z = m[2] * n->x + m[6] * n->y + m[10] * n->z;
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

    static int shown;

    if (_nj_current_texlist == NULL || n >= _nj_current_texlist->nbTexture) {
        if (shown < 30) {
            shown++;
            zlog("vbind", _nj_current_texlist == NULL ? "sem texlist" : "n fora", (int)n);
        }
        return 0;
    }
    mem = (NJS_TEXMEMLIST *)_nj_current_texlist->textures[n].texaddr;
    if (mem == NULL) {
        if (shown < 30) {
            shown++;
            zlog("vbind", "textura nao carregada", (int)n | ((int)_nj_current_texlist->nbTexture << 16));
        }
        return 0;
    }
    if (_nj_current_texlist->nbTexture == 90 && n == 1) {
        static int shown1;

        if (shown1 < 3) {
            static const char *const bn[3] = {"b0", "b1", "b2"};

            zlog("vlive", bn[shown1], ztexture_index(&mem->texinfo.texsurface));
            shown1++;
        }
    }
    zstrip_tex_select(ztexture_index(&mem->texinfo.texsurface));
    return 1;
}

/* USE_ALPHA com mistura alfa/1-alfa e tudo alfa 255 (material, sem cor por vertice, textura sem
 * texel translucido) sai igual a um opaco: o jogo liga USE_ALPHA em quase todo material, e o
 * caminho de CPU dos translucidos (projeta, sombreia e ordena em float) custava ~25% do tempo */

static Uint32 mat_color(const NJS_MATERIAL *mat, Uint32 flags, int textured);

/* 0 = opaco de fato; senao o motivo (2 cor por vertice, 3 mistura, 4 alfa do difuso, 5 material
 * constante, 6 textura com alfa ou ausente) */
static int alpha_opaque(const NJS_MODEL *model, const NJS_MESHSET *ms, Uint32 flags, int *cutout)
{
    const NJS_MATERIAL *mat = &model->mats[ms->type_matId & 0x3fff];

    if (ms->vertcolor != NULL) {
        return 2;
    }
    if (((flags >> 29) & 7) != 4 || ((flags >> 26) & 7) != 5) {
        return 3;
    }
    /* o alfa final do material (o difuso, ou o do jogo com CONSTANT/OFFSET_MATERIAL) */
    if ((mat_color(mat, flags, 0) >> 24) != 0xffu) {
        return 4;
    }
    if (flags & NJD_FLAG_USE_TEXTURE) {
        Uint32 n = mat->attr_texId & 0xffff;
        NJS_TEXMEMLIST *mem;

        if (_nj_current_texlist == NULL || n >= _nj_current_texlist->nbTexture) {
            return 9;
        }
        mem = (NJS_TEXMEMLIST *)_nj_current_texlist->textures[n].texaddr;
        if (mem == NULL) {
            return 8;
        }
        {
            int cls = zstrip_tex_opaque(ztexture_index(&mem->texinfo.texsurface));

            if (cls == 2) {
                *cutout = 1;
            }
            if (cls != 0) {
                return 0;
            }
        }
        return 6;
    }
    return 0;
}

/* a cor-base de um material, como _njDrawModelSub monta _nj_light_material_:
 * o difuso, ou a constante do jogo (CONSTANT_MATERIAL) ou somada a ele
 * (OFFSET_MATERIAL); textura com CONSTANT_TEXTURE_MATERIAL parte de branco; e
 * um material que ignora a luz com textura sai branco, só com o alfa */
static Uint32 mat_color(const NJS_MATERIAL *mat, Uint32 flags, int textured)
{
    extern NJS_ARGB _nj_constant_material_;
    float c[4]; /* a r g b em 0..1 */
    Uint32 d = mat->diffuse.color;
    Uint32 out = 0;
    int i;

    /* o caso comum (sem material constante e sem textura que ignora a luz) devolve o difuso:
     * antes as quatro conversoes para float rodavam em todo meshset mesmo assim */
    if (!(_nj_control_3d_flag_ & (NJD_CONTROL_3D_CONSTANT_MATERIAL |
                                  NJD_CONTROL_3D_OFFSET_MATERIAL |
                                  NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL)) &&
        !(textured && (flags & NJD_FLAG_IGNORE_LIGHT))) {
        return d;
    }
    c[0] = (float)(d >> 24) * (1.0F / 255.0F);
    c[1] = (float)((d >> 16) & 0xff) * (1.0F / 255.0F);
    c[2] = (float)((d >> 8) & 0xff) * (1.0F / 255.0F);
    c[3] = (float)(d & 0xff) * (1.0F / 255.0F);
    if (!(_nj_control_3d_flag_ & (NJD_CONTROL_3D_CONSTANT_MATERIAL |
                                  NJD_CONTROL_3D_OFFSET_MATERIAL |
                                  NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL)) &&
        !(textured && (flags & NJD_FLAG_IGNORE_LIGHT))) {
        return d;
    }
    if (textured && (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL)) {
        c[1] = c[2] = c[3] = 1.0F;
    }
    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_MATERIAL) {
        c[0] = _nj_constant_material_.a;
        c[1] = _nj_constant_material_.r;
        c[2] = _nj_constant_material_.g;
        c[3] = _nj_constant_material_.b;
    } else if (_nj_control_3d_flag_ & NJD_CONTROL_3D_OFFSET_MATERIAL) {
        c[0] += _nj_constant_material_.a;
        c[1] += _nj_constant_material_.r;
        c[2] += _nj_constant_material_.g;
        c[3] += _nj_constant_material_.b;
    }
    if (textured && (flags & NJD_FLAG_IGNORE_LIGHT)) {
        c[1] = c[2] = c[3] = 1.0F;
    }
    for (i = 0; i < 4; i++) {
        float v = c[i] < 0.0F ? 0.0F : (c[i] > 1.0F ? 1.0F : c[i]);

        out = out << 8 | (Uint32)(v * 255.0F);
    }
    return out;
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
    NJS_VECTOR e1, e2, n, nv, l;
    float ll, invl;
    float len, d;

    light_get(&l, &ll, &invl);

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

/* um vértice do recorte: view space, uv e cor (ARGB em float) */
struct cvert {
    float x, y, z, u, v, c[4];
};

static void cvert_of(struct cvert *o, int k, const NJS_COLOR *uv,
                     const NJS_COLOR *vc, int i, Uint32 diffuse, int textured,
                     float shade)
{
    Uint32 c = shade_color(vc != NULL ? vc[i].color : diffuse, shade);

    o->x = pts[k].vx;
    o->y = pts[k].vy;
    o->z = pts[k].vz;
    o->u = textured && uv != NULL ? uv[i].tex.u * (1.0F / 256.0F) : 0.0F;
    o->v = textured && uv != NULL ? uv[i].tex.v * (1.0F / 256.0F) : 0.0F;
    o->c[0] = (float)((c >> 24) & 0xff);
    o->c[1] = (float)((c >> 16) & 0xff);
    o->c[2] = (float)((c >> 8) & 0xff);
    o->c[3] = (float)(c & 0xff);
}

static void emit(const struct cvert *p)
{
    float inv = 1.0F / p->z;
    Uint32 c = ((Uint32)p->c[0] << 24) | ((Uint32)p->c[1] << 16) |
               ((Uint32)p->c[2] << 8) | (Uint32)p->c[3];

    zstrip_vertex(_nj_screen_.cx + p->x * scale_x * inv,
                  _nj_screen_.cy + p->y * scale_y * inv, inv, p->u, p->v, c);
}

/* o triângulo a b c recortado em z = near (Sutherland-Hodgman, um plano):
 * sai como strip de 3 ou 4 vértices. O PVR recorta no hardware; sem isto o
 * chão perto da câmera, com pontos atrás do near, sumia inteiro (preto) */
static void clip_triangle(const struct cvert *a, const struct cvert *b,
                          const struct cvert *c)
{
    const struct cvert *in[3];
    struct cvert out[4];
    float n = _nj_near_clip_ + 0.001F;
    int i, m = 0;

    in[0] = a; in[1] = b; in[2] = c;
    for (i = 0; i < 3; i++) {
        const struct cvert *p = in[i], *q = in[(i + 1) % 3];
        int pin = p->z > n, qin = q->z > n;

        if (pin) {
            out[m++] = *p;
        }
        if (pin != qin) {
            float t = (n - p->z) / (q->z - p->z);
            struct cvert *r = &out[m++];
            int j;

            r->x = p->x + (q->x - p->x) * t;
            r->y = p->y + (q->y - p->y) * t;
            r->z = n;
            r->u = p->u + (q->u - p->u) * t;
            r->v = p->v + (q->v - p->v) * t;
            for (j = 0; j < 4; j++) {
                r->c[j] = p->c[j] + (q->c[j] - p->c[j]) * t;
            }
        }
    }
    if (m < 3) {
        return;
    }
    zstrip_begin();
    emit(&out[0]);
    emit(&out[1]);
    emit(&out[m - 1]); /* polígono 0 1 2 [3] em ordem de strip: 0 1 3 2 */
    if (m == 4) {
        emit(&out[2]);
    }
    zstrip_end();
}

/* um polígono de count índices em ordem de strip */
static void send_strip(const Sint16 *idx, int count, const NJS_COLOR *uv,
                       const NJS_COLOR *vc, Uint32 diffuse, int nbPoint,
                       int textured, float face)
{
    int i;
    int near = 0;

    for (i = 0; i < count; i++) {
        int k = idx[i] & 0x7fff;

        if (k >= nbPoint || k >= MAX_POINTS) {
            return;
        }
        near |= !pts[k].ok;
    }
    if (near) {
        /* triângulo a triângulo do strip, cada um recortado */
        for (i = 0; i + 2 < count; i++) {
            struct cvert v[3];
            int j;

            for (j = 0; j < 3; j++) {
                int k = idx[i + j] & 0x7fff;

                cvert_of(&v[j], k, uv, vc, i + j, diffuse, textured,
                         face >= 0.0F ? face : pts[k].shade);
            }
            clip_triangle(&v[0], &v[1], &v[2]);
        }
        return;
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
    Uint32 base = mat_color(mat, flags, textured);
    int i;

    if (textured) {
        zstrip_use_texture();
    } else {
        zstrip_no_texture();
    }
    zstrip_blend(alpha);
    if (alpha) {
        zstrip_blend_pvr((flags >> 29) & 7, (flags >> 26) & 7);
    }
    for (i = 0; i < ms->nbMesh; i++) {
        int count;
        Sint16 tmp[3];

        switch (type) {
        case 0: /* triângulos */
            count = 3;
            send_strip(m, 3, uv, vc, base, model->nbPoint, textured, (model->normals != NULL ? face_shade(model, m) : 1.0F));
            break;
        case 1: /* quads: já em ordem de strip (engine/dc/sa1/model.c manda
                 * 0 1 2 3 direto, polygon_sizes[1] = 4) */
            count = 4;
            send_strip(m, 4, uv, vc, base, model->nbPoint, textured, (model->normals != NULL ? face_shade(model, m) : 1.0F));
            break;
        default: /* N e strip: a palavra de cabeça traz a contagem */
            count = m[0] & 0x3fff;
            m++;
            send_strip(m, count, uv, vc, base, model->nbPoint, textured, (model->normals != NULL ? face_shade(model, m) : 1.0F));
            break;
        }
        (void)tmp;
        m += count;
        if (uv) uv += count;
        if (vc) vc += count;
    }
}

/* --- caminho GL: pontos em GL_FIXED guardados por modelo ---
 * O jogo não muda os pontos de um modelo no meio da vida dele (menos os
 * deformáveis, que a soma de verificação pega), então cada modelo é
 * convertido uma vez, e cada quadro só copia inteiros. */
void *malloc(unsigned int n);
#define MC_SLOTS 4096
#define MC_ARENA (2u << 20) /* cache de pontos convertidos: o pico medido em 75 s foi 3,8 MB (praia) com 6 MB; com 2 MB ele reinicia a cada ~40 s e reconverte so o que aparece */
struct mcache {
    const NJS_MODEL *model;
    int n;
    Uint32 sum;
    int *pts;          /* n x 3, 16.16 */
    signed char *nrm;  /* n x 4 (xyz, 0) ou NULL */
};
static struct mcache mc[MC_SLOTS];
/* arena no heap (primeiro uso): 6 MB de .bss iam zerados dentro do .mod */
static unsigned char *mc_arena;
static unsigned int mc_used;
#ifndef ZLIVE_TDEFER
#define ZLIVE_TDEFER 1 /* 0: translucidos pelo caminho de CPU (A/B) */
#endif
#ifndef ZLIVE_NOSHADE
#define ZLIVE_NOSHADE 0
#endif
static int zlive_gl = 1; /* 0: tudo pelo caminho de CPU (A/B) */

static int fixf(float v)
{
    if (v > 32767.0F) {
        v = 32767.0F;
    } else if (v < -32767.0F) {
        v = -32767.0F;
    }
    return (int)(v * 65536.0F);
}

static Uint32 model_sum(const NJS_MODEL *model)
{
    const Uint32 *a = (const Uint32 *)&model->points[0];
    const Uint32 *b = (const Uint32 *)&model->points[model->nbPoint / 2];
    const Uint32 *c = (const Uint32 *)&model->points[model->nbPoint - 1];

    return a[0] ^ (a[1] << 1) ^ (a[2] << 2) ^ b[0] ^ (b[1] << 1) ^ (b[2] << 2) ^
           c[0] ^ (c[1] << 1) ^ (c[2] << 2) ^ ((Uint32)model->nbPoint << 16);
}

static struct mcache *mcache_get(const NJS_MODEL *model)
{
    unsigned int h = ((unsigned int)(unsigned long)model >> 4) % MC_SLOTS;
    int probe, n = model->nbPoint, i;
    Uint32 sum;
    struct mcache *e = 0;
    unsigned int need;

    if (n <= 0) {
        return 0;
    }
    sum = model_sum(model);
    for (probe = 0; probe < 16; probe++) {
        struct mcache *c = &mc[(h + probe) % MC_SLOTS];

        if (c->model == model) {
            if (c->sum == sum && c->n == n) {
                return c;
            }
            e = c; /* mudou: reconverte no mesmo lugar */
            break;
        }
        if (c->model == 0 && e == 0) {
            e = c;
        }
    }
    need = (unsigned int)n * 12u + (model->normals != NULL ? (unsigned int)n * 4u : 0u);
    if (mc_arena == 0) {
        mc_arena = (unsigned char *)malloc(MC_ARENA);
        if (mc_arena == 0) {
            return 0;
        }
    }
    if (e == 0 || mc_used + need > MC_ARENA) {
        /* cheio: recomeça (a próxima passada reconverte o que usar) */
        for (i = 0; i < MC_SLOTS; i++) {
            mc[i].model = 0;
        }
        mc_used = 0;
        if (need > MC_ARENA) {
            return 0;
        }
        e = &mc[h];
    }
    e->model = model;
    e->n = n;
    e->sum = sum;
    e->pts = (int *)(mc_arena + mc_used);
    mc_used += (unsigned int)n * 12u;
    for (i = 0; i < n; i++) {
        e->pts[i * 3 + 0] = fixf(model->points[i].x);
        e->pts[i * 3 + 1] = fixf(model->points[i].y);
        e->pts[i * 3 + 2] = fixf(model->points[i].z);
    }
    e->nrm = 0;
    if (model->normals != NULL) {
        e->nrm = (signed char *)(mc_arena + mc_used);
        mc_used += (unsigned int)n * 4u;
        for (i = 0; i < n; i++) {
            e->nrm[i * 4 + 0] = (signed char)(model->normals[i].x * 127.0F);
            e->nrm[i * 4 + 1] = (signed char)(model->normals[i].y * 127.0F);
            e->nrm[i * 4 + 2] = (signed char)(model->normals[i].z * 127.0F);
            e->nrm[i * 4 + 3] = 0;
        }
    }
    return e;
}

float *native_xmtrx(void);

/* projeção: x_clip = sx/320 vx + (cx-320)/320 vz, y_clip = -sy/240 vy -
 * (cy-240)/240 vz, z_clip = -1, w = vz. -1/z é o z do passe 2D */
static void build_proj(int *p)
{
    int i;

    for (i = 0; i < 16; i++) {
        p[i] = 0;
    }
    p[0] = fixf(scale_x * (1.0F / 320.0F));
    p[8] = fixf((_nj_screen_.cx - 320.0F) * (1.0F / 320.0F));
    p[5] = fixf(-scale_y * (1.0F / 240.0F));
    p[9] = fixf(-(_nj_screen_.cy - 240.0F) * (1.0F / 240.0F));
    p[14] = -(1 << 16);
    p[11] = 1 << 16;
}

/* os meshes opacos de um modelo pelo GL; devolve 1 se desenhou */
static void draw_meshset_gl(const NJS_MODEL *model, const NJS_MESHSET *ms,
                            const struct mcache *c, const int *lo, int cut)
{
    const NJS_MATERIAL *mat = &model->mats[ms->type_matId & 0x3fff];
    Uint32 flags = material_flags(mat);
    int type = ms->type_matId >> 14;
    const Sint16 *m = ms->meshes;
    const NJS_COLOR *uv = ms->vertuv;
    const NJS_COLOR *vc = ms->vertcolor;
    int textured = (flags & NJD_FLAG_USE_TEXTURE) && bind_texture(mat->attr_texId & 0xffff);
    Uint32 diffuse = mat_color(mat, flags, textured);
    int i;

    if (textured) {
        zstrip_use_texture();
    } else {
        zstrip_no_texture();
    }
    zstrip_blend(0);
    if (cut) {
        zstrip_cutout(1);
    }
    for (i = 0; i < ms->nbMesh; i++) {
        int count, j;

        switch (type) {
        case 0:
            count = 3;
            break;
        case 1:
            count = 4;
            break;
        default:
            count = m[0] & 0x3fff;
            m++;
            break;
        }
        for (j = 0; j < count; j++) {
            int k = m[j] & 0x7fff;

            if (k >= c->n) {
                break;
            }
            Uint32 col = vc != NULL ? vc[j].color : diffuse;

            if (lo != 0) {
                /* sombreamento 0,45 + 0,55 max(0, n.l), em inteiros (/256) */
                const signed char *n = &c->nrm[k * 4];
                int d = n[0] * lo[0] + n[1] * lo[1] + n[2] * lo[2];
                unsigned int sh;

                d = d < 0 ? 0 : (d > 16384 ? 16384 : d);
                sh = 179u + (((unsigned int)d * 77u) >> 14); /* 0,70 + 0,30 n.l: empírico, medido contra o PC */
                col = (col & 0xff000000u) |
                      ((((col >> 16) & 0xffu) * sh >> 8) << 16) |
                      ((((col >> 8) & 0xffu) * sh >> 8) << 8) |
                      ((col & 0xffu) * sh >> 8);
            }
            zstrip_model_vertex(&c->pts[k * 3],
                                textured && uv != NULL ? uv[j].tex.u : 0,
                                textured && uv != NULL ? uv[j].tex.v : 0,
                                col, j == 0);
        }
        m += count;
        if (uv) uv += count;
        if (vc) vc += count;
    }
}

static int model_in_view(const NJS_MODEL *model)
{
    /* so o z do centro importa: tres produtos em vez de um njCalcPoint inteiro (que passava
     * pelo FTRV de 28 operacoes e era a maior fonte de chamadas dele) */
    const float *m = native_xmtrx();
    float r = model->r;
    float cz = m[2] * model->center.x + m[6] * model->center.y + m[10] * model->center.z + m[14];

    return cz > _nj_near_clip_ - r && cz < _nj_far_clip_ + r;
}

static void draw_basic(const NJS_MODEL *model, int cull)
{
    int i;

    if (model == NULL || model->points == NULL || model->meshsets == NULL) {
        return;
    }
    if (cull && !(_nj_control_3d_flag_ & NJD_CONTROL_3D_NO_CLIP_CHECK) &&
        !model_in_view(model)) {
        return;
    }
    {
        struct mcache *c = zlive_gl ? mcache_get(model) : 0;
        int projected = 0;
        int lit = 0, light_o[3] = {0, 0, 0};
        int key_set = 0;
        float key = 0.0F;

        if (c != 0) {
            int proj[16], mdl[16];
            const float *x = native_xmtrx();
            NJS_VECTOR l;
            float ll, invl_unused;

            light_get(&l, &ll, &invl_unused);

            build_proj(proj);
            for (i = 0; i < 16; i++) {
                mdl[i] = fixf(x[i]);
            }
            zstrip_model_begin(proj, mdl);
            lit = 0;
            if (c->nrm != NULL && ll > 0.0F) {
                /* a luz em espaço de objeto: n_vista . l = n . (M^T l). A
                 * cada vértice só inteiros: normal (bytes) x luz (bytes) */
                float lo[3], len;
                int k;

                for (k = 0; k < 3; k++) {
                    lo[k] = x[k * 4] * l.x + x[k * 4 + 1] * l.y + x[k * 4 + 2] * l.z;
                }
                len = sqrtf(lo[0] * lo[0] + lo[1] * lo[1] + lo[2] * lo[2]);
                if (len > 0.0F) {
                    len = -127.0F / len;
                    light_o[0] = (int)(lo[0] * len);
                    light_o[1] = (int)(lo[1] * len);
                    light_o[2] = (int)(lo[2] * len);
                    lit = !ZLIVE_NOSHADE;
                }
            }
        }
        for (i = 0; i < model->nbMeshset; i++) {
            const NJS_MESHSET *ms = &model->meshsets[i];

            if (c != 0) {
                Uint32 mf = material_flags(&model->mats[ms->type_matId & 0x3fff]);

                int cut = 0;
                int why = (mf & NJD_FLAG_USE_ALPHA) ? alpha_opaque(model, ms, mf, &cut) : 0;

                if (why == 0) {
                    if (mf & NJD_FLAG_USE_ALPHA) {
                        zstrip_note_alpha(cut ? 7 : 1);
                    }
                    draw_meshset_gl(model, ms, c, (lit && !(mf & NJD_FLAG_IGNORE_LIGHT)) ? light_o : 0, cut);
                    if (cut) {
                        zstrip_cutout(0);
                    }
                    continue;
                }
                zstrip_note_alpha(why);
                if (ZLIVE_TDEFER) {
                    /* translucido de verdade: o mesmo desenho GL, adiado e ordenado por 1/z do
                     * centro do modelo com os strips translucidos (zstrip_flush) */
                    if (!key_set) {
                        const float *xm = native_xmtrx();
                        float vz = xm[2] * model->center.x + xm[6] * model->center.y +
                                   xm[10] * model->center.z + xm[14];

                        key = vz > 0.001F ? 1.0F / vz : 1000.0F;
                        key_set = 1;
                    }
                    zstrip_blend_pvr((mf >> 29) & 7, (mf >> 26) & 7);
                    zstrip_defer(1, key);
                    draw_meshset_gl(model, ms, c, (lit && !(mf & NJD_FLAG_IGNORE_LIGHT)) ? light_o : 0, 0);
                    zstrip_model_flush();
                    zstrip_defer(0, 0.0F);
                    continue;
                }
            }
            if (!projected) {
                projected = 1;
                project_model(model);
            }
            draw_meshset(model, ms);
        }
        zstrip_blend(0);
        zstrip_model_flush();
    }
}

/* um caractere do print do Ninja (njDebug.c: njCharacter, SA_SA1_CHARACTER):
 * a fonte de 16x16 células que njInitPrint carregou no contexto de debug,
 * como engine/dc/sa1/text.h manda ao PVR */
#define SA_SA1_CHARACTER 8
#define DEBUG_CONTEXT 4
#define CHAR_Z 1.0F
struct sa1_live_character {
    Sint16 x, y;
    Float size;
    Uint32 color;
};
extern KMVERTEXCONTEXT _nj_context_[];

static void draw_character(const struct sa1_live_character *st, Uint8 code)
{
    const float cell = 1.0F / 16.0F;
    float u = (code % 16) * cell, v = (code / 16) * cell;
    float x = st->x, y = st->y, s = st->size;
    const void *font = _nj_context_[DEBUG_CONTEXT].pTextureSurfaceDesc;
    int idx = font != NULL ? ztexture_index(font) : -1;

    if (idx < 0) {
        return;
    }
    zstrip_tex_select(idx);
    zstrip_use_texture();
    zstrip_blend(1);
    zstrip_begin();
    zstrip_vertex(x, y, CHAR_Z, u, v, st->color);
    zstrip_vertex(x, y + s, CHAR_Z, u, v + cell, st->color);
    zstrip_vertex(x + s, y, CHAR_Z, u + cell, v, st->color);
    zstrip_vertex(x + s, y + s, CHAR_Z, u + cell, v + cell, st->color);
    zstrip_end();
    zstrip_blend(0);
}

/* njCharacter (njDebug.c remendado por tools/gen_patch.py) chama isto */
void zlive_character(const void *state, Uint8 code)
{
    draw_character((const struct sa1_live_character *)state, code);
}

static int sa_present_live_(sa_engine *engine, const sa_draw_live *draw);
unsigned int zprof_ms(void);
Uint32 zlive_us; /* tempo dentro do sa_present_live, para o perfil */

int sa_present_live(sa_engine *engine, const sa_draw_live *draw)
{
    Uint32 t0 = zprof_ms();
    int r = sa_present_live_(engine, draw);
    static Uint32 acc;
    static int calls;

    acc += zprof_ms() - t0;
    zlive_us += acc;
    if ((++calls & 1023) == 0) {
        zlog("vz", "ms em 1024 draws", (int)acc);
        acc = 0;
    }
    return r;
}

static int sa_present_live_(sa_engine *engine, const sa_draw_live *draw)
{
    Uint32 how;
    static int seen;

    (void)engine;
    if (draw->format == SA_SA1_CHARACTER) {
        draw_character((const struct sa1_live_character *)draw->state,
                       *(const Uint8 *)draw->geometry);
        return 0;
    }
    if (draw->format != 1) { /* só SA_SA1_BASIC é um NJS_MODEL */
        static int warned;

        if (!warned) {
            warned = 1;
            zlog("vlive", "formato sem suporte", (int)draw->format);
        }
        return 0;
    }
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

/* Ninja "simple" (njModelS.c fica fora do build, ver game.mk): os dois pontos de entrada
 * que o jogo chama desenham pelo mesmo caminho dos "easy" */
void njSimpleDrawModel(NJS_MODEL *model)
{
    draw_basic(model, 1);
}

void njSimpleDrawObject(NJS_OBJECT *object)
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
