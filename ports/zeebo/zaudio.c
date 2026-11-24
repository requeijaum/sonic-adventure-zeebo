/* zaudio.c — decodifica ADX do disco com o C do decomp e desenha a forma de
 * onda no zeebx. Ainda não há saída de áudio (BREW IMedia entra depois); o
 * objetivo é provar ADX -> PCM no alvo ARM32. */

#include <AEEStdLib.h>
#include <AEEShell.h>
#include <AEEClassIDs.h>
#include <AEEMedia.h>

void *memcpy(void *d, const void *s, unsigned int n);
#include <sofdec.h>
#include <sg_xpt.h>

#include "zstrip.h"
#include "zmix.h"

/* ganhos dos dois streams do jogo: a música e a voz (ADXT_SetOutVol) */
static IShell *audio_shell;
static void nat_stop_movie(void);
static void nat_stop_all(void);
static int music_gain = 256;
static int voice_gain = 256;

/* o ADX vem do armazenamento do app (IFileMgr), não do .mod */
#define ADX_FILE "FISHMISS.ADX;1"
#define ADX_CAP (192 * 1024)
static unsigned char zasset_adx[ADX_CAP];
static unsigned int zasset_adx_len;

static void adx_load(void)
{
    int n = zgdfs_read_file(ADX_FILE, zasset_adx, ADX_CAP);

    zasset_adx_len = n > 0 ? (unsigned int)n : 0;
}

#define BARS 128
#define CHUNK_BLOCKS 16 /* por canal */
#define CHUNK_SAMPLES (CHUNK_BLOCKS * 32)

static short wave_lo[BARS];
static short wave_hi[BARS];
static int ready;

/* clipe inteiro tocado pelo IMedia: até ~5,1 s estéreo 16 bits num WAV na
 * memória; o PCM é escrito direto após o cabeçalho (sem buffer duplicado) */
#define CLIP_FRAMES 163840
static unsigned char clip_wav[44 + CLIP_FRAMES * 4];
#define clip_pcm ((short *)(clip_wav + 44))
static IMedia *media;
static Sint32 clip_rate = 32000;

static Sint16 left[CHUNK_SAMPLES], right[CHUNK_SAMPLES];

void zaudio_init(void)
{
    adx_load();
    Sint16 dtofs;
    Sint8 enc, bps, blklen, nch;
    Sint32 sfreq, total, blksmpl;
    int cof;
    int blocks;
    int done = 0;
    int column = 0;
    int per_column;
    ADXPD xpd;
    const unsigned char *data;

    if (zasset_adx_len < 32) {
        return;
    }
    if (ADX_DecodeInfo((const Sint8 *)zasset_adx, (Sint32)zasset_adx_len,
                       &dtofs, &enc, &bps, &blklen, &nch, &sfreq, &total,
                       &blksmpl) != 0 ||
        nch != 2) {
        return;
    }
    cof = (zasset_adx[16] << 8) | zasset_adx[17];
    blocks = (int)((total + blksmpl - 1) / blksmpl);
    if (blocks <= 0) {
        return;
    }
    per_column = (blocks + BARS - 1) / BARS;
    if (per_column < 1) {
        per_column = 1;
    }

    ADXPD_Init();
    xpd = ADXPD_Create();
    if (xpd == 0) {
        return;
    }
    ADXPD_SetCoef(xpd, sfreq, cof);
    data = zasset_adx + dtofs;

    for (column = 0; column < BARS; column++) {
        int lo = 32767;
        int hi = -32768;

        for (int c = 0; c < per_column && done < blocks; c += CHUNK_BLOCKS) {
            int per = per_column - c;
            int nblk;

            if (per > CHUNK_BLOCKS) {
                per = CHUNK_BLOCKS;
            }
            if (done + per > blocks) {
                per = blocks - done;
            }
            if (per <= 0) {
                break;
            }
            nblk = per * 2;
            if (!ADXPD_EntrySte(xpd, (Sint8 *)data + (unsigned long)done * 2 * blklen,
                                nblk, left, right)) {
                break;
            }
            ADXPD_Start(xpd);
            ADXPD_ExecHndl(xpd);
            {
                int got = ADXPD_GetNumBlk(xpd);
                int ns = (got / 2) * blksmpl;

                if (ns > CHUNK_SAMPLES) {
                    ns = CHUNK_SAMPLES;
                }
                for (int i = 0; i < ns; i++) {
                    int a = left[i];
                    int b = right[i];

                    if (a < lo) lo = a;
                    if (a > hi) hi = a;
                    if (b < lo) lo = b;
                    if (b > hi) hi = b;
                }
                ADXPD_Reset(xpd);
                if (got <= 0) {
                    break;
                }
                done += got / 2;
            }
        }
        wave_lo[column] = (short)lo;
        wave_hi[column] = (short)hi;
    }
    ready = 1;
}

