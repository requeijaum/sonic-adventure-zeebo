/* instrumentação por log: desempenho (tperf), estado do jogo (tjogo) e texturas (ttex).
 *
 * Cada relatório roda na janela de 2 s do vprof_report e imprime em decimal (zlogd) só o que
 * mudou. Os contadores são alimentados por zgame_step (modo), ztexture.c (texturas) e app.c
 * (quadros). Tudo em inteiro: o ARM11 do Zeebo não tem VFP. */

#include <AEEStdLib.h>

#include "zstrip.h"

extern void zlogd(const char *what, const char *name, int value);

unsigned zinst_ms(void)
{
    return (unsigned)GETUPTIMEMS();
}

/* --- tprs: descompressao PRS dos arquivos do disco ----------------------------------- */

extern unsigned zprs_calls, zprs_out, zprs_ms;

void zinst_prs_report(void)
{
    static unsigned pc, po, pm;

    if (zprs_calls != pc) {
        zlogd("tprs", "arquivos descomprimidos (total)", (int)zprs_calls);
        zlogd("tprs", "KB descomprimidos (total)", (int)(zprs_out >> 10));
        zlogd("tprs", "ms descomprimindo (total)", (int)zprs_ms);
        zlogd("tprs", "KB/s medio (total)", zprs_ms != 0 ? (int)((zprs_out >> 10) * 1000u / zprs_ms) : 0);
        zlogd("tprs", "ms na janela", (int)(zprs_ms - pm));
        zlogd("tprs", "KB na janela", (int)((zprs_out - po) >> 10));
        pc = zprs_calls;
        po = zprs_out;
        pm = zprs_ms;
    }
}

/* --- tperf: tempo de quadro e onde a janela foi gasta --------------------------------- */

static uint32 pf_last;
static uint32 pf_min = 0xffffffffu, pf_max, pf_sum, pf_n, pf_slow100, pf_slow250;

/* um quadro apresentado (vprof_report) */
void zinst_frame(void)
{
    uint32 now = GETUPTIMEMS();

    if (pf_last != 0) {
        uint32 dt = now - pf_last;

        if (dt < pf_min) {
            pf_min = dt;
        }
        if (dt > pf_max) {
            pf_max = dt;
        }
        pf_sum += dt;
        pf_n++;
        pf_slow100 += dt > 100;
        pf_slow250 += dt > 250;
    }
    pf_last = now;
}

/* game/gl/swap: ms da janela em cada parte; window: duração da janela em ms */
void zinst_perf_report(unsigned game, unsigned gl, unsigned swap, unsigned window)
{
    static int prev[8];
    int now[8], i;
    static const char *const names[8] = {"quadro minimo (ms)", "quadro medio (ms)",
                                         "quadro maximo (ms)", "quadros acima de 100 ms",
                                         "quadros acima de 250 ms", "jogo (% da janela)",
                                         "gl (% da janela)", "swap (% da janela)"};

    if (pf_n == 0 || window == 0) {
        return;
    }
    now[0] = (int)pf_min;
    now[1] = (int)(pf_sum / pf_n);
    now[2] = (int)pf_max;
    now[3] = (int)pf_slow100;
    now[4] = (int)pf_slow250;
    now[5] = (int)(game * 100u / window);
    now[6] = (int)(gl * 100u / window);
    now[7] = (int)(swap * 100u / window);
    /* min, medio e maximo mudam quase sempre: imprime todos, uma linha de cada, por janela;
     * os acima de 100/250 ms só quando mudam */
    for (i = 0; i < 8; i++) {
        if (i < 3 || i >= 5 || now[i] != prev[i]) {
            zlogd("tperf", names[i], now[i]);
        }
        prev[i] = now[i];
    }
    pf_min = 0xffffffffu;
    pf_max = pf_sum = pf_n = pf_slow100 = pf_slow250 = 0;
}

/* --- tjogo: quanto tempo cada modo do jogo dura ------------------------------------- */

