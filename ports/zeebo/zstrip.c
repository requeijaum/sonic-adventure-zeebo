/* zstrip.c — acumulador de strip e sua submissão GLES1 (caminho fixo).
 *
 * O Zeebo é ARM11 sem FPU e o caminho provado no zeebx/IGLES11 é o de
 * arranjos *tight* (stride 0) em GL_FIXED s15.16, com glOrthox/glClearDepthx.
 * Os vértices chegam em floats (convenção do SA1) e são convertidos aqui.
 *
 * Sem libc: arrays estáticos. */

#define GL_OES_VERSION_1_1
#include "gles/gl.h"

#include "assets/prs.h"
#include "assets/pvm.h"
#include "assets/pvr.h"
#include "zstrip.h"

extern const unsigned char zasset_pvr[];
extern const unsigned int zasset_pvr_len;
/* o PRS vem do armazenamento do app (IFileMgr), não do .mod */
#define PRS_FILE "FROG.PRS"
static unsigned char zasset_prs[64 * 1024];
static unsigned int zasset_prs_len;

#define ZSTRIP_MOST 4096
#define FIX(v) ((GLfixed)((v) * 65536.0f))

/* arranjos separados e contíguos: stride 0 */
static GLfixed vxyz[ZSTRIP_MOST * 3];
static GLfixed vuv[ZSTRIP_MOST * 2];
static GLubyte vrgba[ZSTRIP_MOST * 4];
static int vert_count;

static GLuint texture; /* definida na cena; usada pelo estado do strip */

/* tabela global de texturas: o índice é o globalIndex do Ninja */
#define ZTEX_MOST 512
static GLuint tex_names[ZTEX_MOST];
static int tex_count;
static GLuint tex_current;

void zstrip_begin(void)
{
    vert_count = 0;
}

static GLubyte byte_of(uint32_t argb, int shift)
{
    return (GLubyte)((argb >> shift) & 0xffu);
}

void zstrip_vertex(float x, float y, float z, float u, float v,
                   uint32_t argb)
{
    int at;

    if (vert_count >= ZSTRIP_MOST) {
        return;
    }
    at = vert_count++;
    vxyz[at * 3 + 0] = FIX(x);
    vxyz[at * 3 + 1] = FIX(y);
    vxyz[at * 3 + 2] = FIX(z);
    vuv[at * 2 + 0] = FIX(u);
    vuv[at * 2 + 1] = FIX(v);
    /* GL espera RGBA; a convenção do jogo é ARGB. */
    vrgba[at * 4 + 0] = byte_of(argb, 16);
    vrgba[at * 4 + 1] = byte_of(argb, 8);
    vrgba[at * 4 + 2] = byte_of(argb, 0);
    vrgba[at * 4 + 3] = byte_of(argb, 24);
}

void zstrip_end(void)
{
    if (vert_count < 3) {
        vert_count = 0;
        return;
    }
    glVertexPointer(3, GL_FIXED, 0, vxyz);
    glTexCoordPointer(2, GL_FIXED, 0, vuv);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, vrgba);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vert_count);
    vert_count = 0;
}

void zstrip_blend(int on)
{
    if (on) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
}

void zstrip_use_texture(void)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_current != 0 ? tex_current : texture);
}

int zstrip_tex_new(int w, int h, const void *rgba)
{
    GLuint name = 0;

    if (tex_count >= ZTEX_MOST || w <= 0 || h <= 0) {
        return -1;
    }
    glGenTextures(1, &name);
    glBindTexture(GL_TEXTURE_2D, name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, rgba);
    tex_names[tex_count] = name;
    return tex_count++;
}

void zstrip_tex_select(int index)
{
    tex_current = (index >= 0 && index < tex_count) ? tex_names[index] : 0;
}

void zstrip_tex_release_all(void)
{
    if (tex_count > 0) {
        glDeleteTextures(tex_count, tex_names);
    }
    tex_count = 0;
    tex_current = 0;
}

void zstrip_no_texture(void)
{
    glDisable(GL_TEXTURE_2D);
}

/* brew_egl.c avisa o lado GL do tamanho real do painel; aqui só o viewport
 * importa. (O nome vem do contrato do sa1hello.) */
void Sa1Hello_Resize(int w, int h)
{
    if (w > 0 && h > 0) {
        glViewport(0, 0, w, h);
    }
}