static void put32(unsigned char *p, unsigned int v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static void put16(unsigned char *p, unsigned int v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}

/* decodifica até CLIP_FRAMES quadros (L,R) para clip_pcm */
static int decode_clip(void)
{
    Sint16 dtofs;
    Sint8 enc, bps, blklen, nch;
    Sint32 sfreq, total, blksmpl;
    int cof;
    int blocks;
    int done = 0;
    int framed = 0;
    ADXPD xpd;
    const unsigned char *data;

    if (zasset_adx_len < 32 ||
        ADX_DecodeInfo((const Sint8 *)zasset_adx, (Sint32)zasset_adx_len,
                       &dtofs, &enc, &bps, &blklen, &nch, &sfreq, &total,
                       &blksmpl) != 0 ||
        nch != 2) {
        return 0;
    }
    cof = (zasset_adx[16] << 8) | zasset_adx[17];
    clip_rate = sfreq;
    blocks = (int)((total + blksmpl - 1) / blksmpl);
    ADXPD_Init();
    xpd = ADXPD_Create();
    if (xpd == 0) {
        return 0;
    }
    ADXPD_SetCoef(xpd, sfreq, cof);
    data = zasset_adx + dtofs;
    while (done < blocks && framed < CLIP_FRAMES) {
        int per = blocks - done;
        int nblk;
        int got;
        int ns;

        if (per > CHUNK_BLOCKS) {
            per = CHUNK_BLOCKS;
        }
        nblk = per * 2;
        if (!ADXPD_EntrySte(xpd, (Sint8 *)data + (unsigned long)done * 2 * blklen,
                            nblk, left, right)) {
            break;
        }
        ADXPD_Start(xpd);
        ADXPD_ExecHndl(xpd);
        got = ADXPD_GetNumBlk(xpd);
        ns = (got / 2) * blksmpl;
        if (ns > CHUNK_SAMPLES) {
            ns = CHUNK_SAMPLES;
        }
        for (int i = 0; i < ns && framed < CLIP_FRAMES && framed < total; i++) {
            clip_pcm[framed * 2 + 0] = left[i];
            clip_pcm[framed * 2 + 1] = right[i];
            framed++;
        }
        ADXPD_Reset(xpd);
        if (got <= 0) {
            break;
        }
        done += got / 2;
    }
    return framed;
}

void zaudio_stop(void)
{
    if (media != 0) {
        IMEDIA_Stop(media);
        IMEDIA_Release(media);
        media = 0;
    }
    zmix_stream_stop(1); /* o áudio do filme */
    nat_stop_movie();
}

/* fim do app: desliga o mixer inteiro */
void zaudio_shutdown(void)
{
    zaudio_stop();
    nat_stop_all();
    zmix_shutdown();
}

/* toca o trecho pelo IMedia do BREW (WAV em memória) */
void zaudio_start(IShell *shell)
{
#ifdef ZAUDIO_OFF
    (void)shell;
    return;
#endif
    int frames = decode_clip();
    unsigned int data_bytes;
    unsigned char *w = clip_wav;
    AEEMediaData md;

    if (frames <= 0 || shell == 0) {
        return;
    }
    data_bytes = (unsigned int)frames * 4;
    memcpy(w, "RIFF", 4);
    put32(w + 4, 36 + data_bytes);
    memcpy(w + 8, "WAVEfmt ", 8);
    put32(w + 16, 16);
    put16(w + 20, 1);
    put16(w + 22, 2);
    put32(w + 24, (unsigned int)clip_rate);
    put32(w + 28, (unsigned int)clip_rate * 4);
    put16(w + 32, 4);
    put16(w + 34, 16);
    memcpy(w + 36, "data", 4);
    put32(w + 40, data_bytes);

    zaudio_stop();
    if (ISHELL_CreateInstance(shell, AEECLSID_MEDIA, (void **)&media) !=
            AEE_SUCCESS ||
        media == 0) {
        media = 0;
        return;
    }
    md.clsData = MMD_BUFFER;
    md.pData = clip_wav;
    md.dwSize = 44 + data_bytes;
    if (IMEDIA_SetMediaData(media, &md) != AEE_SUCCESS ||
        IMEDIA_Play(media) != AEE_SUCCESS) {
        IMEDIA_Release(media);
        media = 0;
    }
}

void zaudio_frame(uint32_t tick)
{
    (void)tick;
    if (!ready) {
        return;
    }
    /* painel escuro atrás da onda */
    zstrip_no_texture();
    zstrip_blend(0);
    zstrip_begin();
    zstrip_vertex(0.0f, 300.0f, 0.95f, 0.0f, 0.0f, 0xff101018u);
    zstrip_vertex(640.0f, 300.0f, 0.95f, 0.0f, 0.0f, 0xff101018u);
    zstrip_vertex(0.0f, 430.0f, 0.95f, 0.0f, 0.0f, 0xff101018u);
    zstrip_vertex(640.0f, 430.0f, 0.95f, 0.0f, 0.0f, 0xff101018u);
    zstrip_end();

    /* cada coluna é uma barra de min..max, escala 32768 -> 60 px */
    zstrip_blend(0);
    for (int i = 0; i < BARS; i++) {
        float x0 = (float)i * 5.0f;
        float y0 = 365.0f - (float)wave_hi[i] * (60.0f / 32768.0f);
        float y1 = 365.0f - (float)wave_lo[i] * (60.0f / 32768.0f);

        if (y1 - y0 < 1.0f) {
            y1 = y0 + 1.0f;
        }
        zstrip_begin();
        zstrip_vertex(x0, y0, 0.96f, 0.0f, 0.0f, 0xff30e070u);
        zstrip_vertex(x0 + 4.0f, y0, 0.96f, 0.0f, 0.0f, 0xff30e070u);
        zstrip_vertex(x0, y1, 0.96f, 0.0f, 0.0f, 0xff30e070u);
        zstrip_vertex(x0 + 4.0f, y1, 0.96f, 0.0f, 0.0f, 0xff30e070u);
        zstrip_end();
    }
}

/* --- música do jogo (src/sound/stream.c via zADXT_*) -----------------------
 * O jogo pede um ADX por nome ao ADXT do Sofdec, que sai pelo driver de som
 * do DC (stubs, mudo). Aqui o mesmo arquivo é lido inteiro, decodificado para
 * PCM e tocado pelo IMedia como WAV em memória, uma vez (sem laço): até
 * MUSIC_SECONDS de música. */
#define MUSIC_SECONDS 40

void zaudio_set_shell(IShell *shell)
{
    audio_shell = shell;
#ifndef ZAUDIO_OFF
    zmix_start(shell);
#endif
}

double sqrt(double x);
double cos(double x);

static int be16(const unsigned char *p) { return (p[0] << 8) | p[1]; }
static unsigned int be32(const unsigned char *p)
{
    return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) |
           ((unsigned int)p[2] << 8) | p[3];
}
static int round_int(double v) { return v < 0 ? -(int)(-v + 0.5) : (int)(v + 0.5); }

/* ADX tipo 3 (blocos de 18 bytes, 4 bits) direto em inteiro: o mesmo
 * resultado do ADXPD (conferido bit a bit contra o ffmpeg no host), muito
 * mais barato. Devolve -1 para o que não for esse formato. */
