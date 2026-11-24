/* zmix.c — mixer de som por software, a saída do jogo no Zeebo.
 *
 * Um único IMedia (AEECLSID_MEDIAPCM) toca um fluxo PCM 16 bits estéreo; a
 * "fonte" desse fluxo é um ISource nosso, cujo Read mixa o que estiver ativo
 * naquele instante: dois streams ADX (música, voz/filme), lidos aos blocos do
 * arquivo e decodificados na hora, e vozes de efeito em PCM. Não há buffer do
 * trecho inteiro na memória (a música tinha até 40 s, sem laço), os laços do
 * cabeçalho ADX valem, e não há o limite de 4 objetos de áudio do DSP.
 *
 * Tudo em inteiro: o ARM11 do Zeebo não tem VFP. */

#include <AEEStdLib.h>
#include <AEEShell.h>
#include <AEEClassIDs.h>
#include <AEEMedia.h>

#include "zstrip.h"
#include "zmix.h"

void zlog(const char *what, const char *name, int value);

#define MIX_RATE 44100
#define BUFFER_BYTES 8192 /* o que o DSP/zeebx pede por Read (2048 quadros, ~46 ms) */
#define CHUNK 256         /* quadros mixados por vez */

/* --- ADX ---------------------------------------------------------------- */

#define FBUF 2304 /* 64 blocos estéreo de 18 bytes por leitura de arquivo */

typedef struct {
    int active;
    /* fonte: memória (própria) ou arquivo a partir de base */
    unsigned char *mem;
    int memlen;
    void *fh;
    int base;
    int len;
    int rd;         /* próximo byte, relativo a base */
    unsigned char fbuf[FBUF];
    int fbuf_pos, fbuf_n;
    int off, nch, bs, rate;
    unsigned total, pos; /* quadros */
    int c1, c2;
    int h1[2], h2[2];
    int loop;
    unsigned ls, le;
    int lblk;      /* bloco onde o laço recomeça */
    int lskip;     /* quadros a descartar depois de saltar */
    int lh1[2], lh2[2];
    int lcap;      /* histórico do início do laço já guardado */
    int blk_no;
    short blk[64]; /* um bloco decodificado: 32 quadros, 2 canais */
    int blk_n, blk_i;
    int skip;
    unsigned step; /* 16.16: rate / MIX_RATE */
    unsigned frac;
    short p0[2], p1[2];
    int primed;
    int gain; /* 0..256 */
} ZStream;

static ZStream st[ZMIX_STREAMS];

static int be16(const unsigned char *p) { return (p[0] << 8) | p[1]; }
static unsigned be32(const unsigned char *p)
{
    return ((unsigned)p[0] << 24) | ((unsigned)p[1] << 16) | ((unsigned)p[2] << 8) | p[3];
}

double sqrt(double x);
double cos(double x);

/* lê n bytes da fonte na posição rd; devolve quantos conseguiu */
static int src_read(ZStream *s, unsigned char *dst, int n)
{
    int got = 0;

    if (s->mem != 0) {
        got = s->memlen - s->rd;
        if (got > n) {
            got = n;
        }
        if (got < 0) {
            got = 0;
        }
        for (int i = 0; i < got; i++) {
            dst[i] = s->mem[s->rd + i];
        }
        s->rd += got;
        return got;
    }
    while (got < n) {
        int k;

        if (s->fbuf_pos >= s->fbuf_n) {
            int want = FBUF;

            if (s->rd + want > s->len) {
                want = s->len - s->rd;
            }
            if (want <= 0) {
                break;
            }
            s->fbuf_n = zfile_handle_read(s->fh, s->fbuf, want);
            s->fbuf_pos = 0;
            if (s->fbuf_n <= 0) {
                s->fbuf_n = 0;
                break;
            }
        }
        k = s->fbuf_n - s->fbuf_pos;
        if (k > n - got) {
            k = n - got;
        }
        for (int i = 0; i < k; i++) {
            dst[got + i] = s->fbuf[s->fbuf_pos + i];
        }
        s->fbuf_pos += k;
        s->rd += k;
        got += k;
    }
    return got;
}

static int src_seek(ZStream *s, int rel)
{
    s->rd = rel;
    s->fbuf_pos = s->fbuf_n = 0;
    if (s->mem != 0) {
        return rel <= s->memlen ? 0 : -1;
    }
    return zfile_handle_seek(s->fh, s->base + rel);
}

static void stream_free(ZStream *s)
{
    if (s->fh != 0) {
        zfile_handle_close(s->fh);
        s->fh = 0;
    }
    if (s->mem != 0) {
        FREE(s->mem);
        s->mem = 0;
    }
    s->active = 0;
}