static const char *mode_name(int m)
{
    switch (m) {
    case 0:  return "BOOT";
    case 1:  return "LOGO";
    case 2:  return "TITLE";
    case 3:  return "STAGE_SELECT";
    case 4:  return "ACTION";
    case 5:  return "FIELD";
    case 8:  return "MOVIE";
    case 9:  return "TRIAL";
    case 10: return "MENU";
    case 11: return "MENU_RUN";
    case 14: return "SUMMARY";
    case 15: return "SUMMARY_RUN";
    case 16: return "TUTORIAL";
    case 17: return "TUTORIAL_RUN";
    case 18: return "STAFF_ROLL";
    case 19: return "STAFF_ROLL_RUN";
    default: return "?";
    }
}

static char *put_s(char *d, const char *s)
{
    while (*s != 0) {
        *d++ = *s++;
    }
    return d;
}

/* "prefixo NOME sufixo" num buffer: zlogd só aceita o nome pronto */
static const char *mode_label(char *buf, const char *pre, int m, const char *suf)
{
    char *d = buf;

    d = put_s(d, pre);
    d = put_s(d, mode_name(m));
    d = put_s(d, suf);
    *d = 0;
    return buf;
}

static int mode_cur = -1;
static unsigned mode_since_ms, mode_since_frame, boot_ms;
static int mode_visits[20];

/* o modo mudou (ou é o primeiro): frames é o contador de quadros do jogo até agora */
void zinst_mode(int mode, unsigned frames, int movie_request_now)
{
    char buf[48];
    unsigned now = (unsigned)GETUPTIMEMS();

    if (boot_ms == 0) {
        boot_ms = now;
    }
    if (mode_cur >= 0) {
        zlogd("tjogo", mode_label(buf, "saiu do modo ", mode_cur, ": ms"), (int)(now - mode_since_ms));
        zlogd("tjogo", mode_label(buf, "saiu do modo ", mode_cur, ": quadros"),
              (int)(frames - mode_since_frame));
    }
    zlogd("tjogo", mode_label(buf, "entrou no modo ", mode, " (ms desde o inicio)"),
          (int)(now - boot_ms));
    if (mode >= 0 && mode < 20) {
        mode_visits[mode]++;
        if (mode_visits[mode] > 1) {
            zlogd("tjogo", mode_label(buf, "modo ", mode, ": visitas"), mode_visits[mode]);
        }
    }
    if (mode == 8) { /* GAME_MODE_MOVIE: qual filme */
        zlogd("tjogo", "filme pedido (movie_request)", movie_request_now);
    }
    mode_cur = mode;
    mode_since_ms = now;
    mode_since_frame = frames;
}

/* --- ttex: texturas vivas, formatos e custo de subir -------------------------------- */

#define TEX_SLOTS 2048

static unsigned short tex_w[TEX_SLOTS], tex_h[TEX_SLOTS];
static int tx_live, tx_peak_kb;
static unsigned tx_live_px;                 /* soma de w*h das vivas */
static unsigned tx_up, tx_up_ms_total, tx_fail, tx_free, tx_part, tx_reload;
static unsigned tx_fmt[8], tx_dfmt[16];
static int tx_fail_last;

/* uma textura subiu (ok) ou falhou; type = formato | modo << 8; ms = tempo de decodificar e subir */
void zinst_tex_upload(int ok, unsigned type, int w, int h, int index, unsigned ms, int reload)
{
    tx_up_ms_total += ms;
    tx_reload += reload != 0;
    if (!ok) {
        tx_fail++;
        tx_fail_last = (int)type;
        return;
    }
    tx_up++;
    tx_fmt[type & 7u]++;
    tx_dfmt[(type >> 8) & 15u]++;
    if (index >= 0 && index < TEX_SLOTS) {
        if (tex_w[index] != 0) { /* o índice foi reaproveitado sem passar por zinst_tex_free */
            tx_live_px -= (unsigned)tex_w[index] * tex_h[index];
            tx_live--;
        }
        tex_w[index] = (unsigned short)w;
        tex_h[index] = (unsigned short)h;
        tx_live_px += (unsigned)w * (unsigned)h;
        tx_live++;
        if ((int)(tx_live_px >> 8) > tx_peak_kb) {
            tx_peak_kb = (int)(tx_live_px >> 8); /* KB de RGBA8888: px * 4 / 1024 */
        }
    }
}