static int decode_adx_fast(const unsigned char *a, int len, short *pcm,
                           int most, Sint32 *rate)
{
    int off, bs, nch, spf, n = 0, ch, i;
    unsigned int total;
    int h1[2] = {0, 0}, h2[2] = {0, 0};
    int c1, c2;
    const unsigned char *p;
    double z, A, B, C;
    const double pi = 3.14159265358979323846;
    const double r2 = 1.41421356237309504880;

    if (len < 32 || a[0] != 0x80 || a[4] != 3 || a[5] != 18 || a[6] != 4 ||
        (a[7] != 1 && a[7] != 2)) {
        return -1;
    }
    off = be16(a + 2) + 4;
    bs = a[5];
    nch = a[7];
    *rate = (Sint32)be32(a + 8);
    total = be32(a + 12);
    z = cos(2.0 * pi * be16(a + 16) / (double)*rate);
    A = r2 - z;
    B = r2 - 1.0;
    C = (A - sqrt((A + B) * (A - B))) / B;
    c1 = round_int(C * 8192.0);
    c2 = round_int(C * C * -4096.0);
    spf = (bs - 2) * 2;
    p = a + off;
    while (n < most && (unsigned int)n < total && p + bs * nch <= a + len) {
        int m = spf;

        if (n + m > most) m = most - n;
        if ((unsigned int)(n + m) > total) m = (int)(total - (unsigned int)n);
        for (ch = 0; ch < nch; ch++) {
            const unsigned char *f = p + ch * bs;
            int head = be16(f);
            int scale = head & 0x1fff;
            int s1 = h1[ch], s2 = h2[ch];
            short *o = pcm + n * 2 + ch;

            if (head & 0x8000) {
                return n; /* bloco de fim */
            }
            for (i = 0; i < m; i++) {
                int b = f[2 + (i >> 1)];
                int d = (i & 1) ? (b & 15) : (b >> 4);
                int s;

                if (d & 8) d -= 16;
                s = d * scale + ((c1 * s1 + c2 * s2) >> 12);
                if (s > 32767) s = 32767;
                if (s < -32768) s = -32768;
                o[i * 2] = (short)s;
                if (nch == 1) o[i * 2 + 1] = (short)s;
                s2 = s1;
                s1 = s;
            }
            h1[ch] = s1;
            h2[ch] = s2;
        }
        n += m;
        p += bs * nch;
    }
    return n;
}

static int decode_adx(const unsigned char *adx, int len, short *pcm, int most,
                      Sint32 *rate)
{
    Sint16 dtofs;
    Sint8 enc, bps, blklen, nch;
    Sint32 sfreq, total, blksmpl;
    int cof;
    int fast = decode_adx_fast(adx, len, pcm, most, rate);
    int blocks;
    int done = 0;
    int framed = 0;
    ADXPD xpd;

    if (fast >= 0) {
        return fast;
    }
    if (len < 32 ||
        ADX_DecodeInfo((const Sint8 *)adx, (Sint32)len, &dtofs, &enc, &bps,
                       &blklen, &nch, &sfreq, &total, &blksmpl) != 0 ||
        (nch != 1 && nch != 2)) {
        return 0;
    }
    cof = (adx[16] << 8) | adx[17];
    *rate = sfreq;
    blocks = (int)((total + blksmpl - 1) / blksmpl);
    ADXPD_Init();
    xpd = ADXPD_Create();
    if (xpd == 0) {
        return 0;
    }
    ADXPD_SetCoef(xpd, sfreq, cof);
    while (done < blocks && framed < most) {
        int per = blocks - done;
        int got;
        int ns;
        int i;

        if (per > CHUNK_BLOCKS) {
            per = CHUNK_BLOCKS;
        }
        if ((long)dtofs + (long)(done + per) * nch * blklen > len) {
            break; /* arquivo truncado */
        }
        if (nch == 2) {
            if (!ADXPD_EntrySte(xpd, (Sint8 *)adx + dtofs + (long)done * 2 * blklen,
                                per * 2, left, right)) {
                break;
            }
        } else if (!ADXPD_EntryMono(xpd, (Sint8 *)adx + dtofs + (long)done * blklen,
                                    per, left)) {
            break;
        }
        ADXPD_Start(xpd);
        ADXPD_ExecHndl(xpd);
        got = ADXPD_GetNumBlk(xpd) / nch;
        ns = got * blksmpl;
        if (ns > CHUNK_SAMPLES) {
            ns = CHUNK_SAMPLES;
        }
        for (i = 0; i < ns && framed < most && framed < total; i++) {
            pcm[framed * 2 + 0] = left[i];
            pcm[framed * 2 + 1] = nch == 2 ? right[i] : left[i];
            framed++;
        }
        ADXPD_Reset(xpd);
        if (got <= 0) {
            break;
        }
        done += got;
    }
    ADXPD_Destroy(xpd);
    return framed;
}

/* toca um ADX em memória (o áudio de um filme) no stream de voz do mixer; o
 * mixer guarda a própria cópia, então o chamador pode liberar adx. seconds
 * ficou por compatibilidade: o stream não tem mais limite de duração */
void zlog(const char *what, const char *name, int value);
void zaudio_play_adx(const unsigned char *adx, int len, int seconds)
{
#ifdef ZAUDIO_OFF
    (void)adx; (void)len; (void)seconds;
    return;
#endif
    (void)seconds;
    zaudio_stop();
    if (adx == 0 || len <= 0) {
        return;
    }
    zmix_stream_mem(1, adx, len, 256);
}

/* nome do disco -> arquivo do app: sem ";N" e, se faltar, em minúsculas */
static void *open_disc_file(const char *name, int *size)
{
    char buf[40];
    int n = 0, i;
    void *h;

    while (name[n] != 0 && name[n] != ';' && n < 39) {
        buf[n] = name[n];
        n++;
    }
    buf[n] = 0;
    *size = zfile_size(buf);
    if (*size < 0) {
        for (i = 0; i < n; i++) {
            if (buf[i] >= 'A' && buf[i] <= 'Z') {
                buf[i] = (char)(buf[i] + 32);
            }
        }
        *size = zfile_size(buf);
    }
    if (*size < 0) {
        return 0;
    }
    h = zfile_handle_open(buf);
    return h;
}

/* ADXT_SetOutVol: décimos de dB (0 = cheio, -960 = mudo) -> ganho linear
 * 0..256. Tabela por dB inteiro, cada degrau vezes 10^(-1/20) em ponto fixo. */
static int vol_to_gain(int v)
{
    static unsigned short tab[97];
    static int ready_tab;
    int db;

    if (!ready_tab) {
        unsigned g = 256u << 16;
        int k;

        for (k = 0; k < 97; k++) {
            tab[k] = (unsigned short)(g >> 16);
            g = (unsigned)(((unsigned long long)g * 58409u) >> 16);
        }
        ready_tab = 1;
    }
    if (v >= 0) {
        return 256;
    }
    db = (-v + 5) / 10;
    return db >= 96 ? 0 : tab[db];
}

/* --- reprodução nativa (DSP) ------------------------------------------------
 * A música e o áudio dos filmes são MP3 (tools/adx2media.py), tocados pelo
 * IMedia direto do arquivo: o DSP decodifica, sem custo de CPU nem
 * dependência da taxa de quadros do jogo. A faixa com laço vem em dois
 * arquivos: nome.mp3 (inteira, uma vez) e nome_lp.mp3 (do início do laço ao
 * fim, repetida sem fim), encadeados no aviso de fim (MM_STATUS_DONE). A voz
 * (event_adx.zva) é IMA ADPCM .wav em memória. Sem esses arquivos, o ADX é
 * tocado pelo mixer por software (zmix.c). */