/* abre o cabeçalho; 0 se for um ADX que sabemos tocar */
static int adx_header(ZStream *s)
{
    unsigned char h[64];
    unsigned char hd[8];
    double z, A, B, C;
    const double pi = 3.14159265358979323846;
    const double r2 = 1.41421356237309504880;
    int got = src_read(s, h, 64);

    (void)hd;
    if (got < 32 || h[0] != 0x80 || h[4] != 3 || h[5] != 18 || h[6] != 4 ||
        (h[7] != 1 && h[7] != 2)) {
        return -1;
    }
    s->off = be16(h + 2) + 4;
    s->bs = h[5];
    s->nch = h[7];
    s->rate = (int)be32(h + 8);
    s->total = be32(h + 12);
    if (s->rate < 4000 || s->rate > 96000) {
        return -1;
    }
    z = cos(2.0 * pi * be16(h + 16) / (double)s->rate);
    A = r2 - z;
    B = r2 - 1.0;
    C = (A - sqrt((A + B) * (A - B))) / B;
    {
        double v = C * 8192.0;
        s->c1 = v < 0 ? -(int)(-v + 0.5) : (int)(v + 0.5);
        v = C * C * -4096.0;
        s->c2 = v < 0 ? -(int)(-v + 0.5) : (int)(v + 0.5);
    }
    s->loop = 0;
    if (got >= 0x2c && h[18] == 3 && be32(h + 0x18) != 0) {
        unsigned ls = be32(h + 0x1c);
        unsigned lb = be32(h + 0x20);
        unsigned le = be32(h + 0x24);

        if (lb >= (unsigned)s->off && le > ls && le <= s->total) {
            int per = s->bs * s->nch;

            s->loop = 1;
            s->ls = ls;
            s->le = le;
            s->lblk = (int)((lb - (unsigned)s->off) / (unsigned)per);
            s->lskip = (int)(ls - (unsigned)s->lblk * 32u);
            if (s->lskip < 0 || s->lskip >= 32) {
                s->lskip = 0;
            }
        }
    }
    s->step = (unsigned)(((unsigned long long)s->rate << 16) / MIX_RATE);
    src_seek(s, s->off);
    return 0;
}

/* decodifica o próximo bloco em s->blk; 0 no fim */
static int adx_block(ZStream *s)
{
    unsigned char raw[36];
    int per = s->bs * s->nch;
    int ch, i, m;

    if (s->loop && s->pos >= s->le) {
        /* fim do laço: volta ao começo dele com o histórico guardado lá */
        if (!s->lcap || src_seek(s, s->off + s->lblk * per) != 0) {
            return 0;
        }
        for (ch = 0; ch < 2; ch++) {
            s->h1[ch] = s->lh1[ch];
            s->h2[ch] = s->lh2[ch];
        }
        s->pos = (unsigned)s->lblk * 32u;
        s->skip = s->lskip;
        s->blk_no = s->lblk;
    }
    if (s->pos >= s->total) {
        return 0;
    }
    if (s->loop && !s->lcap && s->blk_no == s->lblk) {
        for (ch = 0; ch < 2; ch++) {
            s->lh1[ch] = s->h1[ch];
            s->lh2[ch] = s->h2[ch];
        }
        s->lcap = 1;
    }
    if (src_read(s, raw, per) != per) {
        return 0;
    }
    m = 32;
    if (s->pos + 32u > s->total) {
        m = (int)(s->total - s->pos);
    }
    if (s->loop && s->pos + (unsigned)m > s->le && s->pos < s->le) {
        m = (int)(s->le - s->pos);
    }
    for (ch = 0; ch < s->nch; ch++) {
        const unsigned char *f = raw + ch * s->bs;
        int head = be16(f);
        int scale = head & 0x1fff;
        int s1 = s->h1[ch], s2 = s->h2[ch];

        if (head & 0x8000) {
            return 0; /* bloco de fim */
        }
        for (i = 0; i < 32; i++) {
            int b = f[2 + (i >> 1)];
            int d = (i & 1) ? (b & 15) : (b >> 4);
            int v;

            if (d & 8) {
                d -= 16;
            }
            v = d * scale + ((s->c1 * s1 + s->c2 * s2) >> 12);
            if (v > 32767) {
                v = 32767;
            }
            if (v < -32768) {
                v = -32768;
            }
            s->blk[i * 2 + ch] = (short)v;
            if (s->nch == 1) {
                s->blk[i * 2 + 1] = (short)v;
            }
            s2 = s1;
            s1 = v;
        }
        s->h1[ch] = s1;
        s->h2[ch] = s2;
    }
    s->blk_n = m;
    s->blk_i = 0;
    if (s->skip > 0) {
        int k = s->skip > m ? m : s->skip;

        s->blk_i = k;
        s->skip -= k;
    }
    s->pos += (unsigned)m;
    s->blk_no++;
    return 1;
}

