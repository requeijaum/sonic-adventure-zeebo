#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

struct model_vertex {
    uint32_t control;
    float sx, sy, oow;
    float px, py, pz;
    uint32_t reserve;
    float payload[4];
    float vx, vy, vz;
    uint32_t lit_key;
};
struct calc_work {
    int32_t count;
    uint32_t vertex, points, normals;   /* 32 bits: endereços do domínio */
    float scale_x, scale_y;
    uint32_t clip_and, clip_or;
    float left, right, top, bottom;
};
#define NV 6
static float pts[NV][3] = {{0,0,5},{1,0,6},{-2,1,4},{0.5f,-3,2.5f},{0,0,0.5f},{3,2,-1}};
static float nrm[NV][3] = {{0,0,1},{0,1,0},{1,0,0},{0.3f,0.4f,0.5f},{0,0,-1},{2,0,0}};
static void dump(const char *tag, struct model_vertex *v, struct calc_work *w)
{
    int i;
    printf("%s and=%08x or=%08x\n", tag, w->clip_and, w->clip_or);
    for (i = 0; i < NV; i++)
        printf("%d ctl=%02x s=(%.3f,%.3f) oow=%.5f p=(%.3f,%.3f,%.3f) n=(%.4f,%.4f,%.4f) key=%08x\n",
               i, v[i].control, v[i].sx, v[i].sy, v[i].oow, v[i].px, v[i].py,
               v[i].pz, v[i].vx, v[i].vy, v[i].vz, v[i].lit_key);
}