#define ZMM_FILE_NAME 0
#define ZMM_BUFFER 1
#define ZMM_PARM_PLAY_REPEAT 11
#define ZMM_STATUS_DONE 2
#define ZMM_CMD_PLAY 4

typedef struct {
    IMedia *m;
    char loopfile[48];
    char fname[48];
    int loop;           /* ao acabar, tocar loopfile para sempre */
    int gain;           /* 0..256 */
    unsigned char *buf; /* voz/efeito: o .wav em memória */
    int key;            /* efeito: canal MIDI ou porta de sequência dono */
    int done;           /* o IMedia avisou o fim */
    unsigned age;       /* ordem de início, para roubar o mais antigo */
    int looped;         /* repete até a nota acabar */
    unsigned char *buf2; /* efeito sustentado: o trecho de laço, em memória */
    int len2;
} ZNat;

/* contadores do som: o relatorio de cada janela (zaudio_report) so imprime o que mudou */
static struct {
    int sfx_play, sfx_seq, sfx_miss, sfx_steal, sfx_loopsw;
    int sfx_e_open, sfx_e_read, sfx_e_inst, sfx_e_data, sfx_e_play, sfx_e_big;
    int sfx_last_play, sfx_last_miss;
    int voz_req, voz_nat, voz_mix;
    int mus_nat, mus_mix, mus_loopsw, filme_nat;
} zst;

static ZNat nat_music, nat_movie, nat_voice;

static void nat_free(ZNat *n)
{
    if (n->m != 0) {
        IMEDIA_RegisterNotify(n->m, 0, 0);
        IMEDIA_Stop(n->m);
        IMEDIA_Release(n->m);
        n->m = 0;
    }
    if (n->buf != 0) {
        FREE(n->buf);
        n->buf = 0;
    }
    if (n->buf2 != 0) {
        FREE(n->buf2);
        n->buf2 = 0;
    }
    n->loop = 0;
    n->done = 0;
    n->looped = 0;
}

static void nat_volume(ZNat *n)
{
    if (n->m != 0) {
        IMEDIA_SetVolume(n->m, (uint16)((n->gain * 100 + 128) / 256));
    }
}

static void nat_notify(void *user, AEEMediaCmdNotify *c)
{
    ZNat *n = (ZNat *)user;

    if (n != 0 && c != 0 && c->nCmd == ZMM_CMD_PLAY && c->nStatus == ZMM_STATUS_DONE) {
        n->done = 1;
    }
    if (n != 0 && n->m != 0 && n->loop == 2 && n->buf2 != 0 && c != 0 &&
        c->nCmd == ZMM_CMD_PLAY && c->nStatus == ZMM_STATUS_DONE) {
        AEEMediaData md2;

        /* a introdução do efeito acabou: o mesmo objeto toca o laço, sem fim */
        zst.sfx_loopsw++;
        n->loop = 0;
        n->done = 0;
        md2.clsData = ZMM_BUFFER;
        md2.pData = n->buf2;
        md2.dwSize = (uint32)n->len2;
        if (IMEDIA_SetMediaData(n->m, &md2) == AEE_SUCCESS) {
            IMEDIA_SetMediaParm(n->m, ZMM_PARM_PLAY_REPEAT, 0, 0);
            IMEDIA_Play(n->m);
        }
        return;
    }
    if (n == 0 || n->m == 0 || c == 0 || c->nCmd != ZMM_CMD_PLAY ||
        c->nStatus != ZMM_STATUS_DONE || !n->loop) {
        return;
    }
    zst.mus_loopsw++;
    {
        AEEMediaData md;
        int i;

        /* a faixa inteira acabou: o mesmo objeto passa ao laço */
        n->loop = 0;
        for (i = 0; n->loopfile[i] != 0 && i < 47; i++) {
            n->fname[i] = n->loopfile[i];
        }
        n->fname[i] = 0;
        md.clsData = ZMM_FILE_NAME;
        md.pData = (void *)n->fname;
        md.dwSize = 0;
        if (IMEDIA_SetMediaData(n->m, &md) == AEE_SUCCESS) {
            IMEDIA_SetMediaParm(n->m, ZMM_PARM_PLAY_REPEAT, 0, 0); /* 0: sem fim */
            IMEDIA_Play(n->m);
        }
    }
}

/* arquivo nativo do ADX do jogo: "TITLE2.ADX;1" + ".mp3" -> "title2.mp3";
 * 1 se existe */
static int native_name(const char *name, const char *ext, char *out, int cap)
{
    int n = 0, dot = -1, i;

    while (name[n] != 0 && name[n] != ';' && n < cap - 12) {
        char c = name[n];

        out[n] = (char)(c >= 'A' && c <= 'Z' ? c + 32 : c);
        if (c == '.') {
            dot = n;
        }
        n++;
    }
    if (dot >= 0) {
        n = dot;
    }
    for (i = 0; ext[i] != 0; i++) {
        out[n + i] = ext[i];
    }
    out[n + i] = 0;
    return zfile_size(out) > 0;
}

/* toca file (MP3 do disco) em n; loopname != 0: depois dele, o laço */
static int nat_start_file(ZNat *n, const char *file, const char *loopname, int gain)
{
    AEEMediaData md;
    int i;

    nat_free(n);
    if (audio_shell == 0 ||
        ISHELL_CreateInstance(audio_shell, AEECLSID_MEDIA, (void **)&n->m) != AEE_SUCCESS ||
        n->m == 0) {
        n->m = 0;
        return 0;
    }
    n->gain = gain;
    if (loopname != 0) {
        for (i = 0; loopname[i] != 0 && i < 47; i++) {
            n->loopfile[i] = loopname[i];
        }
        n->loopfile[i] = 0;
        n->loop = 1;
    }
    for (i = 0; file[i] != 0 && i < 47; i++) {
        n->fname[i] = file[i];
    }
    n->fname[i] = 0;
    md.clsData = ZMM_FILE_NAME;
    md.pData = (void *)n->fname; /* o IMedia pode ler depois do retorno */
    md.dwSize = 0;
    IMEDIA_RegisterNotify(n->m, nat_notify, n);
    if (IMEDIA_SetMediaData(n->m, &md) != AEE_SUCCESS) {
        nat_free(n);
        return 0;
    }
    nat_volume(n);
    if (IMEDIA_Play(n->m) != AEE_SUCCESS) {
        nat_free(n);
        return 0;
    }
    return 1;
}