/* próximo quadro (L,R) do stream; 0 no fim */
static int stream_next(ZStream *s, short *l, short *r)
{
    while (s->blk_i >= s->blk_n) {
        if (!adx_block(s)) {
            return 0;
        }
    }
    *l = s->blk[s->blk_i * 2];
    *r = s->blk[s->blk_i * 2 + 1];
    s->blk_i++;
    return 1;
}

/* soma frames quadros do stream em acc (L,R intercalados), reamostrando */
static void stream_mix(ZStream *s, int *acc, int frames)
{
    int i;

    if (!s->primed) {
        short l, r;

        if (!stream_next(s, &l, &r)) {
            stream_free(s);
            return;
        }
        s->p1[0] = l;
        s->p1[1] = r;
        s->p0[0] = l;
        s->p0[1] = r;
        s->frac = 0;
        s->primed = 1;
    }
    for (i = 0; i < frames; i++) {
        int f = (int)(s->frac >> 8); /* 0..255 */
        int l = s->p0[0] + (((s->p1[0] - s->p0[0]) * f) >> 8);
        int r = s->p0[1] + (((s->p1[1] - s->p0[1]) * f) >> 8);

        acc[i * 2] += (l * s->gain) >> 8;
        acc[i * 2 + 1] += (r * s->gain) >> 8;
        s->frac += s->step;
        while (s->frac >= 0x10000u) {
            short nl, nr;

            s->frac -= 0x10000u;
            s->p0[0] = s->p1[0];
            s->p0[1] = s->p1[1];
            if (!stream_next(s, &nl, &nr)) {
                stream_free(s);
                return;
            }
            s->p1[0] = nl;
            s->p1[1] = nr;
        }
    }
}

/* --- vozes de efeito (PCM 16 bits mono) ----------------------------------- */

typedef struct {
    int active;
    const short *pcm;
    unsigned n;
    unsigned pos; /* 16.16 */
    unsigned step;
    int loop_s, loop_e; /* em amostras; loop_e == 0: sem laço */
    int gl, gr;         /* 0..256 */
    int id;
    int prio;
} ZVoice;

static ZVoice vo[ZMIX_VOICES];

static void voice_mix(ZVoice *v, int *acc, int frames)
{
    int i;

    for (i = 0; i < frames; i++) {
        unsigned ix = v->pos >> 16;
        int a, b, s;

        if (v->loop_e > 0 && ix >= (unsigned)v->loop_e) {
            v->pos -= (unsigned)(v->loop_e - v->loop_s) << 16;
            ix = v->pos >> 16;
        }
        if (ix >= v->n) {
            v->active = 0;
            return;
        }
        a = v->pcm[ix];
        b = ix + 1 < v->n ? v->pcm[ix + 1] : a;
        s = a + (((b - a) * (int)((v->pos >> 8) & 255)) >> 8);
        acc[i * 2] += (s * v->gl) >> 8;
        acc[i * 2 + 1] += (s * v->gr) >> 8;
        v->pos += v->step;
    }
}

/* --- ISource + IMedia ------------------------------------------------------ */

typedef struct ZSrc ZSrc;
typedef struct {
    uint32 (*AddRef)(ZSrc *);
    uint32 (*Release)(ZSrc *);
    int (*QueryInterface)(ZSrc *, AEECLSID, void **);
    int32 (*Read)(ZSrc *, char *, int32);
    void (*Readable)(ZSrc *, AEECallback *);
} ZSrcVtbl;
struct ZSrc {
    ZSrcVtbl *pvt;
    uint32 refs;
};

static ZSrcVtbl src_vtbl;
static ZSrc src_obj;

typedef struct {
    uint32 clsData;
    void *pData;
    uint32 dwSize;
    uint32 dwStructSize;
    uint32 dwCaps;
    uint32 bRaw;
    void *pSpec;
    uint32 dwSpecSize;
    uint32 dwBufferSize;
} ZMediaDataEx;

typedef struct {
    uint16 wSize;
    uint16 pad0;
    uint32 clsMedia;
    uint16 wChannels;
    uint16 pad1;
    uint32 dwSamplesPerSec;
    uint16 wBitsPerSample;
    uint16 flags; /* bit 0: unsigned */
    uint32 dwBytesPerSec;
    uint16 wBlockSize;
    uint16 pad2;
    uint32 dwExtra;
} ZWaveSpec;

