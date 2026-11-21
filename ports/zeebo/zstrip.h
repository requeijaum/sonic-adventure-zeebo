/* Sink GLES1 dos draws de tela do SA1.
 *
 * Substitui engine/dc/sa1/strip.h: em vez de escrever vértices na store
 * queue do PowerVR, acumula um strip e o desenha com glDrawArrays. A
 * convenção é a mesma que o jogo entrega a sa1_send_vertex:
 *
 *   x, y  pixels de tela (0..640, 0..480)
 *   z     1/profundidade (maior é mais perto), como o PVR recebe
 *   u, v  textura normalizada
 *   argb  0xAARRGGBB
 *
 * Um strip é begin -> N vértices -> end. A ordem dos vértices é a de
 * GL_TRIANGLE_STRIP, igual à que o jogo manda. */
#ifndef SA1Z_ZSTRIP_H
#define SA1Z_ZSTRIP_H

#include <stdint.h>

void zstrip_begin(void);
void zstrip_vertex(float x, float y, float z, float u, float v,
                   uint32_t argb);
void zstrip_end(void);

/* estado por strip, como o njDraw.c do decomp escolhe ao abrir */
void zstrip_blend(int on);
void zstrip_use_texture(void);
void zstrip_no_texture(void);

/* texturas do jogo: zstrip_tex_new devolve o índice global (ou -1);
 * zstrip_tex_select(-1) volta à textura de teste */
int zstrip_tex_new(int w, int h, const void *rgba);
void zstrip_tex_select(int index);
void zstrip_tex_release_all(void);

/* carrega o PVM da cena no texlist do Ninja (nj_sprite.c); devolve o
 * número de texturas */
int zninja_load_scene_pvm(unsigned char *pvm, int len);
int zninja_scene_tex_count(void);

/* cena de teste: textura procedural + estado GLES1; os dois quads vêm do
 * njDrawTexture real (zninja.c) */
void zscene_init(void);
void zscene_frame(uint32_t tick);
void zninja_frame(uint32_t tick);

/* cubo 3D com T&L de hardware, matriz vinda da pilha do Ninja */
void zcube_init(void);
void zsprite_init(void);
void zaudio_init(void);
struct _IShell;
void zaudio_start(struct _IShell *shell);
void zaudio_stop(void);
void zaudio_frame(uint32_t tick);
/* arquivos pelo BREW IFileMgr (substitui o GDFS) */
int zfile_open(struct _IShell *shell);
void zfile_close(void);
int zfile_size(const char *path);
int zfile_read(const char *path, void *dst, int cap);
int zfile_write(const char *path, const void *src, int len);
int zgdfs_read_file(const char *name, void *dst, int cap);
void *zfile_handle_open(const char *path);
void zfile_handle_close(void *h);
int zfile_handle_read(void *h, void *dst, int len);
int zfile_handle_seek(void *h, int offset);
int zdc_selftest(void);
void zsprite_frame(uint32_t tick);
void zcube_frame(uint32_t tick);

#endif /* SA1Z_ZSTRIP_H */