/* o áudio de um filme (nome do .sfd): 1 se tocou o MP3 */
int zaudio_play_movie(const char *name)
{
#ifdef ZAUDIO_OFF
    (void)name;
    return 0;
#else
    char mp3[48];

    nat_free(&nat_movie);
    if (!native_name(name, ".mp3", mp3, sizeof mp3)) {
        return 0;
    }
    if (nat_start_file(&nat_movie, mp3, 0, 256)) {
        zst.filme_nat++;
        return 1;
    }
    return 0;
#endif
}

/* a voz: entrada flid do contêiner event_adx.zva (IMA ADPCM .wav) */
static int nat_start_voice(int flid)
{
    unsigned char hd[8], ent[8];
    int size, n, off, len;
    void *h = open_disc_file("event_adx.zva", &size);
    AEEMediaData md;

    nat_free(&nat_voice);
    if (h == 0) {
        return 0;
    }
    if (zfile_handle_read(h, hd, 8) != 8 || hd[0] != 'Z' || hd[1] != 'V' || hd[2] != 'A') {
        zfile_handle_close(h);
        return 0;
    }
    n = (int)(hd[4] | (hd[5] << 8) | (hd[6] << 16) | ((unsigned)hd[7] << 24));
    if (flid < 0 || flid >= n || zfile_handle_seek(h, 8 + flid * 8) != 0 ||
        zfile_handle_read(h, ent, 8) != 8) {
        zfile_handle_close(h);
        return 0;
    }
    off = (int)(ent[0] | (ent[1] << 8) | (ent[2] << 16) | ((unsigned)ent[3] << 24));
    len = (int)(ent[4] | (ent[5] << 8) | (ent[6] << 16) | ((unsigned)ent[7] << 24));
    if (len <= 44 || len > (1 << 20) || zfile_handle_seek(h, off) != 0) {
        zfile_handle_close(h);
        return 0;
    }
    nat_voice.buf = (unsigned char *)MALLOC((uint32)len);
    if (nat_voice.buf == 0 || zfile_handle_read(h, nat_voice.buf, len) != len) {
        zfile_handle_close(h);
        nat_free(&nat_voice);
        return 0;
    }
    zfile_handle_close(h);
    if (audio_shell == 0 ||
        ISHELL_CreateInstance(audio_shell, AEECLSID_MEDIA, (void **)&nat_voice.m) !=
            AEE_SUCCESS ||
        nat_voice.m == 0) {
        nat_voice.m = 0;
        nat_free(&nat_voice);
        return 0;
    }
    nat_voice.gain = voice_gain;
    md.clsData = ZMM_BUFFER;
    md.pData = nat_voice.buf;
    md.dwSize = (uint32)len;
    if (IMEDIA_SetMediaData(nat_voice.m, &md) != AEE_SUCCESS) {
        nat_free(&nat_voice);
        return 0;
    }
    nat_volume(&nat_voice);
    if (IMEDIA_Play(nat_voice.m) != AEE_SUCCESS) {
        nat_free(&nat_voice);
        return 0;
    }
    return 1;
}

/* --- efeitos sonoros --------------------------------------------------------
 * O jogo toca os efeitos pela biblioteca de som do Dreamcast (MIDI para o
 * driver MASARU.DRV do AICA). Aqui o sdMidi* é nosso: cada MLT carregado
 * tem um .zsf renderizado no host com o backend de som do PC
 * (tools/sfx_render.cpp): um clipe IMA ADPCM por programa (nota 60) e por
 * sequência, que o IMedia toca. 'ZSF1', n, n x {tipo, banco, número, flags,
 * taxa, deslocamento, tamanho}. flags bit 0: soava no fim do clipe (repete
 * até a nota acabar). */

/* o DSP toca no maximo 4 objetos codificados ao mesmo tempo (guia do Zeebo, 9.2): musica (1) +
 * voz (1) + 2 efeitos; um quinto objeto falha no aparelho (o zeebx nao impoe o limite) */
#define SFX_POOL 2
#define SFX_SETS 8
#define SFX_PORTS 8
#define SFX_MAXCLIP (1 << 20)

typedef struct {
    char name[28]; /* o .zsf */
    unsigned char *idx;
    int n;
} ZSfxSet;

static ZSfxSet sfx_sets[SFX_SETS];
static ZNat sfx_pool[SFX_POOL];
static unsigned sfx_clock;
static void *sfx_port_h[SFX_PORTS];
static struct {
    unsigned char bank, program, volume;
} sfx_chan[SFX_PORTS * 16];
static unsigned char sfx_seq_vol[SFX_PORTS];

extern const char *zgdfs_last_name(void);

static int sfx_name_eq(const char *x, const char *y)
{
    while (*x != 0 && *x == *y) {
        x++;
        y++;
    }
    return *x == *y;
}

static void sfx_set_free(ZSfxSet *s)
{
    if (s->idx != 0) {
        FREE(s->idx);
    }
    s->idx = 0;
    s->n = 0;
    s->name[0] = 0;
}

/* sdMultiUnitDownload: o MLT que o jogo acabou de ler vira o conjunto de clipes mais recente */
static void sfx_register(const char *mlt)
{
    char zsf[28];
    unsigned char hd[8];
    int size, n, i, k;
    void *h;

    if (!native_name(mlt, ".zsf", zsf, sizeof zsf)) {
        return;
    }
    for (k = 0; k < SFX_SETS - 1; k++) {
        if (sfx_sets[k].idx != 0 && sfx_name_eq(sfx_sets[k].name, zsf)) {
            break;
        }
    }
    /* k: o conjunto de mesmo nome, ou o último (o mais antigo): sai, e o resto desce */
    sfx_set_free(&sfx_sets[k]);
    for (; k > 0; k--) {
        sfx_sets[k] = sfx_sets[k - 1];
    }
    sfx_sets[0].idx = 0;
    sfx_sets[0].n = 0;
    sfx_sets[0].name[0] = 0;
    h = open_disc_file(zsf, &size);
    if (h == 0) {
        return;
    }
    if (zfile_handle_read(h, hd, 8) == 8 && hd[0] == 'Z' && hd[1] == 'S' && hd[2] == 'F') {
        n = (int)(hd[4] | (hd[5] << 8) | (hd[6] << 16) | ((unsigned)hd[7] << 24));
        if (n > 0 && n < 4096) {
            sfx_sets[0].idx = (unsigned char *)MALLOC((uint32)(n * 16));
            if (sfx_sets[0].idx != 0 &&
                zfile_handle_read(h, sfx_sets[0].idx, n * 16) == n * 16) {
                sfx_sets[0].n = n;
                for (i = 0; zsf[i] != 0 && i < 27; i++) {
                    sfx_sets[0].name[i] = zsf[i];
                }
                sfx_sets[0].name[i] = 0;
            } else {
                sfx_set_free(&sfx_sets[0]);
            }
        }
    }
    zfile_handle_close(h);
}

