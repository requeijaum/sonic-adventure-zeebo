#define _GNU_SOURCE
#include "common.h"
typedef float Float; typedef uint32_t Uint32; typedef int32_t Sint32;
typedef struct { float x, y, z; } NJS_POINT3, NJS_VECTOR;
float _nj_far_clip_ = 10000.0f, _nj_near_clip_ = 1.0f, _nj_vertex_offset_ = 0.5f;
struct { float dist, w, h, cx, cy; } _nj_screen_ = {640, 640, 480, 320, 240};
typedef struct { float x, y, z, w; } shz_vec4_t;
static float XM[16] = {0.9f,0.1f,0.0f,0.0f,  -0.1f,0.95f,0.2f,0.0f,  0.0f,-0.2f,1.0f,0.0f,  0.3f,-0.4f,2.0f,1.0f};
static shz_vec4_t shz_vec4_init(float x,float y,float z,float w){shz_vec4_t r={x,y,z,w};return r;}
/* FTRV: out = M * v, M em colunas (m[0..3] = coluna 0) */
static shz_vec4_t shz_xmtrx_transform_vec4(shz_vec4_t v){
    shz_vec4_t r;
    r.x = XM[0]*v.x + XM[4]*v.y + XM[8]*v.z + XM[12]*v.w;
    r.y = XM[1]*v.x + XM[5]*v.y + XM[9]*v.z + XM[13]*v.w;
    r.z = XM[2]*v.x + XM[6]*v.y + XM[10]*v.z + XM[14]*v.w;
    r.w = XM[3]*v.x + XM[7]*v.y + XM[11]*v.z + XM[15]*v.w;
    return r;
}
/* o trecho de calc_point e dos LOOP do njmodel_arm.c, sem mudar uma linha */
#define STRUCTS_ALREADY
#define KM_VERTEXPARAM_ENDOFSTRIP 0xf0000000
#undef NJS_ARGB
/* tipos do njmodel_arm.c, alinhados a common.h */
struct mv { Uint32 control; Float sx, sy, oow; NJS_POINT3 point; Uint32 reserve; Float payload[4]; NJS_VECTOR vector; Uint32 lit_key; };
struct cw { Sint32 count; struct mv *vertex; NJS_POINT3 *points; NJS_VECTOR *normals; Float scale_x, scale_y; Uint32 clip_and, clip_or; Float left, right, top, bottom; };
static void calc_point(struct cw *work, Sint32 i, int has_normal,
                       int normalize, int has_offset, int clip)
{
    struct mv *v = &work->vertex[i];
    NJS_POINT3 p = work->points[i];
    shz_vec4_t t;
    Uint32 flags = 0;

    if (has_normal) {
        const NJS_VECTOR *n = &work->normals[i];
        shz_vec4_t nv =
            shz_xmtrx_transform_vec4(shz_vec4_init(n->x, n->y, n->z, 0.0F));
        float nx = nv.x, ny = nv.y, nz = nv.z;

        if (normalize) {
            float len2 = nx * nx + ny * ny + nz * nz;
            float inv = len2 > 0.0F ? 1.0F / __builtin_sqrtf(len2) : 0.0F;

            v->vector.x = nx * inv;
            v->vector.y = ny * inv;
            v->vector.z = nz * inv;
        } else {
            v->vector.x = nx;
            v->vector.y = ny;
            v->vector.z = nz;
        }
        /* o asm copia a normal para FV0 antes do FTRV: o deslocamento é a
         * normal do MODELO vezes _nj_vertex_offset_, somada ao ponto antes
         * da matriz (conferido contra o asm traduzido por asm2c.py) */
        if (has_offset) {
            p.x += n->x * _nj_vertex_offset_;
            p.y += n->y * _nj_vertex_offset_;
            p.z += n->z * _nj_vertex_offset_;
        }
    }

    t = shz_xmtrx_transform_vec4(shz_vec4_init(p.x, p.y, p.z, 1.0F));
    {
        float z = t.z;
        float inv = 1.0F / z;

        v->point.x = t.x;
        v->point.y = t.y;
        v->point.z = z;
        v->oow = inv;
        v->sx = t.x * work->scale_x * inv + _nj_screen_.cx;
        v->sy = t.y * work->scale_y * inv + _nj_screen_.cy;
        if (clip) {
            if (!(_nj_near_clip_ > z)) {
                flags = (Uint32)(v->sy > work->bottom) << 5 |
                        (Uint32)(work->top > v->sy) << 4 |
                        (Uint32)(v->sx > work->right) << 3 |
                        (Uint32)(work->left > v->sx) << 2;
            }
            flags |= (z > _nj_far_clip_ ? 2u : 0u) |
                     (_nj_near_clip_ > z ? 1u : 0u);
            work->clip_and &= flags;
            work->clip_or |= flags;
        }
    }
    v->control = flags;
    v->lit_key = 0xffffffffu;
}

#define LOOP(fn, has_normal, normalize, has_offset, clip)                     \
    void fn(struct cw *work)                                           \
    {                                                                         \
        Sint32 i;                                                             \
                                                                              \
        for (i = 0; i < work->count; i++) {                                   \
            calc_point(work, i, has_normal, normalize, has_offset, clip);     \
        }                                                                     \
    }

LOOP(loop1_body, 1, 1, 1, 0)
LOOP(loop2_body, 1, 0, 1, 0)
LOOP(loop3_body, 1, 1, 0, 0)
LOOP(loop4_body, 1, 0, 0, 0)
LOOP(loop5_body, 0, 0, 0, 0)
LOOP(loop6_body, 1, 1, 1, 1)
LOOP(loop7_body, 1, 0, 1, 1)
LOOP(loop8_body, 1, 1, 0, 1)
LOOP(loop9_body, 1, 0, 0, 1)
LOOP(loopa_body, 0, 0, 0, 1)
int main(int argc, char **argv){
    int loop = atoi(argv[1]);
    static struct mv V[NV]; static NJS_POINT3 P[NV], N[NV]; struct cw w;
    int i; memcpy(P, pts, sizeof pts); memcpy(N, nrm, sizeof nrm); memset(V, 0xAA, sizeof V);
    w.count=NV; w.vertex=V; w.points=P; w.normals=N; w.scale_x=640.0f*0.75f; w.scale_y=640.0f;
    w.clip_and=0xffffffffu; w.clip_or=0; w.left=0; w.right=640; w.top=0; w.bottom=480;
    void (*fn[])(struct cw*) = {0,loop1_body,loop2_body,loop3_body,loop4_body,loop5_body,loop6_body,loop7_body,loop8_body,loop9_body,loopa_body};
    fn[loop](&w);
    printf("mine and=%08x or=%08x\n", w.clip_and, w.clip_or);
    for (i=0;i<NV;i++) printf("%d ctl=%02x s=(%.3f,%.3f) oow=%.5f p=(%.3f,%.3f,%.3f) n=(%.4f,%.4f,%.4f) key=%08x\n", i, V[i].control, V[i].sx, V[i].sy, V[i].oow, V[i].point.x, V[i].point.y, V[i].point.z, V[i].vector.x, V[i].vector.y, V[i].vector.z, V[i].lit_key);
    return 0; }
