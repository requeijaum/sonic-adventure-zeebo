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
static IShell *audio_shell;
static unsigned char *music_wav;

void zaudio_set_shell(IShell *shell)
{
    audio_shell = shell;
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

/* toca um ADX em memória (adx, len) pelo IMedia, até seconds segundos */
void zlog(const char *what, const char *name, int value);
void zaudio_play_adx(const unsigned char *adx, int len, int seconds)
{
#ifdef ZAUDIO_OFF
    (void)adx; (void)len; (void)seconds;
    return;
#endif
    Sint32 rate = 44100;
    int most;
    int frames;
    unsigned int data_bytes;
    AEEMediaData md;

    zaudio_stop();
    if (music_wav != 0) {
        FREE(music_wav);
        music_wav = 0;
    }
    if (adx == 0 || len <= 0 || audio_shell == 0) {
        return;
    }
    most = seconds * 44100;
    music_wav = (unsigned char *)MALLOC((uint32)(44 + most * 4));
    if (music_wav == 0) {
        zlog("audio", "MALLOC falhou bytes", 44 + most * 4);
        return;
    }
    frames = decode_adx(adx, len, (short *)(music_wav + 44), most, &rate);
    zlog("audio", "quadros decodificados", frames);
    if (frames <= 0) {
        return;
    }
    data_bytes = (unsigned int)frames * 4;
    memcpy(music_wav, "RIFF", 4);
    put32(music_wav + 4, 36 + data_bytes);
    memcpy(music_wav + 8, "WAVEfmt ", 8);
    put32(music_wav + 16, 16);
    put16(music_wav + 20, 1);
    put16(music_wav + 22, 2);
    put32(music_wav + 24, (unsigned int)rate);
    put32(music_wav + 28, (unsigned int)rate * 4);
    put16(music_wav + 32, 4);
    put16(music_wav + 34, 16);
    memcpy(music_wav + 36, "data", 4);
    put32(music_wav + 40, data_bytes);
    if (ISHELL_CreateInstance(audio_shell, AEECLSID_MEDIA, (void **)&media) !=
            AEE_SUCCESS ||
        media == 0) {
        media = 0;
        return;
    }
    md.clsData = MMD_BUFFER;
    md.pData = music_wav;
    md.dwSize = 44 + data_bytes;
    if (IMEDIA_SetMediaData(media, &md) != AEE_SUCCESS ||
        IMEDIA_Play(media) != AEE_SUCCESS) {
        IMEDIA_Release(media);
        media = 0;
    }
}

void zaudio_play_file(const char *name)
{
#ifdef ZAUDIO_OFF
    (void)name;
    return;
#endif
    int size = zfile_size(name);
    unsigned char *adx;

    zaudio_stop();
    if (size <= 0 || audio_shell == 0) {
        return;
    }
    adx = (unsigned char *)MALLOC((uint32)size);
    if (adx == 0) {
        return;
    }
    if (zgdfs_read_file(name, adx, size) == size) {
        zaudio_play_adx(adx, size, MUSIC_SECONDS);
    }
    FREE(adx);
}

/* o stream.c do jogo chega aqui (game.mk renomeia as chamadas); o ADXT real
 * só existe com o jogo linkado */
extern void ADXT_StartFname(ADXT adxt, char *fname) __attribute__((weak));
extern void ADXT_Stop(ADXT adxt) __attribute__((weak));

void zlog(const char *what, const char *name, int value);
void zADXT_StartFname(ADXT adxt, char *fname)
{
    uint32 t0 = GETUPTIMEMS();
    uint32 t1;

    zaudio_play_file((const char *)fname);
    t1 = GETUPTIMEMS();
    if (ADXT_StartFname != 0) {
        ADXT_StartFname(adxt, fname);
    }
    zlog("adxt", "play_file ms", (int)(t1 - t0));
    zlog("adxt", "start real ms", (int)(GETUPTIMEMS() - t1));
}

void zADXT_Stop(ADXT adxt)
{
    zaudio_stop();
    if (ADXT_Stop != 0) {
        ADXT_Stop(adxt);
    }
}