/* o clipe (tipo, banco, número) do conjunto mais recente que o tem; 0 se não há */
static int sfx_find(int kind, int bank, int number, ZSfxSet **set, const unsigned char **ent)
{
    int k, i;

    for (k = 0; k < SFX_SETS; k++) {
        for (i = 0; i < sfx_sets[k].n; i++) {
            const unsigned char *e = sfx_sets[k].idx + i * 16;

            if (e[0] == kind && e[1] == bank && e[2] == number) {
                *set = &sfx_sets[k];
                *ent = e;
                return 1;
            }
        }
    }
    return 0;
}

static void sfx_stop_key(int key)
{
    int i;

    for (i = 0; i < SFX_POOL; i++) {
        if (sfx_pool[i].m != 0 && sfx_pool[i].key == key) {
            nat_free(&sfx_pool[i]);
        }
    }
}

static unsigned sfx_played, sfx_missed;

static void sfx_count(unsigned *c, const char *const *names, int val)
{
    unsigned v = ++*c;

    if ((v & (v - 1)) == 0) {
        int e = 0;

        while (v > 1) {
            v >>= 1;
            e++;
        }
        zlog("tsfx", names[e < 9 ? e : 8], val);
    }
}

static void sfx_play(int kind, int bank, int number, int key, int volume)
{
#ifndef ZAUDIO_OFF
    ZSfxSet *set;
    const unsigned char *e;
    int i, pick = -1, len, off;
    void *h;
    int size;
    AEEMediaData md;
    ZNat *n;

    if (audio_shell == 0) {
        return;
    }
    if (!sfx_find(kind, bank, number, &set, &e)) {
        static const char *const miss[] = {"sem clipe 1", "sem clipe 2", "sem clipe 4",
                                           "sem clipe 8", "sem clipe 16", "sem clipe 32",
                                           "sem clipe 64", "sem clipe 128", "sem clipe mais"};
        sfx_count(&sfx_missed, miss, kind << 16 | bank << 8 | number);
        zst.sfx_miss++;
        zst.sfx_last_miss = kind << 16 | bank << 8 | number;
        return;
    }
    {
        static const char *const hit[] = {"clipe tocado 1", "clipe tocado 2", "clipe tocado 4",
                                          "clipe tocado 8", "clipe tocado 16", "clipe tocado 32",
                                          "clipe tocado 64", "clipe tocado 128", "clipe tocado mais"};
        sfx_count(&sfx_played, hit, kind << 16 | bank << 8 | number);
        zst.sfx_play++;
        zst.sfx_seq += kind;
        zst.sfx_last_play = kind << 16 | bank << 8 | number;
    }
    off = (int)(e[8] | (e[9] << 8) | (e[10] << 16) | ((unsigned)e[11] << 24));
    len = (int)(e[12] | (e[13] << 8) | (e[14] << 16) | ((unsigned)e[15] << 24));
    if (len <= 44 || len > SFX_MAXCLIP) {
        zst.sfx_e_big++;
        return;
    }
    sfx_stop_key(key); /* uma nota nova no mesmo canal troca a anterior */
    for (i = 0; i < SFX_POOL; i++) {
        if (sfx_pool[i].m == 0 || sfx_pool[i].done) {
            pick = i;
            break;
        }
    }
    if (pick < 0) {
        zst.sfx_steal++;
        pick = 0;
        for (i = 1; i < SFX_POOL; i++) {
            if (sfx_pool[i].age < sfx_pool[pick].age) {
                pick = i;
            }
        }
    }
    n = &sfx_pool[pick];
    nat_free(n);
    h = open_disc_file(set->name, &size);
    if (h == 0) {
        zst.sfx_e_open++;
        return;
    }
    n->buf = (unsigned char *)MALLOC((uint32)len);
    if (n->buf == 0 || zfile_handle_seek(h, off) != 0 || zfile_handle_read(h, n->buf, len) != len) {
        zfile_handle_close(h);
        zst.sfx_e_read++;
        nat_free(n);
        return;
    }
    zfile_handle_close(h);
    if (ISHELL_CreateInstance(audio_shell, AEECLSID_MEDIA, (void **)&n->m) != AEE_SUCCESS ||
        n->m == 0) {
        n->m = 0;
        zst.sfx_e_inst++;
        nat_free(n);
        return;
    }
    md.clsData = ZMM_BUFFER;
    md.pData = n->buf;
    md.dwSize = (uint32)len;
    IMEDIA_RegisterNotify(n->m, nat_notify, n);
    if (IMEDIA_SetMediaData(n->m, &md) != AEE_SUCCESS) {
        zst.sfx_e_data++;
        nat_free(n);
        return;
    }
    n->key = key;
    n->age = ++sfx_clock;
    n->gain = volume * 256 / 127;
    n->looped = e[3] & 1;
    nat_volume(n);
    if (n->looped) {
        ZSfxSet *set2;
        const unsigned char *e2;

        /* sustentado: a introdução toca uma vez e o trecho de laço segue sem fim; sem o
         * trecho (conjunto antigo), repete o clipe inteiro */
        if (sfx_find(kind | 0x80, bank, number, &set2, &e2) && set2 == set) {
            int off2 = (int)(e2[8] | (e2[9] << 8) | (e2[10] << 16) | ((unsigned)e2[11] << 24));
            int len2 = (int)(e2[12] | (e2[13] << 8) | (e2[14] << 16) | ((unsigned)e2[15] << 24));
            void *h2 = open_disc_file(set->name, &size);

            if (h2 != 0) {
                if (len2 > 44 && len2 <= SFX_MAXCLIP) {
                    n->buf2 = (unsigned char *)MALLOC((uint32)len2);
                    if (n->buf2 != 0 &&
                        (zfile_handle_seek(h2, off2) != 0 ||
                         zfile_handle_read(h2, n->buf2, len2) != len2)) {
                        FREE(n->buf2);
                        n->buf2 = 0;
                    }
                    n->len2 = len2;
                }
                zfile_handle_close(h2);
            }
        }
        if (n->buf2 != 0) {
            n->loop = 2;
        } else {
            IMEDIA_SetMediaParm(n->m, ZMM_PARM_PLAY_REPEAT, 0, 0);
        }
    }
    if (IMEDIA_Play(n->m) != AEE_SUCCESS) {
        zst.sfx_e_play++;
        nat_free(n);
    }
#else
    (void)kind; (void)bank; (void)number; (void)key; (void)volume;
#endif
}

