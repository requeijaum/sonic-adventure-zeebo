/* MOVIE.BIN no Zeebo: os filmes Sofdec (.sfd) do disco tocados no ARM.
 *
 * movie_play (src/progress/movie.c) lê MOVIE.BIN sobre movie_entry (zgdfs.c
 * ignora a leitura) e chama movie_entry(list). Aqui movie_entry toca os
 * filmes da lista como modules/movie/movie.c: as mesmas tabelas de filmes e
 * listas; Start (ou A) pula o resto da lista, e a volta é sempre 1.
 *
 * O vídeo (MPEG-1, stream 0xE0) sai do pl_mpeg pelo demux direto (o
 * cabeçalho de sistema do Sofdec só declara o áudio), vira RGB565 e sobe
 * numa textura 512x256 (zstrip_movie_upload) desenhada na tela toda. O
 * ritmo segue o relógio: a cada vsync decodifica até alcançar o quadro do
 * tempo passado (no máximo 3 por vsync; o excesso é pulado sem converter).
 * Ainda sem o áudio (ADX no stream 0xC0). */
#include <kamui.h>
#include <shinobi.h>
#include <ninja.h>


#include "zstrip.h"
#include "system/pad.h"

void zlog(const char *what, const char *name, int value);

/* modules/movie/movie.c: os filmes e as listas que movie_entry recebe */
static const char *const movie_files[10] = {
    "slh448_1.sfd", "sah448_6.sfd", "sa1_600.sfd", "sa2_600.sfd",
    "sa3_600.sfd",  "sa4_600.sfd",  "sa5_600.sfd", "sa6_600.sfd",
    "sa7_300.sfd",  "sa8_600.sfd",
};
static const signed char movie_list0[] = {0, 1, -1};
static const signed char movie_list1[] = {2, -1};
static const signed char movie_list2[] = {3, -1};
static const signed char movie_list3[] = {4, -1};
static const signed char movie_list4[] = {5, -1};
static const signed char movie_list5[] = {6, -1};
static const signed char movie_list6[] = {7, -1};
static const signed char movie_list7[] = {8, -1};
static const signed char movie_list8[] = {9, -1};
static const signed char *const movie_lists[9] = {
    movie_list0, movie_list1, movie_list2, movie_list3, movie_list4,
    movie_list5, movie_list6, movie_list7, movie_list8,
};

/* o decoder fica em zplm.c, compilado sem os cabeçalhos do jogo */
struct zplm;
struct zplm *zplm_open(void *file);
int zplm_decode(struct zplm *z);            /* 1 = quadro, 0 = fim */
int zplm_convert(struct zplm *z, unsigned short *out, int maxw, int maxh,
                 int *w, int *h);
void zplm_close(struct zplm *z);
unsigned char *zplm_extract_audio(void *file, int *len);
void free(void *p);

static unsigned short frame565[320 * 256];

static void draw_frame(int index, int w, int h)
{
    float u = (float)w / 512.0F, v = (float)h / 256.0F;

    zstrip_tex_select(index);
    zstrip_use_texture();
    zstrip_blend(0);
    zstrip_begin();
    zstrip_vertex(0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0xffffffffu);
    zstrip_vertex(640.0F, 0.0F, 1.0F, u, 0.0F, 0xffffffffu);
    zstrip_vertex(0.0F, 480.0F, 1.0F, 0.0F, v, 0xffffffffu);
    zstrip_vertex(640.0F, 480.0F, 1.0F, u, v, 0xffffffffu);
    zstrip_end();
}

/* toca um filme; 0 se Start o pulou */
static int movie_run(const char *name)
{
    void *file = zfile_handle_open(name);
    struct zplm *z;
    Uint32 start;
    int shown = 0;
    int index = -1;
    int w = 320, h = 240;
    int ended = 0;

    if (file == 0) {
        zlog("movie", name, -1);
        return 1;
    }
    /* o áudio primeiro: o ADX inteiro do filme, decodificado e tocado pelo
     * IMedia junto com o primeiro quadro */
    {
        void *af = zfile_handle_open(name);
        int alen = 0;
        unsigned char *adx = af ? zplm_extract_audio(af, &alen) : 0;

        if (af) zfile_handle_close(af);
        zlog("movie", "audio adx", alen);
        if (adx != 0) {
            zaudio_play_adx(adx, alen, 120);
            free(adx);
        }
    }
    z = zplm_open(file);
    zlog("movie", name, 0);
    start = syTmrGetCount();
    for (;;) {
        Uint32 us = syTmrCountToMicro(syTmrDiffCount(start, syTmrGetCount()));
        int want = (int)(us / 33333u) + 1; /* 30 fps */
        int budget = 3;
        int got = 0;

        while (shown < want && budget-- > 0) {
            if (!zplm_decode(z)) {
                ended = 1;
                break;
            }
            got = 1;
            shown++;
        }
        if (got) {
            zplm_convert(z, frame565, 320, 256, &w, &h);
            index = zstrip_movie_upload(frame565, w, h);
        }
        if (index >= 0) {
            draw_frame(index, w, h);
        }
        njWaitVSync();
        if (ended) {
            break;
        }
        if (per_info[0] != 0 && (per_info[0]->press & (PDD_DGT_ST | PDD_DGT_TA))) {
            zplm_close(z);
            zfile_handle_close(file);
            zaudio_stop();
            zlog("movie", "pulado", shown);
            return 0; /* pulado: a lista para */
        }
    }
    zplm_close(z);
    zfile_handle_close(file);
    zaudio_stop();
    zlog("movie", "fim", shown);
    return 1;
}

int movie_entry(int list)
{
    const signed char *l;
    int result = 1;

    if (list < 0 || list >= 9) {
        return 1;
    }
    for (l = movie_lists[list]; *l >= 0; l++) {
        if (movie_run(movie_files[*l]) == 0) {
            break;
        }
    }
    /* não zero: main.c segue para os menus depois da abertura (pulada ou
     * não); dentro da história, só < 0 (sair) muda o caminho */
    return result;
}