void zinst_tex_free(int index)
{
    tx_free++;
    if (index >= 0 && index < TEX_SLOTS && tex_w[index] != 0) {
        tx_live_px -= (unsigned)tex_w[index] * tex_h[index];
        tx_live--;
        tex_w[index] = tex_h[index] = 0;
    }
}

void zinst_tex_part(void)
{
    tx_part++;
}

void zinst_tex_report(void)
{
    static int prev[14];
    static unsigned ms_prev;
    int now[14], i;
    static const char *const names[14] = {
        "uploads (total)", "falhas de decodificacao (total)", "ultimo tipo sem decodificador",
        "liberadas (total)", "vivas", "vivas: RGBA8888 nominal (KB)", "pico vivas: RGBA8888 (KB)",
        "equivalente 16 bpp (KB)", "equivalente ATITC 8 bpp (KB)", "equivalente ATITC 4 bpp (KB)",
        "pedacos kmLoadTexturePart (total)", "recargas sem liberar (total)",
        "ms subindo (janela)", "ms subindo (total)"};
    unsigned px = tx_live_px;

    now[0] = (int)tx_up;
    now[1] = (int)tx_fail;
    now[2] = tx_fail_last;
    now[3] = (int)tx_free;
    now[4] = tx_live;
    now[5] = (int)(px >> 8);
    now[6] = tx_peak_kb;
    now[7] = (int)(px >> 9);
    now[8] = (int)(px >> 10);
    now[9] = (int)(px >> 11);
    now[10] = (int)tx_part;
    now[11] = (int)tx_reload;
    now[12] = (int)(tx_up_ms_total - ms_prev);
    now[13] = (int)tx_up_ms_total;
    ms_prev = tx_up_ms_total;
    for (i = 0; i < 14; i++) {
        if (now[i] != prev[i]) {
            zlogd("ttex", names[i], now[i]);
        }
        prev[i] = now[i];
    }
    /* formatos: quantos uploads de cada um, só quando mudam (poucas vezes por fase) */
    {
        static unsigned pf[8], pd[16];
        static const char *const fmt_names[8] = {
            "formato ARGB1555 (uploads)", "formato RGB565 (uploads)", "formato ARGB4444 (uploads)",
            "formato YUV422 (uploads)", "formato BUMP (uploads)", "formato PAL4 (uploads)",
            "formato PAL8 (uploads)", "formato 7 (uploads)"};
        static const char *const dfmt_names[16] = {
            "modo 0 (uploads)", "modo TWIDDLED (uploads)", "modo TWIDDLED+MIPMAP (uploads)",
            "modo VQ (uploads)", "modo VQ+MIPMAP (uploads)", "modo PAL4 (uploads)",
            "modo PAL4+MIPMAP (uploads)", "modo PAL8 (uploads)", "modo PAL8+MIPMAP (uploads)",
            "modo RECTANGLE (uploads)", "modo 10 (uploads)", "modo STRIDE (uploads)",
            "modo 12 (uploads)", "modo TWIDDLED RECT (uploads)", "modo 14 (uploads)",
            "modo 15 (uploads)"};

        for (i = 0; i < 8; i++) {
            if (tx_fmt[i] != pf[i]) {
                zlogd("ttex", fmt_names[i], (int)tx_fmt[i]);
                pf[i] = tx_fmt[i];
            }
        }
        for (i = 0; i < 16; i++) {
            if (tx_dfmt[i] != pd[i]) {
                zlogd("ttex", dfmt_names[i], (int)tx_dfmt[i]);
                pd[i] = tx_dfmt[i];
            }
        }
    }
}