static void sfx_midi(int port, int status, int d1, int d2)
{
    int ch = status & 15, k = port * 16 + ch;

    if (port < 0 || port >= SFX_PORTS) {
        return;
    }
    switch (status & 0xf0) {
    case 0xb0:
        if (d1 == 32) {
            sfx_chan[k].bank = (unsigned char)d2;
        } else if (d1 == 7) {
            sfx_chan[k].volume = (unsigned char)d2;
            {
                int i;

                for (i = 0; i < SFX_POOL; i++) {
                    if (sfx_pool[i].m != 0 && sfx_pool[i].key == k) {
                        sfx_pool[i].gain = d2 * 256 / 127;
                        nat_volume(&sfx_pool[i]);
                    }
                }
            }
        }
        break;
    case 0xc0:
        sfx_chan[k].program = (unsigned char)d1;
        break;
    case 0x90:
        if (d2 > 0) {
            sfx_play(0, sfx_chan[k].bank, sfx_chan[k].program, k,
                     sfx_chan[k].volume ? sfx_chan[k].volume : 100);
            break;
        }
        /* velocidade 0: note off */
    case 0x80:
    {
        int i;

        for (i = 0; i < SFX_POOL; i++) {
            if (sfx_pool[i].m != 0 && sfx_pool[i].key == k && sfx_pool[i].looped) {
                nat_free(&sfx_pool[i]);
            }
        }
        break;
    }
    default:
        break;
    }
}

static int sfx_port_of(void **h)
{
    long i = h == 0 ? -1 : (long)(h - sfx_port_h);

    return i >= 0 && i < SFX_PORTS ? (int)i : -1;
}

int sdMidiOpenPort(void ***handle)
{
    static int next;

    if (handle == 0) {
        return -1;
    }
    *handle = &sfx_port_h[next];
    next = (next + 1) % SFX_PORTS;
    return 0;
}

int sdMidiSetMes(void **handle, unsigned char *mes, int status, ...)
{
    __builtin_va_list ap;
    int d1, d2 = 0;

    (void)handle;
    __builtin_va_start(ap, status);
    d1 = __builtin_va_arg(ap, int);
    if ((status & 0xf0) != 0xc0 && (status & 0xf0) != 0xd0) {
        d2 = __builtin_va_arg(ap, int);
    }
    __builtin_va_end(ap);
    mes[0] = (unsigned char)status;
    mes[1] = (unsigned char)d1;
    mes[2] = (unsigned char)d2;
    mes[3] = 0;
    return 0;
}

int sdMidiSendMes(void **handle, const unsigned char *mes, int delay)
{
    (void)delay;
    if (mes != 0 && mes[0] >= 0x80) {
        sfx_midi(sfx_port_of(handle), mes[0], mes[1], mes[2]);
    }
    return 0;
}

int sdMidiPlay(void **handle, int bank_num, int data_num, int prio)
{
    int p = sfx_port_of(handle);

    (void)prio;
    if (p >= 0) {
        sfx_play(1, bank_num, data_num, 1000 + p, sfx_seq_vol[p] ? sfx_seq_vol[p] : 127);
    }
    return 0;
}

int sdMidiStop(void **handle)
{
    int p = sfx_port_of(handle);

    if (p >= 0) {
        sfx_stop_key(1000 + p);
    }
    return 0;
}

int sdMidiSetVol(void **handle, int vol, int fade_time)
{
    int p = sfx_port_of(handle), i;

    (void)fade_time;
    if (p >= 0) {
        sfx_seq_vol[p] = (unsigned char)(vol < 0 ? 0 : vol);
        for (i = 0; i < SFX_POOL; i++) {
            if (sfx_pool[i].m != 0 && sfx_pool[i].key == 1000 + p) {
                sfx_pool[i].gain = (vol < 0 ? 0 : vol) * 256 / 127;
                nat_volume(&sfx_pool[i]);
            }
        }
    }
    return 0;
}

int sdMultiUnitDownload(void *handle)
{
    (void)handle;
    sfx_register(zgdfs_last_name());
    return 0;
}

static void nat_stop_movie(void) { nat_free(&nat_movie); }

static void nat_stop_all(void)
{
    int i;

    for (i = 0; i < SFX_POOL; i++) {
        nat_free(&sfx_pool[i]);
    }
    nat_free(&nat_music);
    nat_free(&nat_movie);
    nat_free(&nat_voice);
}

void zaudio_play_file(const char *name)
{
#ifdef ZAUDIO_OFF
    (void)name;
    return;
#else
    char mp3[48], lp[48];
    int size;
    void *h;

    zmix_stream_stop(0);
    nat_free(&nat_music);
    if (native_name(name, ".mp3", mp3, sizeof mp3)) {
        int has_lp = native_name(name, "_lp.mp3", lp, sizeof lp);

        if (nat_start_file(&nat_music, mp3, has_lp ? lp : 0, music_gain)) {
            zst.mus_nat++;
            return;
        }
    }
    /* sem o MP3: o ADX pelo mixer por software */
    zst.mus_mix++;
    h = open_disc_file(name, &size);
    if (h == 0) {
        return;
    }
    zfile_handle_close(h);
    {
        char buf[40];
        int n = 0, i2;

        while (name[n] != 0 && name[n] != ';' && n < 39) {
            buf[n] = name[n];
            n++;
        }
        buf[n] = 0;
        if (zfile_size(buf) < 0) {
            for (i2 = 0; i2 < n; i2++) {
                if (buf[i2] >= 'A' && buf[i2] <= 'Z') {
                    buf[i2] = (char)(buf[i2] + 32);
                }
            }
        }
        zmix_stream_file(0, buf, 0, size, music_gain);
    }
#endif
}

/* o stream.c do jogo chega aqui (game.mk renomeia as chamadas); o ADXT real
 * só existe com o jogo linkado */
extern void ADXT_StartFname(ADXT adxt, char *fname) __attribute__((weak));
extern void ADXT_StartAfs(ADXT adxt, Sint32 ptid, Sint32 flid) __attribute__((weak));
extern void ADXT_Stop(ADXT adxt) __attribute__((weak));
extern void ADXT_SetOutVol(ADXT adxt, Sint32 vol) __attribute__((weak));

/* o jogo cria o ADXT da música e depois o da voz, e dá a cada um o volume
 * antes de tocar: o primeiro handle visto é a música, o segundo a voz */
static ADXT h_music, h_voice;

static int slot_of(ADXT h)
{
    if (h_music == 0) {
        h_music = h;
    } else if (h_voice == 0 && h != h_music) {
        h_voice = h;
    }
    return h == h_voice && h != 0 ? 1 : 0;
}