#define ZMMD_ISOURCE 0x01001012u

static IMedia *media;
static ZMediaDataEx mde;
static ZWaveSpec spec;
static int started;
static IShell *mix_shell;
static int master = 256;
static int acc[CHUNK * 2];
static unsigned reads_total;

static uint32 src_addref(ZSrc *p) { return ++p->refs; }
static uint32 src_release(ZSrc *p) { return p->refs > 1 ? --p->refs : p->refs; }
static int src_qi(ZSrc *p, AEECLSID c, void **pp)
{
    (void)p;
    (void)c;
    *pp = 0;
    return 1; /* EFAILED: só o Read é usado */
}
static void src_readable(ZSrc *p, AEECallback *cb)
{
    (void)p;
    (void)cb;
}

static int32 src_read_fn(ZSrc *p, char *dst, int32 cb)
{
    int frames = cb / 4;
    int done = 0;
    short *out = (short *)dst;

    (void)p;
    reads_total++;
    while (done < frames) {
        int n = frames - done;
        int i, k;

        if (n > CHUNK) {
            n = CHUNK;
        }
        for (i = 0; i < n * 2; i++) {
            acc[i] = 0;
        }
        for (k = 0; k < ZMIX_STREAMS; k++) {
            if (st[k].active) {
                stream_mix(&st[k], acc, n);
            }
        }
        for (k = 0; k < ZMIX_VOICES; k++) {
            if (vo[k].active) {
                voice_mix(&vo[k], acc, n);
            }
        }
        for (i = 0; i < n * 2; i++) {
            int v = (acc[i] * master) >> 8;

            if (v > 32767) {
                v = 32767;
            }
            if (v < -32768) {
                v = -32768;
            }
            out[done * 2 + i] = (short)v;
        }
        done += n;
    }
    return frames * 4;
}

int zmix_start(IShell *shell)
{
    int k;

    if (started) {
        return 1;
    }
    if (shell == 0) {
        return 0;
    }
    src_vtbl.AddRef = src_addref;
    src_vtbl.Release = src_release;
    src_vtbl.QueryInterface = src_qi;
    src_vtbl.Read = src_read_fn;
    src_vtbl.Readable = src_readable;
    src_obj.pvt = &src_vtbl;
    src_obj.refs = 1;
    for (k = 0; k < ZMIX_STREAMS; k++) {
        st[k].active = 0;
    }
    for (k = 0; k < ZMIX_VOICES; k++) {
        vo[k].active = 0;
    }
    spec.wSize = (uint16)sizeof spec;
    spec.wChannels = 2;
    spec.dwSamplesPerSec = MIX_RATE;
    spec.wBitsPerSample = 16;
    spec.flags = 0;
    spec.dwBytesPerSec = MIX_RATE * 4;
    spec.wBlockSize = 4;
    mde.clsData = ZMMD_ISOURCE;
    mde.pData = &src_obj;
    mde.dwSize = 0;
    mde.dwStructSize = sizeof mde;
    mde.dwCaps = 0;
    mde.bRaw = 1;
    mde.pSpec = &spec;
    mde.dwSpecSize = sizeof spec;
    mde.dwBufferSize = BUFFER_BYTES;
    mix_shell = shell;
    started = 1;
    return 1;
}

/* o fluxo PCM só sobe na primeira vez que há o que tocar: um fluxo mudo
 * correndo desde o início do app custa CPU à toa, e o zeebx só liga o gravador
 * de áudio depois dos primeiros quadros */
static int ensure_media(void)
{
    if (media != 0) {
        return 1;
    }
    if (mix_shell == 0) {
        return 0;
    }
    if (ISHELL_CreateInstance(mix_shell, 0x01005511u /* AEECLSID_MEDIAPCM */,
                              (void **)&media) != AEE_SUCCESS ||
        media == 0) {
        media = 0;
        zlog("audio", "mixer: sem IMediaPCM", 0);
        return 0;
    }
    if (IMEDIA_SetMediaData(media, (AEEMediaData *)(void *)&mde) != AEE_SUCCESS ||
        IMEDIA_Play(media) != AEE_SUCCESS) {
        zlog("audio", "mixer: Play falhou", 0);
        IMEDIA_Release(media);
        media = 0;
        return 0;
    }
    zlog("audio", "mixer: fluxo PCM no ar (Hz)", MIX_RATE);
    return 1;
}

void zmix_stop_all(void)
{
    int k;

    for (k = 0; k < ZMIX_STREAMS; k++) {
        if (st[k].active) {
            stream_free(&st[k]);
        }
    }
    for (k = 0; k < ZMIX_VOICES; k++) {
        vo[k].active = 0;
    }
}