/* --- cena de teste ------------------------------------------------------- */

#define CHECKER 64
#define CHECKER_BYTES (CHECKER * CHECKER * 4)

static unsigned char texels[CHECKER_BYTES];
static unsigned char tex_rgba[256 * 256 * 4];
static unsigned char prs_out[96 * 1024];
static GLuint texture;

static void texture_make(void)
{
    {
        int n = zgdfs_read_file(PRS_FILE, zasset_prs, (int)sizeof zasset_prs);

        zasset_prs_len = n > 0 ? (unsigned int)n : 0;
    }
    int x;
    int y;

    for (y = 0; y < CHECKER; y++) {
        for (x = 0; x < CHECKER; x++) {
            int on = ((x >> 4) ^ (y >> 4)) & 1;
            int at = ((y * CHECKER) + x) * 4;

            texels[at + 0] = on ? 240 : 48;
            texels[at + 1] = on ? 240 : 48;
            texels[at + 2] = on ? 120 : 24;
            texels[at + 3] = 255;
        }
    }
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    /* PRS -> PVM -> PVRT: a cadeia completa dos assets do jogo */
    if (zasset_prs_len != 0) {
        int unpacked = prs_decompress(zasset_prs, (int)zasset_prs_len, prs_out,
                                      (int)sizeof prs_out);

        if (unpacked > 0 &&
            zninja_load_scene_pvm(prs_out, unpacked) > 0) {
            /* o checker fica como textura de fallback (índice -1) */
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHECKER, CHECKER, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, texels);
            return;
        }
    }

    /* textura PVRT avulsa, quando o build a embutiu */
    {
        PvrTexture pvr[1];
        int count = zasset_pvr_len != 0
                        ? pvr_scan(zasset_pvr, (int)zasset_pvr_len, pvr, 1)
                        : 0;
        int bytes = 0;

        if (count > 0) {
            bytes = pvr_decode_rgba8888(
                &pvr[0], zasset_pvr + pvr[0].data_offset,
                (int)zasset_pvr_len - (int)pvr[0].data_offset, tex_rgba,
                (int)sizeof tex_rgba);
        }
        if (bytes > 0) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)pvr[0].width,
                         (GLsizei)pvr[0].height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         tex_rgba);
            return;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHECKER, CHECKER, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, texels);
}

void zscene_init(void)
{
    texture_make();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* x para a direita, y para baixo, como o jogo entrega. */
    glOrthox(0, 640 << 16, 480 << 16, 0, -1 << 16, 1 << 16);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_TRUE);
    glClearDepthx(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    zcube_init();
    zsprite_init();
    zaudio_init();
}

void zscene_frame(uint32_t tick)
{
    /* passe 3D: depth padrão (menor = mais perto), clear em 1 */
    glClearDepthx(1 << 16);
    glClearColor(0.06f, 0.06f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* passe 2D: convenção do PVR (maior 1/z = mais perto, clear 0) */
    glClearDepthx(0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_GEQUAL);
    glDisable(GL_CULL_FACE);
    /* o 2D do Zeebo ordena por ordem de desenho: sem depth test. O passe 3D
     * (cubo) liga o seu com clear próprio. */
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthox(0, 640 << 16, 480 << 16, 0, -1 << 16, 1 << 16);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    /* os quads texturizados vêm do njDrawTexture real (zninja.c) */
    zninja_frame(tick);

    /* e a banda usa o sink direto, sem Ninja: os dois caminhos convivem */
    zstrip_use_texture();
    zstrip_blend(1);
    zstrip_begin();
    zstrip_vertex(0.0f, 430.0f, 0.90f, 0.0f, 0.0f, 0x6000ff80u);
    zstrip_vertex(640.0f, 430.0f, 0.90f, 4.0f, 0.0f, 0x6000ff80u);
    zstrip_vertex(0.0f, 480.0f, 0.90f, 0.0f, 0.5f, 0x6000ff80u);
    zstrip_vertex(640.0f, 480.0f, 0.90f, 4.0f, 0.5f, 0x6000ff80u);
    zstrip_end();

    zaudio_frame(tick);
    zsprite_frame(tick);

    /* passe 3D por cima, com depth próprio (clear 1, menor = mais perto) */
    glClearDepthx(1 << 16);
    glClear(GL_DEPTH_BUFFER_BIT);
    zcube_frame(tick);
}