void zlog(const char *what, const char *name, int value);
void zADXT_StartFname(ADXT adxt, char *fname)
{
    uint32 t0 = GETUPTIMEMS();
    uint32 t1;

    (void)slot_of(adxt);
    zaudio_play_file((const char *)fname);
    t1 = GETUPTIMEMS();
    if (ADXT_StartFname != 0) {
        ADXT_StartFname(adxt, fname);
    }
    zlog("adxt", "play_file ms", (int)(t1 - t0));
    zlog("adxt", "start real ms", (int)(GETUPTIMEMS() - t1));
}

/* a voz: o arquivo da partição é event_adx.afs; o AFS é 'AFS\0', n, e n
 * pares (offset, tamanho) little-endian */
void zADXT_StartAfs(ADXT adxt, Sint32 ptid, Sint32 flid)
{
#ifndef ZAUDIO_OFF
    (void)slot_of(adxt);
    zmix_stream_stop(1);
    zlog("tvoz", "voz pedida (flid)", (int)flid);
    zst.voz_req++;
    if (nat_start_voice((int)flid)) {
        zst.voz_nat++;
        zlog("tvoz", "voz nativa tocando (flid)", (int)flid);
    } else {
        zst.voz_mix++;
        zlog("tvoz", "voz sem .zva, mixer (flid)", (int)flid);
        /* sem event_adx.zva: o ADX da voz pelo mixer, a partir do AFS */
        unsigned char hd[8];
        unsigned char ent[8];
        int size;
        void *h = open_disc_file("event_adx.afs", &size);

        if (h != 0) {
            int n = 0;

            if (zfile_handle_read(h, hd, 8) == 8 && hd[0] == 'A' && hd[1] == 'F' && hd[2] == 'S') {
                n = (int)(hd[4] | (hd[5] << 8) | (hd[6] << 16) | ((unsigned)hd[7] << 24));
            }
            if (flid >= 0 && flid < n && zfile_handle_seek(h, 8 + flid * 8) == 0 &&
                zfile_handle_read(h, ent, 8) == 8) {
                int off = (int)(ent[0] | (ent[1] << 8) | (ent[2] << 16) | ((unsigned)ent[3] << 24));
                int len = (int)(ent[4] | (ent[5] << 8) | (ent[6] << 16) | ((unsigned)ent[7] << 24));

                zfile_handle_close(h);
                h = 0;
                zmix_stream_file(1, "event_adx.afs", off, len, voice_gain);
            }
            if (h != 0) {
                zfile_handle_close(h);
            }
        }
    }
#endif
    (void)ptid;
    if (ADXT_StartAfs != 0) {
        ADXT_StartAfs(adxt, ptid, flid);
    }
}

void zADXT_SetOutVol(ADXT adxt, Sint32 vol)
{
    int g = vol_to_gain((int)vol);

    if (slot_of(adxt) == 1) {
        voice_gain = g;
        nat_voice.gain = g;
        nat_volume(&nat_voice);
    } else {
        music_gain = g;
        nat_music.gain = g;
        nat_volume(&nat_music);
    }
    zmix_stream_gain(slot_of(adxt), g);
    if (ADXT_SetOutVol != 0) {
        ADXT_SetOutVol(adxt, vol);
    }
}

void zADXT_Stop(ADXT adxt)
{
    int slot = slot_of(adxt);

    zmix_stream_stop(slot);
    nat_free(slot == 1 ? &nat_voice : &nat_music);
    if (ADXT_Stop != 0) {
        ADXT_Stop(adxt);
    }
}

/* relatorio da janela de 2 s (chamado por vprof_report): so o que mudou desde o anterior */
extern void zlogd(const char *what, const char *name, int value);

void zaudio_report(void)
{
#ifndef ZAUDIO_OFF
    static int prev[24];
    static int first = 1;
    int busy = 0, i;
    int now[24];
    static const char *const names[24] = {
        "sfx tocados", "sfx sequencias", "sfx ausentes", "sfx roubados", "sfx emendas laco",
        "sfx erro abrir", "sfx erro ler", "sfx erro instancia", "sfx erro dados", "sfx erro tocar",
        "sfx clipe grande", "sfx ultimo tocado (tipo<<16|banco<<8|num)",
        "sfx ultimo ausente (tipo<<16|banco<<8|num)", "voz pedidas", "voz nativas", "voz mixer",
        "musica nativa", "musica mixer", "musica emendas laco", "filme nativo", "sfx pool ocupado (de 2)",
        "mixer ativo", "sfx conjuntos .zsf", "(reservado)"};

    for (i = 0; i < SFX_POOL; i++) {
        busy += sfx_pool[i].m != 0 && !sfx_pool[i].done;
    }
    now[0] = zst.sfx_play; now[1] = zst.sfx_seq; now[2] = zst.sfx_miss; now[3] = zst.sfx_steal;
    now[4] = zst.sfx_loopsw; now[5] = zst.sfx_e_open; now[6] = zst.sfx_e_read;
    now[7] = zst.sfx_e_inst; now[8] = zst.sfx_e_data; now[9] = zst.sfx_e_play;
    now[10] = zst.sfx_e_big; now[11] = zst.sfx_last_play; now[12] = zst.sfx_last_miss;
    now[13] = zst.voz_req; now[14] = zst.voz_nat; now[15] = zst.voz_mix;
    now[16] = zst.mus_nat; now[17] = zst.mus_mix; now[18] = zst.mus_loopsw;
    now[19] = zst.filme_nat; now[20] = busy; now[21] = zmix_active();
    now[22] = 0;
    for (i = 0; i < SFX_SETS; i++) {
        now[22] += sfx_sets[i].idx != 0;
    }
    now[23] = 0;
    for (i = 0; i < 23; i++) {
        if (first ? now[i] != 0 : now[i] != prev[i]) {
            zlogd("tau", names[i], now[i]);
        }
        prev[i] = now[i];
    }
    first = 0;
#endif
}

/* ADXF_LoadPartition: o CRI le a tabela de arquivos do AFS das vozes (event_adx.afs, 72 MB, 2148
 * arquivos) para o ADXT_StartAfs achar o offset. O zADXT_StartAfs (acima) le o AFS por conta
 * propria e ignora a particao, entao carregar a tabela so custava ~20 s de CPU emulada a cada
 * campo/evento (tperf: um quadro de 20 s logo apos abrir event_adx.afs). Retorna 0 (ok). */
Sint32 ADXF_LoadPartition(Sint32 ptid, char *fname, void *ptinfo, Sint32 nfile)
{
    (void)ptid;
    (void)ptinfo;
    zlog("tvoz", "ADXF_LoadPartition ignorado (arquivos)", nfile);
    (void)fname;
    return 0;
}