void zmix_shutdown(void)
{
    zmix_stop_all();
    if (media != 0) {
        IMEDIA_Stop(media);
        IMEDIA_Release(media);
        media = 0;
    }
    started = 0;
    mix_shell = 0;
}

int zmix_active(void) { return started; }

static void stream_reset(ZStream *s)
{
    s->active = 0;
    s->mem = 0;
    s->fh = 0;
    s->rd = 0;
    s->fbuf_pos = s->fbuf_n = 0;
    s->pos = 0;
    s->blk_n = s->blk_i = 0;
    s->blk_no = 0;
    s->skip = 0;
    s->lcap = 0;
    s->primed = 0;
    s->frac = 0;
    s->h1[0] = s->h1[1] = s->h2[0] = s->h2[1] = 0;
}

static int stream_begin(int slot)
{
    ZStream *s = &st[slot];

    if (adx_header(s) != 0) {
        stream_free(s);
        zlog("audio", "ADX nao suportado", slot);
        return 0;
    }
    s->gain = s->gain > 0 ? s->gain : 256;
    s->active = 1;
    return 1;
}

int zmix_stream_file(int slot, const char *name, int base, int len, int gain)
{
    ZStream *s;

    if (slot < 0 || slot >= ZMIX_STREAMS || !started) {
        return 0;
    }
    if (!ensure_media()) {
        return 0;
    }
    s = &st[slot];
    if (s->active) {
        stream_free(s);
    }
    stream_reset(s);
    s->gain = gain;
    s->fh = zfile_handle_open(name);
    if (s->fh == 0) {
        return 0;
    }
    s->base = base;
    s->len = len;
    if (zfile_handle_seek(s->fh, base) != 0) {
        stream_free(s);
        return 0;
    }
    return stream_begin(slot);
}

int zmix_stream_mem(int slot, const unsigned char *adx, int len, int gain)
{
    ZStream *s;
    unsigned char *copy;

    if (slot < 0 || slot >= ZMIX_STREAMS || !started || adx == 0 || len <= 0) {
        return 0;
    }
    if (!ensure_media()) {
        return 0;
    }
    s = &st[slot];
    if (s->active) {
        stream_free(s);
    }
    copy = (unsigned char *)MALLOC((uint32)len);
    if (copy == 0) {
        return 0;
    }
    for (int i = 0; i < len; i++) {
        copy[i] = adx[i];
    }
    stream_reset(s);
    s->gain = gain;
    s->mem = copy;
    s->memlen = len;
    return stream_begin(slot);
}

void zmix_stream_stop(int slot)
{
    if (slot >= 0 && slot < ZMIX_STREAMS && st[slot].active) {
        stream_free(&st[slot]);
    }
}

void zmix_stream_gain(int slot, int gain)
{
    if (slot >= 0 && slot < ZMIX_STREAMS) {
        st[slot].gain = gain;
    }
}

int zmix_stream_playing(int slot)
{
    return slot >= 0 && slot < ZMIX_STREAMS && st[slot].active;
}

/* vozes de efeito: pcm fica com o chamador e precisa durar até a voz acabar */
int zmix_voice(int id, int prio, const short *pcm, unsigned n, int rate, int loop_s,
               int loop_e, int gl, int gr)
{
    int k, pick = -1, lowest = 0x7fffffff;

    if (!started || pcm == 0 || n == 0 || !ensure_media()) {
        return -1;
    }
    for (k = 0; k < ZMIX_VOICES; k++) {
        if (!vo[k].active) {
            pick = k;
            break;
        }
        if (vo[k].prio < lowest) {
            lowest = vo[k].prio;
            pick = k;
        }
    }
    if (pick < 0 || (vo[pick].active && vo[pick].prio > prio)) {
        return -1;
    }
    vo[pick].pcm = pcm;
    vo[pick].n = n;
    vo[pick].pos = 0;
    vo[pick].step = (unsigned)(((unsigned long long)rate << 16) / MIX_RATE);
    vo[pick].loop_s = loop_s;
    vo[pick].loop_e = loop_e;
    vo[pick].gl = gl;
    vo[pick].gr = gr;
    vo[pick].id = id;
    vo[pick].prio = prio;
    vo[pick].active = 1;
    return pick;
}

void zmix_voice_stop(int id)
{
    for (int k = 0; k < ZMIX_VOICES; k++) {
        if (vo[k].active && vo[k].id == id) {
            vo[k].active = 0;
        }
    }
}

void zmix_master(int gain) { master = gain; }

unsigned zmix_reads(void) { return reads_total; }
