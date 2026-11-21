/* zcube.c — prova do caminho 3D do Zeebo: T&L de hardware.
 *
 * O ARM11 não tem FPU e o Adreno 130 tem vertex engine. A geometria vai em
 * espaço de objeto, em GL_FIXED; a matriz do Ninja (pilha real do decomp, já
 * portada sobre SH4ZAM SW) é convertida para 16.16 e entregue ao GLES1 com
 * glMultMatrixx. Nenhum vértice é transformado no CPU. */

#define GL_OES_VERSION_1_1
#include "gles/gl.h"

#include <ninja.h>

#include "zstrip.h"

#define F(v) ((GLfixed)((v) * 65536.0f))

/* 6 faces, 2 triângulos cada */
static const float cube_points[36][3] = {
    {-0.7f, -0.7f, 0.7f}, {0.7f, -0.7f, 0.7f}, {0.7f, 0.7f, 0.7f},
    {-0.7f, -0.7f, 0.7f}, {0.7f, 0.7f, 0.7f}, {-0.7f, 0.7f, 0.7f},
    {0.7f, -0.7f, -0.7f}, {0.7f, -0.7f, 0.7f}, {0.7f, 0.7f, 0.7f},
    {0.7f, -0.7f, -0.7f}, {0.7f, 0.7f, 0.7f}, {0.7f, 0.7f, -0.7f},
    {0.7f, -0.7f, -0.7f}, {-0.7f, -0.7f, -0.7f}, {-0.7f, 0.7f, -0.7f},
    {0.7f, -0.7f, -0.7f}, {-0.7f, 0.7f, -0.7f}, {0.7f, 0.7f, -0.7f},
    {-0.7f, -0.7f, 0.7f}, {-0.7f, -0.7f, -0.7f}, {-0.7f, 0.7f, -0.7f},
    {-0.7f, -0.7f, 0.7f}, {-0.7f, 0.7f, -0.7f}, {-0.7f, 0.7f, 0.7f},
    {-0.7f, 0.7f, 0.7f}, {0.7f, 0.7f, 0.7f}, {0.7f, 0.7f, -0.7f},
    {-0.7f, 0.7f, 0.7f}, {0.7f, 0.7f, -0.7f}, {-0.7f, 0.7f, -0.7f},
    {-0.7f, -0.7f, -0.7f}, {0.7f, -0.7f, -0.7f}, {0.7f, -0.7f, 0.7f},
    {-0.7f, -0.7f, -0.7f}, {0.7f, -0.7f, 0.7f}, {-0.7f, -0.7f, 0.7f},
};

static const GLubyte face_rgb[6][3] = {
    {220, 60, 60},  {60, 200, 90},  {70, 110, 230},
    {230, 200, 70}, {190, 90, 220}, {70, 200, 210},
};

static GLfixed cube_xyz[36 * 3];
static GLubyte cube_rgba[36 * 4];

void zcube_init(void)
{
    int i;

    for (i = 0; i < 36; i++) {
        cube_xyz[i * 3 + 0] = F(cube_points[i][0]);
        cube_xyz[i * 3 + 1] = F(cube_points[i][1]);
        cube_xyz[i * 3 + 2] = F(cube_points[i][2]);
        cube_rgba[i * 4 + 0] = face_rgb[i / 6][0];
        cube_rgba[i * 4 + 1] = face_rgb[i / 6][1];
        cube_rgba[i * 4 + 2] = face_rgb[i / 6][2];
        cube_rgba[i * 4 + 3] = 255;
    }
}

void zcube_frame(uint32_t tick)
{
    NJS_MATRIX m;
    GLfixed fixed[16];
    int i;

    njUnitMatrix(NULL);
    njRotateY(NULL, (Angle)(tick * 90));
    njRotateX(NULL, (Angle)(tick * 55));
    njGetMatrix(&m);
    for (i = 0; i < 16; i++) {
        fixed[i] = F(m[i]);
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustumx(-F(1.0f), F(1.0f), -F(0.75f), F(0.75f), F(0.5f), F(20.0f));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatex(F(0.9f), 0, -F(3.4f));
    glMultMatrixx(fixed);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FIXED, 0, cube_xyz);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, cube_rgba);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}
