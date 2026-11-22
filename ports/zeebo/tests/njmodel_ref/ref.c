#define _GNU_SOURCE
#define NATIVE_DOMAIN 0ULL
#include "common.h"
#define _GNU_SOURCE
#include <sys/mman.h>
#include <unistd.h>
#include "loops.c"
/* globais que o asm pede */
float _nj_far_clip_ = 10000.0f, _nj_near_clip_ = 1.0f, _nj_vertex_offset_ = 0.5f;
float _nj_screen_[5] = {640.0f, 640, 480, 320.0f, 240.0f};
void native_asm_init(void);
/* sa_ftrv / sa_fsrra do host: lê play_math */
static struct model_vertex *V; static float (*P)[3]; static float (*N)[3];
int main(int argc, char **argv)
{
    int loop = atoi(argv[1]);
    struct calc_work *wp, w0;
#define w (*wp)
    static float xm[16];
    int i;
    /* XMTRX do SH-4: coluna-maior como o FTRV lê (m[0..3] = coluna 0) */
    float m[16] = {0.9f,0.1f,0.0f,0.0f,  -0.1f,0.95f,0.2f,0.0f,  0.0f,-0.2f,1.0f,0.0f,  0.3f,-0.4f,2.0f,1.0f};
    memcpy(X.f, m, sizeof m);
    {
        void *blk = mmap(0, 1<<20, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, -1, 0);
        if (blk == MAP_FAILED) { perror("mmap"); return 1; }
        V = (struct model_vertex *)blk; P = (float(*)[3])((char*)blk + 4096); N = (float(*)[3])((char*)blk + 8192);
        if (mmap((void*)0xFF000000u, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0) == MAP_FAILED) perror("qacr");
        { int fd = memfd_create("v", 0); ftruncate(fd, 1<<20);
          void *a = mmap(0, 1<<20, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_32BIT, fd, 0);
          blk = a; munmap(0,0);
          void *q = mmap((void*)(0xE0000000u | ((uint32_t)(uintptr_t)a & 0x03FFFFFFu)), 1<<20, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, fd, 0);
          if (q == MAP_FAILED) perror("sq"); }
        V = (struct model_vertex *)blk; P = (float(*)[3])((char*)blk + 4096); N = (float(*)[3])((char*)blk + 8192);
        wp = (struct calc_work *)((char*)blk + 12288);
        memcpy(P, pts, sizeof pts); memcpy(N, nrm, sizeof nrm);
        memset(V, 0xAA, NV * sizeof *V);
    }
    w.count = NV; w.vertex = (uint32_t)(uintptr_t)V; w.points = (uint32_t)(uintptr_t)P;
    w.normals = (uint32_t)(uintptr_t)N;
    w.scale_x = 640.0f * 0.75f; w.scale_y = 640.0f;
    w.clip_and = 0xffffffffu; w.clip_or = 0;
    w.left = 0; w.right = 640; w.top = 0; w.bottom = 480;
    void (*fn[])(uintptr_t) = {0, loop1_body, loop2_body, loop3_body, loop4_body, loop5_body,
        loop6_body, loop7_body, loop8_body, loop9_body, loopa_body};
    fn[loop]((uintptr_t)&w);
    (void)xm; (void)i;
    dump("ref", V, &w);
    return 0;
}
