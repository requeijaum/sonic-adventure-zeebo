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
