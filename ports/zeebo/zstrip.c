/* zstrip.c — acumulador de strip e sua submissão GLES1 (caminho fixo).
 *
 * O Zeebo é ARM11 sem FPU e o caminho provado no zeebx/IGLES11 é o de
 * arranjos *tight* (stride 0) em GL_FIXED s15.16, com glOrthox/glClearDepthx.
 * Os vértices chegam em floats (convenção do SA1) e são convertidos aqui.
 *
 * Sem libc: arrays estáticos. */

#define GL_OES_VERSION_1_1
#include <AEEStdLib.h>
#include "gles/gl.h"

#include "assets/prs.h"
#include "assets/pvm.h"
#include "assets/pvr.h"
#include "zstrip.h"
#include "zblend.h"

extern const unsigned char zasset_pvr[];
extern const unsigned int zasset_pvr_len;
/* o PRS vem do armazenamento do app (IFileMgr), não do .mod */
#define PRS_FILE "FROG.PRS"
static unsigned char zasset_prs[64 * 1024];
static unsigned int zasset_prs_len;

#ifndef Z_TJOIN
/* Unir dois strips num DrawArrays gera o triangulo de ligacao entre o ultimo vertice de um
 * e os dois primeiros do outro (o caminho opaco paga isso com vertices degenerados, ver
 * opaque_add). Sem inserir degenerados a uniao desenha geometria espuria: medido, 10.659
 * pixels diferentes no mesmo quadro. Fica desligada ate haver insercao (copia) no pool */
#define Z_TJOIN 0
#endif
#define ZSTRIP_MOST 4096
#define FIX(v) ((GLfixed)((v) * 65536.0f))
/* Prioridades 2D do Ninja chegam como z=9..15 no menu de pause. O ortho -1..1
 * recortava o painel inteiro; em coordenadas de olho, near=-16 aceita z ate 16
 * e mantem prioridade maior mais perto sob GL_LEQUAL. */
#define SCREEN_NEAR (-(16 << 16))
#define SCREEN_FAR (1 << 16)

/* arranjos separados e contíguos: stride 0 */
static GLfixed vxyz[ZSTRIP_MOST * 3];
static GLfixed vuv[ZSTRIP_MOST * 2];
static GLubyte vrgba[ZSTRIP_MOST * 4];
static int vert_count;

void zlog(const char *what, const char *name, int value);
void zlogd(const char *what, const char *name, int value);

static GLuint texture; /* definida na cena; usada pelo estado do strip */

/* tabela global de texturas: o índice é o globalIndex do Ninja */
#define ZTEX_MOST 2048 /* Station Square sozinha passa de 512 vivas (o PC segura 918) */
static GLuint tex_names[ZTEX_MOST];
static unsigned char tex_opq[ZTEX_MOST]; /* 1: todo texel com alfa 255; 2: so 0 e 255 (recorte) */
static int tex_count;
static GLuint tex_current;
static int movie_index = -1; /* textura dos filmes (zstrip_movie_upload) */

/* estado do strip aberto (para adiar os translúcidos) */
static int st_blend;
static int st_textured;
static GLuint st_tex;
static float vz[ZSTRIP_MOST]; /* z em float, para a ordem da lista */
/* fatores de blend do strip aberto (GL); o padrão é o do PVR para alfa */
static GLenum st_src = GL_SRC_ALPHA, st_dst = GL_ONE_MINUS_SRC_ALPHA;

/* lista translúcida do quadro: o PVR a desenha de trás para a frente (o
 * maior 1/z é o mais perto). Aqui os strips translúcidos ficam guardados até
 * zstrip_flush, que os ordena pela profundidade média (estável: empates
 * mantêm a ordem de envio) e desenha. */
#define TLIST_VERTS 8192
#define TLIST_STRIPS 1024
static GLfixed t_xyz[TLIST_VERTS * 3];
static GLfixed t_uv[TLIST_VERTS * 2];
static GLubyte t_rgba[TLIST_VERTS * 4];
static int t_verts;
static struct tstrip {
    int first, count;
    int textured;
    GLuint tex;
    GLenum src, dst;
    float key; /* z médio, em float: em 16.16 os z pequenos (1/w de
                * prioridades 2D grandes) viram 0 e empatam */
    int model; /* 0: strip em tela; n > 0: lote de modelo tm_b[n - 1] (via GL, adiado) */
} t_strips[TLIST_STRIPS];
static int t_count;
static int t_order[TLIST_STRIPS];

/* opacos em lote: sem ordem (o z-buffer decide), então os strips de um mesmo
 * estado viram uma chamada só, unidos por triângulos degenerados. O lote sai
 * antes de qualquer troca de estado (textura, mistura) e em zstrip_flush */
#define OPQ_MOST 16384
static GLfixed o_xyz[OPQ_MOST * 3];
static GLfixed o_uv[OPQ_MOST * 2];
static GLubyte o_rgba[OPQ_MOST * 4];
static int o_count;

/* --- lote de modelos: vértices em espaço de objeto, o GL transforma ---
 * Os pontos do modelo já estão em GL_FIXED (cache do zlive.c); aqui só se
 * juntam os índices em strips (cópias de inteiros, sem float). A matriz de
 * modelo é a do Ninja; a projeção põe z_clip = -1 e w = z da vista, o mesmo
 * -1/z do 2D (o HUD, em z = 1, segue na frente) */
#define MDL_MOST 16384
static GLfixed m_xyz[MDL_MOST * 3];
static GLshort m_uv[MDL_MOST * 2];
static GLubyte m_rgba[MDL_MOST * 4];
static int m_count;
static GLfixed m_proj[16], m_model[16];
static int m_mode; /* 1 enquanto o GL está em modo de modelo */
/* O preview da seleção é 3D entre camadas 2D. Só esse draw pede que os
 * quads respeitem o Z já escrito pelo modelo; HUD e pause continuam por ordem
 * de pintura, sem competir com a profundidade da cena. */
static int screen_depth_test;

void zstrip_screen_depth(int on)
{
    screen_depth_test = on != 0;
}

static void screen_depth_apply(void)
{
    if (screen_depth_test) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

#ifndef Z_STATECACHE
#define Z_STATECACHE 1 /* 0 = emite todas as chamadas GL como antes (A/B) */
#endif

/* hash rolante do que o jogo mandou desenhar (contagem, 1o vertice e textura de cada
 * strip): gate de otimizacao, dois builds com a mesma entrada dao o mesmo valor
 * (app.c o registra a cada 64 quadros como "estado hash") */
unsigned zs_hash = 2166136261u;
static void hash_mix(unsigned v)
{
    zs_hash = (zs_hash ^ v) * 16777619u;
}

/* Espelho do estado GL que o port muda a cada strip (-1 = desconhecido): só emite a
 * chamada quando o valor muda. Zerado no começo de cada quadro (gl_shadow_reset),
 * porque o cubo de teste e a inicialização mexem no GL direto. */
static int sh_blend = -1, sh_dmask = -1, sh_tex2d = -1, sh_mm = -1;
static unsigned sh_bound = 0xffffffffu; /* textura ligada (0xffffffff = desconhecida) */
static int sh_proj_ok; /* 1 = a projeção do modelo (l_proj) está carregada no GL */
static GLfixed l_proj[16];
static int sh_model_ok; /* 1 = a matriz do modelo (l_model) está carregada no GL */
static GLfixed l_model[16];
static const void *sh_av, *sh_at, *sh_ac;
static int sh_att;
unsigned zs_draws, zs_redundant, zs_tex16;
/* diagnostico de agrupamento: binds de textura por janela e texturas distintas */
unsigned zs_texbinds, zs_texdistinct, zs_truns, zs_tverts, zs_tjoined;
#define TS_SEEN 512 /* potencia de 2: mascara do hash */
static GLuint ts_seen[TS_SEEN];
unsigned zs_tstrips, zs_tshift; /* passe translucido: strips ordenados e deslocamentos da insercao */

/* --- nevoa: kmSetFogDensity/Table/TableColor e njFogEnable/Disable (zkamui.c) ---
 * O PVR da nevoa por tabela de 128 entradas indexada pela distancia: a densidade guarda a
 * distancia longe F (mantissa/8 bits x 2^expoente) e a entrada i vale a distancia
 * F * 16 / (2^(i/16) * (16 + i%16)), de F (i = 0, toda nevoa) ate perto (i = 127, nenhuma).
 * O GLES1 tem nevoa linear: o inicio e o fim saem das entradas onde a tabela passa de 2% e de
 * 98%. Distancia do olho em unidades do Ninja, como o GL (|z|); a cor e a do jogo. So os
 * modelos da via GL (zlive) levam nevoa; o resto (2D, translucidos projetados) nao */
#ifndef Z_FOG
#define Z_FOG 1
#endif
static unsigned fog_far;
static int fog_tab[128]; /* Q16: 1.0 = toda a nevoa */
static GLfixed fog_start, fog_end, fog_col[4] = {65535, 65535, 65535, 65536};
static int fog_on = 1, fog_have, fog_dirty, fog_sent, sh_fog = -1;

static int fog_q16(unsigned b)
{
    int e = (int)((b >> 23) & 255) - 127;
    unsigned m = (b & 0x7fffff) | 0x800000;

    if ((b >> 31) != 0 || e < -16) {
        return 0;
    }
    if (e >= 0) {
        return 65536;
    }
    return (int)(m >> (7 - e));
}

void zfog_density(unsigned d)
{
    unsigned e = d & 0xff;

    if (e > 22) {
        e = 22;
    }
    fog_far = (((d >> 8) & 0xff) << e) >> 7;
    if (fog_far > (1u << 27)) {
        fog_far = 1u << 27;
    }
    zlog("tfog", "distancia longe F (densidade)", (int)fog_far);
    fog_dirty = 1;
}

void zfog_table(const unsigned *t)
{
    int i, q, diff = 0;

    for (i = 0; i < 128; i++) {
        q = fog_q16(t[i]);
        if (q != fog_tab[i]) {
            fog_tab[i] = q;
            diff = 1;
        }
    }
    fog_dirty |= diff;
}

void zfog_color(unsigned c)
{
    GLfixed r = (GLfixed)((c >> 16) & 255) * 257, g = (GLfixed)((c >> 8) & 255) * 257,
            b = (GLfixed)(c & 255) * 257;

    if (r != fog_col[0] || g != fog_col[1] || b != fog_col[2]) {
        fog_col[0] = r;
        fog_col[1] = g;
        fog_col[2] = b;
        fog_dirty = 1;
    }
}

void zfog_enable(int on)
{
    fog_on = on;
}

static unsigned fog_dist(int i)
{
    return (fog_far * 16u) / ((unsigned)(16 + (i & 15)) << (i >> 4));
}

static void fog_compute(void)
{
    int i, lo = -1, hi = -1, top = 0;
    unsigned s, e;

    fog_dirty = 0;
    fog_sent = 0;
    fog_have = 0;
    for (i = 0; i < 128; i++) {
        if (fog_tab[i] > fog_tab[top]) {
            top = i;
        }
        if (fog_tab[i] >= 64225 && hi < 0) {
            hi = i;
        }
        if (fog_tab[i] >= 1311) {
            lo = i;
        }
    }
    if (hi < 0) {
        hi = top;
    }
    zlog("tfog", "tabela: ultima entrada com >2% / primeira com >98%", lo * 1000 + hi);
    if (lo < 0 || fog_far == 0) {
        return;
    }
    s = fog_dist(lo);
    e = fog_dist(hi);
    if (s >= 32000) { /* o GL de ponto fixo vai ate 32767: tao longe que nao ha nevoa */
        return;
    }
    if (e > 32000) {
        e = 32000;
    }
    if (e <= s) {
        e = s + 1;
    }
    fog_start = (GLfixed)(s << 16);
    fog_end = (GLfixed)(e << 16);
    fog_have = 1;
    zlog("tfog", "inicio (unidades)", (int)s);
    zlog("tfog", "fim (unidades)", (int)e);
}

static void gl_fog(int on)
{
    if (sh_fog == on) {
        return;
    }
    sh_fog = on;
    if (on) {
        glEnable(GL_FOG);
    } else {
        glDisable(GL_FOG);
    }
}

static unsigned fog_n_com, fog_n_off, fog_n_sem;
static unsigned zl_n_promoted, zl_n_cpu, zl_why[16];

/* zlive.c: um meshset com USE_ALPHA foi para o caminho GL (opaco de fato) ou ficou no de CPU */
void zstrip_note_alpha(int why)
{
    if (why == 1 || why == 7) {
        zl_n_promoted++;
        if (why == 7) {
            zl_why[7]++;
        }
    } else {
        zl_n_cpu++;
        zl_why[why & 15]++;
    }
}

void zfog_report(void)
{
    static unsigned c, o, s, pp, pc;

    if (zl_n_promoted != pp || zl_n_cpu != pc) {
        zlogd("tzl", "meshsets USE_ALPHA promovidos ao caminho GL (total)", (int)zl_n_promoted);
        zlogd("tzl", "meshsets USE_ALPHA no caminho de CPU (total)", (int)zl_n_cpu);
        zlogd("tzl", "  motivo: cor por vertice", (int)zl_why[2]);
        zlogd("tzl", "  motivo: mistura diferente de alfa/1-alfa", (int)zl_why[3]);
        zlogd("tzl", "  motivo: alfa do difuso < 255", (int)zl_why[4]);
        zlogd("tzl", "  motivo: material constante/offset", (int)zl_why[5]);
        zlogd("tzl", "  motivo: textura com alfa parcial", (int)zl_why[6]);
        zlogd("tzl", "  motivo: textura nao carregada", (int)zl_why[8]);
        zlogd("tzl", "  motivo: sem texlist / indice fora", (int)zl_why[9]);
        zlogd("tzl", "  (dos promovidos) recorte com alfa 0/255", (int)zl_why[7]);
        pp = zl_n_promoted;
        pc = zl_n_cpu;
    }

    if (fog_n_com != c || fog_n_off != o || fog_n_sem != s) {
        zlogd("tfog", "modelos com nevoa (total)", (int)fog_n_com);
        zlogd("tfog", "modelos com njFogDisable (total)", (int)fog_n_off);
        zlogd("tfog", "modelos sem faixa visivel (total)", (int)fog_n_sem);
        zlogd("tfog", "cor 0xRRGGBB", (int)(((fog_col[0] >> 8) << 16) | ((fog_col[1] >> 8) << 8) | (fog_col[2] >> 8)));
        c = fog_n_com;
        o = fog_n_off;
        s = fog_n_sem;
    }
}

static void fog_for_model(void)
{
    if (!Z_FOG) {
        return;
    }
    if (fog_dirty) {
        fog_compute();
    }
    if (!fog_on || !fog_have) {
        if (!fog_on) {
            fog_n_off++;
        } else {
            fog_n_sem++;
        }
        gl_fog(0);
        return;
    }
    fog_n_com++;
    if (!fog_sent) {
        glFogx(GL_FOG_MODE, GL_LINEAR);
        glFogx(GL_FOG_START, fog_start);
        glFogx(GL_FOG_END, fog_end);
        glFogxv(GL_FOG_COLOR, fog_col);
        fog_sent = 1;
    }
    gl_fog(1);
}

static void gl_shadow_reset(void)
{
    sh_blend = sh_dmask = sh_tex2d = sh_mm = -1;
    sh_fog = -1;
    fog_sent = 0;
    sh_proj_ok = 0;
    sh_model_ok = 0;
    sh_av = sh_at = sh_ac = 0;
    sh_bound = 0xffffffffu;
}

static int gl_same(int *shadow, int v)
{
    if (Z_STATECACHE && *shadow == v) {
        zs_redundant++;
        return 1;
    }
    *shadow = v;
    return 0;
}

static void gl_blend(int on)
{
    if (!gl_same(&sh_blend, on)) {
        if (on) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
    }
}

static void gl_dmask(int on)
{
    if (!gl_same(&sh_dmask, on)) {
        glDepthMask(on ? GL_TRUE : GL_FALSE);
    }
}

static void gl_tex2d(int on)
{
    if (!gl_same(&sh_tex2d, on)) {
        if (on) {
            glEnable(GL_TEXTURE_2D);
        } else {
            glDisable(GL_TEXTURE_2D);
        }
    }
}

/* ligar a textura so quando muda; os outros pontos que ligam ou apagam texturas
 * (criacao, filme, delete) usam gl_bind_raw/gl_bound_unknown para manter o espelho */
static void gl_bind(unsigned name)
{
    if (Z_STATECACHE && sh_bound == name) {
        zs_redundant++;
        return;
    }
    sh_bound = name;
    glBindTexture(GL_TEXTURE_2D, name);
}

static void gl_bind_raw(unsigned name)
{
    sh_bound = name;
    glBindTexture(GL_TEXTURE_2D, name);
}

static void gl_mm(GLenum m)
{
    if (!gl_same(&sh_mm, (int)m)) {
        glMatrixMode(m);
    }
}

/* os três ponteiros de vértice; tt = tipo das coordenadas de textura */
static void gl_arrays(const void *v, const void *t, int tt, const void *c)
{
    if (Z_STATECACHE && v == sh_av && t == sh_at && tt == sh_att && c == sh_ac) {
        zs_redundant++;
        return;
    }
    sh_av = v;
    sh_at = t;
    sh_att = tt;
    sh_ac = c;
    glVertexPointer(3, GL_FIXED, 0, v);
    glTexCoordPointer(2, (GLenum)tt, 0, t);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, c);
}

static int proj_differs(void)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (l_proj[i] != m_proj[i]) {
            return 1;
        }
    }
    return 0;
}

static void mode_screen(void)
{
    if (!m_mode) {
        return;
    }
    m_mode = 0;
    gl_fog(0);
    sh_proj_ok = 0;
    sh_model_ok = 0;
    gl_mm(GL_PROJECTION);
    glLoadIdentity();
    glOrthox(0, 640 << 16, 480 << 16, 0, SCREEN_NEAR, SCREEN_FAR);
    gl_mm(GL_MODELVIEW);
    glLoadIdentity();
    gl_mm(GL_TEXTURE);
    glLoadIdentity();
    gl_mm(GL_MODELVIEW);
}

/* --- modelos translucidos da via GL, adiados --- (zlive.c: um meshset com mistura alfa que o
 * caminho de CPU projetava, sombreava e ordenava em float, ~25% do tempo do 3D). O lote
 * guarda os vertices ja prontos (o mesmo buffer do modelo) com as duas matrizes, e entra na
 * lista dos translucidos com a chave 1/z do modelo; zstrip_flush o desenha na ordem, de
 * tras para a frente, como os strips em tela. A GPU transforma e recorta */
#define TM_VERTS 32768
#define TM_BATCHES 768
static GLfixed *tm_xyz;
static GLfixed *tm_uv;
static GLubyte *tm_rgba;
static int tm_verts, tm_n, tm_fail;
static struct tmodel {
    int first, count, textured;
    GLuint tex;
    GLenum src, dst;
    GLfixed proj[16], model[16];
} tm_b[TM_BATCHES];
static int m_defer;
static float m_defer_key;
static void tm_capture(void);
static void opaque_flush(void);

static void model_flush(void)
{
    int i;

    if (m_count < 3) {
        m_count = 0;
        return;
    }
    if (m_defer && tm_n < TM_BATCHES && t_count < TLIST_STRIPS) {
        tm_capture();
        return;
    }
    if (m_defer) { /* pool cheio: desenha agora, na ordem do jogo, ainda translucido */
        gl_blend(1);
        gl_dmask(0);
        glBlendFunc(st_src, st_dst);
    }
    if (!m_mode) {
        m_mode = 1;
        gl_mm(GL_TEXTURE);
        glLoadIdentity();
        glScalex(256, 256, 1 << 16); /* uv em 8.8 */
    }
    /* a projeção quase nunca muda entre modelos; glLoadMatrix já substitui a
     * matriz, então o glLoadIdentity antes dele não faz nada */
    if (!Z_STATECACHE || !sh_proj_ok || proj_differs()) {
        gl_mm(GL_PROJECTION);
        glLoadMatrixx(m_proj);
        for (i = 0; i < 16; i++) {
            l_proj[i] = m_proj[i];
        }
        sh_proj_ok = 1;
    } else {
        zs_redundant++;
    }
    /* a matriz do modelo tambem se repete: particulas, sprites e efeitos reusam a do
     * objeto anterior, e cada LoadMatrixx custa 16 GLfixed. LoadMatrix substitui a
     * matriz, entao nao ha identidade antes quando o cache vale.
     * A comparacao e contra o que esta no GL AGORA, aqui no flush: uma flag marcada no
     * zstrip_model_begin mentia quando outro modelo (ou o passe 2D) mexia na MODELVIEW
     * entre o begin e este flush -- o modelo deferido era desenhado com a matriz de
     * outro objeto e a sprite saia esticada/trocada (halo do sol, painel do menu). */
    gl_mm(GL_MODELVIEW);
    {
        int same = sh_model_ok != 0;

        for (i = 0; same && i < 16; i++) {
            if (l_model[i] != m_model[i]) {
                same = 0;
            }
        }
        if (!Z_STATECACHE || !same) {
            if (!Z_STATECACHE) {
                glLoadIdentity();
            }
            glLoadMatrixx(m_model);
            for (i = 0; i < 16; i++) {
                l_model[i] = m_model[i];
            }
            sh_model_ok = 1;
        } else {
            zs_redundant++;
        }
    }
    /* o modelo entra no hash com a matriz (posicao e orientacao do objeto, camera) e o
     * primeiro vertice: cobre o estado do jogo que so aparece em 3D */
    hash_mix((unsigned)m_count);
    for (i = 0; i < 16; i++) {
        hash_mix((unsigned)m_model[i]);
    }
    hash_mix((unsigned)m_xyz[0]);
    hash_mix((unsigned)m_xyz[1]);
    hash_mix((unsigned)m_xyz[2]);
    gl_arrays(m_xyz, m_uv, GL_SHORT, m_rgba);
    fog_for_model();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, m_count);
    m_count = 0;
}

void zstrip_model_flush(void)
{
    model_flush();
}

void zstrip_defer(int on, float key)
{
    opaque_flush();
    m_defer = on;
    m_defer_key = key;
}

static void tm_capture(void)
{
    struct tmodel *b;
    struct tstrip *t;
    int i;

    if (tm_xyz == 0) {
        tm_xyz = (GLfixed *)MALLOC((uint32)TM_VERTS * 3 * sizeof(GLfixed));
        tm_uv = (GLfixed *)MALLOC((uint32)TM_VERTS * 2 * sizeof(GLfixed));
        tm_rgba = (GLubyte *)MALLOC((uint32)TM_VERTS * 4);
    }
    if (tm_xyz == 0 || tm_uv == 0 || tm_rgba == 0 || tm_verts + m_count > TM_VERTS) {
        if (tm_fail < 3) {
            tm_fail++;
            zlog("vlive", "lote translucido descartado", m_count);
        }
        m_count = 0;
        return;
    }
    b = &tm_b[tm_n++];
    b->first = tm_verts;
    b->count = m_count;
    b->textured = st_textured;
    b->tex = st_tex;
    b->src = st_src;
    b->dst = st_dst;
    for (i = 0; i < 16; i++) {
        b->proj[i] = m_proj[i];
        b->model[i] = m_model[i];
    }
    for (i = 0; i < m_count * 3; i++) {
        tm_xyz[tm_verts * 3 + i] = m_xyz[i];
    }
    for (i = 0; i < m_count * 2; i++) {
        tm_uv[tm_verts * 2 + i] = (GLfixed)m_uv[i] << 8; /* 8.8 -> 16.16 */
    }
    for (i = 0; i < m_count * 4; i++) {
        tm_rgba[tm_verts * 4 + i] = m_rgba[i];
    }
    tm_verts += m_count;
    t = &t_strips[t_count++];
    t->first = 0;
    t->count = 0;
    t->textured = 0;
    t->tex = 0;
    t->src = 0;
    t->dst = 0;
    t->key = m_defer_key;
    t->model = tm_n;
    m_count = 0;
}

/* proj, model: 16 GLfixed em colunas */
void zstrip_model_begin(const int *proj, const int *model)
{
    int i;

    model_flush();
    for (i = 0; i < 16; i++) {
        m_proj[i] = proj[i];
        m_model[i] = model[i];
    }
}

/* um vértice: pontos (xyz fixos), uv em 8.8, cor ARGB do jogo. first = 1 no
 * começo de um strip */
void zstrip_model_vertex(const int *xyz, int u, int v,
                         uint32_t argb, int first)
{
    int at;

    if (m_count + 3 >= MDL_MOST) {
        model_flush();
        first = 1;
    }
    at = m_count;
    if (first && at > 0) {
        /* degenerados e paridade, como em opaque_add */
        int i;

        for (i = 0; i < 3; i++) {
            m_xyz[at * 3 + i] = m_xyz[(at - 1) * 3 + i];
        }
        m_uv[at * 2] = m_uv[(at - 1) * 2];
        m_uv[at * 2 + 1] = m_uv[(at - 1) * 2 + 1];
        for (i = 0; i < 4; i++) {
            m_rgba[at * 4 + i] = m_rgba[(at - 1) * 4 + i];
        }
        at++;
        for (i = 0; i < 3; i++) {
            m_xyz[at * 3 + i] = xyz[i];
        }
        m_uv[at * 2] = (GLshort)u;
        m_uv[at * 2 + 1] = (GLshort)v;
        m_rgba[at * 4 + 0] = (GLubyte)(argb >> 16);
        m_rgba[at * 4 + 1] = (GLubyte)(argb >> 8);
        m_rgba[at * 4 + 2] = (GLubyte)argb;
        m_rgba[at * 4 + 3] = (GLubyte)(argb >> 24);
        at++;
        if (at & 1) { /* o strip novo começa em posição par */
            for (i = 0; i < 3; i++) {
                m_xyz[at * 3 + i] = xyz[i];
            }
            m_uv[at * 2] = (GLshort)u;
            m_uv[at * 2 + 1] = (GLshort)v;
            m_rgba[at * 4 + 0] = (GLubyte)(argb >> 16);
            m_rgba[at * 4 + 1] = (GLubyte)(argb >> 8);
            m_rgba[at * 4 + 2] = (GLubyte)argb;
            m_rgba[at * 4 + 3] = (GLubyte)(argb >> 24);
            at++;
        }
    }
    {
        int i;

        for (i = 0; i < 3; i++) {
            m_xyz[at * 3 + i] = xyz[i];
        }
        m_uv[at * 2] = (GLshort)u;
        m_uv[at * 2 + 1] = (GLshort)v;
        m_rgba[at * 4 + 0] = (GLubyte)(argb >> 16);
        m_rgba[at * 4 + 1] = (GLubyte)(argb >> 8);
        m_rgba[at * 4 + 2] = (GLubyte)argb;
        m_rgba[at * 4 + 3] = (GLubyte)(argb >> 24);
    }
    m_count = at + 1;
}

/* contadores por janela de 2 s (app.c vprof): chamadas de desenho e o estado GL
 * que o passe translucido repetiu */
/* STATECACHE=1 (padrao): o passe translucido so troca o estado GL quando muda */

static void opaque_flush(void)
{
    model_flush();
    if (o_count < 3) {
        o_count = 0;
        return;
    }
    mode_screen();
    /* A lista 2D fica sobre o quadro 3D. As prioridades do Ninja ordenam os
     * quads entre si; testar contra o Z da cena escondia a HUD. */
    screen_depth_apply();
    gl_arrays(o_xyz, o_uv, GL_FIXED, o_rgba);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, o_count);
    glEnable(GL_DEPTH_TEST);
    zs_draws++;
    o_count = 0;
}

static void o_copy(int to, int from)
{
    int i;

    for (i = 0; i < 3; i++) {
        o_xyz[to * 3 + i] = vxyz[from * 3 + i];
    }
    for (i = 0; i < 2; i++) {
        o_uv[to * 2 + i] = vuv[from * 2 + i];
    }
    for (i = 0; i < 4; i++) {
        o_rgba[to * 4 + i] = vrgba[from * 4 + i];
    }
}

/* o strip atual (vert_count vértices) entra no lote */
static void opaque_add(void)
{
    int i, at;

    if (o_count + vert_count + 3 > OPQ_MOST) {
        opaque_flush();
    }
    at = o_count;
    if (at > 0) {
        /* degenerados: último do lote, primeiro do novo, e mais um se for
         * preciso para o novo começar em posição par (paridade da face) */
        for (i = 0; i < 3; i++) {
            o_xyz[at * 3 + i] = o_xyz[(at - 1) * 3 + i];
        }
        for (i = 0; i < 2; i++) {
            o_uv[at * 2 + i] = o_uv[(at - 1) * 2 + i];
        }
        for (i = 0; i < 4; i++) {
            o_rgba[at * 4 + i] = o_rgba[(at - 1) * 4 + i];
        }
        at++;
        o_copy(at++, 0);
        if (at & 1) {
            o_copy(at++, 0);
        }
    }
    for (i = 0; i < vert_count; i++) {
        o_copy(at++, i);
    }
    o_count = at;
}

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
    vz[at] = z;
    vuv[at * 2 + 0] = FIX(u);
    vuv[at * 2 + 1] = FIX(v);
    /* GL espera RGBA; a convenção do jogo é ARGB. */
    vrgba[at * 4 + 0] = byte_of(argb, 16);
    vrgba[at * 4 + 1] = byte_of(argb, 8);
    vrgba[at * 4 + 2] = byte_of(argb, 0);
    vrgba[at * 4 + 3] = byte_of(argb, 24);
}

static void defer_translucent(void)
{
    struct tstrip *t;
    float sum = 0.0f;
    int i;

    if (t_count >= TLIST_STRIPS || t_verts + vert_count > TLIST_VERTS) {
        zstrip_flush(); /* cheio: desenha o que tem e recomeça */
    }
    t = &t_strips[t_count++];
    t->model = 0;
    t->first = t_verts;
    t->count = vert_count;
    t->textured = st_textured;
    t->tex = st_tex;
    t->src = st_src;
    t->dst = st_dst;
    for (i = 0; i < vert_count; i++) {
        sum += vz[i];
    }
    t->key = sum / (float)vert_count;
    for (i = 0; i < vert_count * 3; i++) {
        t_xyz[t_verts * 3 + i] = vxyz[i];
    }
    for (i = 0; i < vert_count * 2; i++) {
        t_uv[t_verts * 2 + i] = vuv[i];
    }
    for (i = 0; i < vert_count * 4; i++) {
        t_rgba[t_verts * 4 + i] = vrgba[i];
    }
    t_verts += vert_count;
}

/* Quadro que o applet nao vai apresentar (MENUFPS30): o que ja foi ao GL fica no buffer que o
 * proximo quadro limpa; os translucidos e os lotes 3D adiados, que so se desenham no flush,
 * sao descartados com os mesmos zeros do fim de zstrip_flush */
void zstrip_frame_drop(void)
{
    opaque_flush();
    mode_screen();
    t_count = 0;
    t_verts = 0;
    tm_n = 0;
    tm_verts = 0;
}

void zstrip_flush(void)
{
    int i, j;

    opaque_flush();
    mode_screen(); /* o que vem depois é 2D: matrizes e luz do 2D */
    if (t_count == 0) {
        return;
    }
    /* ordem: z médio crescente (longe primeiro), estável: radix LSD sobre os bits do float
     * (a inserção de antes era O(n^2) com comparação de float em software: ate 1024 strips
     * por quadro). Mesma ordem: o radix e estavel e o float vira inteiro ordenavel */
    {
        static unsigned ku[TLIST_STRIPS];
        static int tmp[TLIST_STRIPS];
        int *src = t_order, *dst = tmp, *swap;
        unsigned cnt[256];
        int pass;

        for (i = 0; i < t_count; i++) {
            union {
                float f;
                unsigned u;
            } c;

            c.f = t_strips[i].key;
            ku[i] = (c.u & 0x80000000u) ? ~c.u : (c.u | 0x80000000u);
            src[i] = i;
        }
        zs_tstrips += (unsigned)t_count;
        for (pass = 0; pass < 4; pass++) {
            unsigned shift = (unsigned)pass * 8u, at = 0;

            for (j = 0; j < 256; j++) {
                cnt[j] = 0;
            }
            for (i = 0; i < t_count; i++) {
                cnt[(ku[src[i]] >> shift) & 255u]++;
            }
            if (cnt[(ku[src[0]] >> shift) & 255u] == (unsigned)t_count) {
                continue; /* todos iguais neste byte */
            }
            for (j = 0; j < 256; j++) {
                unsigned n = cnt[j];

                cnt[j] = at;
                at += n;
            }
            for (i = 0; i < t_count; i++) {
                dst[cnt[(ku[src[i]] >> shift) & 255u]++] = src[i];
            }
            swap = src;
            src = dst;
            dst = swap;
        }
        if (src != t_order) {
            for (i = 0; i < t_count; i++) {
                t_order[i] = src[i];
            }
        }
    }
    /* os strips vêm em pixels de tela, como o passe 2D: projeção e matriz
     * do 2D, mesmo que o quadro tenha mexido nelas (cubo de teste, 3D) */
    gl_mm(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrthox(0, 640 << 16, 480 << 16, 0, SCREEN_NEAR, SCREEN_FAR);
    gl_mm(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDepthFunc(GL_LEQUAL);
    screen_depth_apply(); /* menu com preview: Z misto; HUD/pause: ordem de pintura */
    gl_blend(1);
    gl_dmask(0);
    gl_arrays(t_xyz, t_uv, GL_FIXED, t_rgba);
    {
        /* -1 / 0xffffffff = estado desconhecido: o primeiro strip sempre emite */
        unsigned p_src = 0xffffffffu, p_dst = 0xffffffffu;

        int in2d = 1; /* as matrizes e os arranjos sao os do 2D */

        {
            GLuint p_tex = 0xffffffffu; /* runs de (textura, blend) na ordem por z */
            /* strips 2D vizinhos no pool (first == fim do anterior) e com o mesmo estado
             * entram num DrawArrays so: sao 125 strips translucidos por quadro e cada um
             * era uma chamada. A ordem por z nao muda: so junta o que ja era vizinho */
            int run_first = -1, run_count = 0, run_tail = 0;
            GLuint run_tex = 0;
            GLenum run_src = 0, run_dst = 0;
            int run_textured = 0;

#define EMIT_RUN()                                                              \
    do {                                                                        \
        if (run_first >= 0) {                                                   \
            gl_arrays(t_xyz, t_uv, GL_FIXED, t_rgba);                           \
            glDrawArrays(GL_TRIANGLE_STRIP, run_first, run_count);              \
            zs_draws++;                                                         \
            run_first = -1;                                                     \
        }                                                                       \
    } while (0)

            zs_tverts += (unsigned)t_verts;
        for (i = 0; i < t_count; i++) {
            const struct tstrip *t = &t_strips[t_order[i]];
            const struct tmodel *b = t->model ? &tm_b[t->model - 1] : 0;
            GLenum src = b ? b->src : t->src, dst = b ? b->dst : t->dst;
            int textured = b ? b->textured : t->textured;
            GLuint tex = b ? b->tex : t->tex;
            int join = !b && Z_TJOIN && run_first >= 0 && t->first == run_tail &&
                       textured == run_textured && (!textured || tex == run_tex) &&
                       src == run_src && dst == run_dst;

            /* O run pendente ainda depende do estado anterior. Emita-o ANTES de ligar a
             * textura/blend deste strip; fazer isso depois deslocava o estado em um strip:
             * botao do pause com textura do cenario, halos e quads gigantes. */
            if (!join) {
                EMIT_RUN();
            }
            if (!textured ? (p_tex != 0xfffffffeu) : (p_tex != tex)) {
                zs_truns++;
            }
            p_tex = textured ? tex : 0xfffffffeu;
            gl_tex2d(textured ? 1 : 0);
            if (textured) {
                gl_bind((unsigned)tex);
            }
            if (p_src == (unsigned)src && p_dst == (unsigned)dst) {
                zs_redundant++;
            }
            if (!Z_STATECACHE || p_src != (unsigned)src || p_dst != (unsigned)dst) {
                glBlendFunc(src, dst);
            }
            p_src = (unsigned)src;
            p_dst = (unsigned)dst;
            if (b) {
                glEnable(GL_DEPTH_TEST);
                gl_mm(GL_PROJECTION);
                glLoadMatrixx(b->proj);
                gl_mm(GL_MODELVIEW);
                glLoadMatrixx(b->model);
                gl_arrays(tm_xyz, tm_uv, GL_FIXED, tm_rgba);
                glDrawArrays(GL_TRIANGLE_STRIP, b->first, b->count);
                in2d = 0;
                zs_draws++;
            } else {
                if (!in2d) {
                    gl_mm(GL_PROJECTION);
                    glLoadIdentity();
                    glOrthox(0, 640 << 16, 480 << 16, 0, SCREEN_NEAR, SCREEN_FAR);
                    gl_mm(GL_MODELVIEW);
                    glLoadIdentity();
                    screen_depth_apply();
                    in2d = 1;
                }
                if (join) {
                    run_count += t->count;
                    run_tail += t->count;
                    zs_tjoined++;
                } else {
                    run_first = t->first;
                    run_count = t->count;
                    run_tail = t->first + t->count;
                    run_tex = tex;
                    run_src = src;
                    run_dst = dst;
                    run_textured = textured;
                }
            }
        }
        EMIT_RUN();
#undef EMIT_RUN
        }
    }
    glBlendFunc(st_src, st_dst);
    gl_mm(GL_PROJECTION);
    glPopMatrix();
    gl_mm(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    t_count = 0;
    t_verts = 0;
    tm_n = 0;
    tm_verts = 0;
    /* volta ao estado do strip aberto */
    zstrip_blend(st_blend);
    if (st_textured) {
        gl_tex2d(1);
        gl_bind(st_tex);
    } else {
        gl_tex2d(0);
    }
}

int zstrip_stat_opaque, zstrip_stat_trans; /* diagnóstico por tique */

void zstrip_end(void)
{
    if (vert_count < 3) {
        vert_count = 0;
        return;
    }
    hash_mix((unsigned)vert_count);
    hash_mix((unsigned)vxyz[0]);
    hash_mix((unsigned)vxyz[1]);
    hash_mix((unsigned)vxyz[2]);
    hash_mix((unsigned)st_tex + (unsigned)st_blend);
    if (st_blend) {
        zstrip_stat_trans++;
        defer_translucent();
        vert_count = 0;
        return;
    }
    opaque_add();
    zstrip_stat_opaque++;
    vert_count = 0;
}

/* fatores do Kamui (KM_* de kamui.h, campos SRC/DSTBlendingMode) em GL */
static GLenum gl_factor(int km, int is_src)
{
    switch (km) {
    case 0: /* KM_BOTHINVSRCALPHA */
        return is_src ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA;
    case 1: /* KM_BOTHSRCALPHA */
        return is_src ? GL_SRC_ALPHA : GL_ONE_MINUS_SRC_ALPHA;
    case 2: return GL_DST_ALPHA;
    case 3: return GL_DST_COLOR;
    case 4: return GL_ONE_MINUS_DST_ALPHA;
    case 5: return GL_ONE_MINUS_DST_COLOR;
    case 6: return GL_ONE_MINUS_SRC_ALPHA;
    case 7: return GL_ONE_MINUS_SRC_COLOR;
    case 8: return GL_SRC_ALPHA;
    case 9: return GL_SRC_COLOR;
    case 10: return GL_ONE;
    case 11: return GL_ZERO;
    default:
        return is_src ? GL_SRC_ALPHA : GL_ONE_MINUS_SRC_ALPHA;
    }
}

/* o modo de blend do contexto do Ninja (njColorBlendingMode) para os
 * próximos strips. Um BOTH* em qualquer campo define os dois fatores. */
void zstrip_blend_mode(int km_src, int km_dst)
{
    opaque_flush();
    if (km_src == 0 || km_src == 1) {
        st_src = gl_factor(km_src, 1);
        st_dst = gl_factor(km_src, 0);
    } else if (km_dst == 0 || km_dst == 1) {
        st_src = gl_factor(km_dst, 1);
        st_dst = gl_factor(km_dst, 0);
    } else {
        st_src = gl_factor(km_src, 1);
        st_dst = gl_factor(km_dst, 0);
    }
}

/* fatores dos materiais do Ninja (NJD_SA_/NJD_DA_ >> 29 e >> 26): os códigos
 * do PVR, não os KM_* de kamui.h. 0 zero, 1 um, 2 cor do outro lado, 3 o
 * inverso, 4 alfa da fonte, 5 inverso, 6 alfa do destino, 7 inverso */
void zstrip_blend_pvr(int src, int dst)
{
    static const GLenum sf[8] = {GL_ZERO, GL_ONE, GL_DST_COLOR,
                                 GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA,
                                 GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ZERO};
    static const GLenum df[8] = {GL_ZERO, GL_ONE, GL_SRC_COLOR,
                                 GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA,
                                 GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ZERO};

    opaque_flush();
    /* sem alfa de destino no RGB565 do Zeebo: zblend_code troca 6 e 7 por
     * 1 e 0 (as entradas 6 e 7 das tabelas não são usadas) */
    st_src = sf[zblend_code(src)];
    st_dst = df[zblend_code(dst)];
}

void zstrip_blend(int on)
{
    opaque_flush();
    /* translúcido não grava profundidade (como no PVR) e é adiado para
     * zstrip_flush, que ordena a lista */
    st_blend = on;
    gl_blend(on ? 1 : 0);
    gl_dmask(on ? 0 : 1);
}

void zstrip_text_janela(unsigned *binds, unsigned *distintas, unsigned *runs, unsigned *tverts,
                        unsigned *unidos)
{
    *binds = zs_texbinds;
    *distintas = zs_texdistinct;
    *runs = zs_truns;
    *tverts = zs_tverts;
    *unidos = zs_tjoined;
    zs_texbinds = 0;
    zs_texdistinct = 0;
    zs_truns = 0;
    zs_tverts = 0;
    zs_tjoined = 0;
    {
        int k;

        for (k = 0; k < TS_SEEN; k++) {
            ts_seen[k] = 0;
        }
    }
}

void zstrip_use_texture(void)
{
    opaque_flush();
    st_textured = 1;
    st_tex = tex_current != 0 ? tex_current : texture;
    zs_texbinds++;
    {
        unsigned k = (st_tex * 2654435761u) >> 23; /* 512 slots */

        for (;;) {
            if (ts_seen[k] == st_tex) {
                break;
            }
            if (ts_seen[k] == 0) {
                ts_seen[k] = st_tex;
                zs_texdistinct++;
                break;
            }
            k = (k + 1) & (TS_SEEN - 1);
        }
    }
    gl_tex2d(1);
    gl_bind(st_tex);
}

#ifndef Z_TEX16
#define Z_TEX16 0 /* 1 = texturas em 16 bpp (a metade da memoria e da banda) */
#endif

/* O jogo entrega texturas PVR de 16 bits (565, 1555, 4444) que o port expande para
 * RGBA8888. Aqui elas voltam a 16 bits sem perda: opaca vira 565, alfa so 0/255 vira
 * 5551, o resto 4444 (cada canal de 4 ou 5 bits expandido por replicacao volta ao valor
 * original com o deslocamento). Devolve 0 quando nao converteu (flag desligada ou sem
 * memoria) e o chamador envia RGBA8888. */
static int tex16_upload(int w, int h, const unsigned char *p)
{
    int n = w * h, i, opaque = 1, binary = 1;
    unsigned short *o;

    if (!Z_TEX16) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        unsigned char a = p[i * 4 + 3];

        if (a != 255) {
            opaque = 0;
            if (a != 0) {
                binary = 0;
                break;
            }
        }
    }
    o = (unsigned short *)MALLOC((uint32)n * 2);
    if (o == 0) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        unsigned r = p[i * 4], g = p[i * 4 + 1], b = p[i * 4 + 2], a = p[i * 4 + 3];

        if (opaque) {
            o[i] = (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        } else if (binary) {
            o[i] = (unsigned short)(((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) |
                                    (a >> 7));
        } else {
            o[i] = (unsigned short)(((r >> 4) << 12) | ((g >> 4) << 8) | ((b >> 4) << 4) |
                                    (a >> 4));
        }
    }
    if (opaque) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, o);
    } else if (binary) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, o);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, o);
    }
    FREE(o);
    zs_tex16++;
    return 1;
}

int zstrip_tex_new(int w, int h, const void *rgba)
{
    opaque_flush();
    GLuint name = 0;
    int slot;

    if (w <= 0 || h <= 0) {
        return -1;
    }
    /* reaproveita um índice que zstrip_tex_free devolveu */
    for (slot = 0; slot < tex_count && tex_names[slot] != 0; slot++) {
    }
    if (slot == ZTEX_MOST) {
        zlog("tex", "tabela GL cheia", ZTEX_MOST);
        return -1;
    }
    glGenTextures(1, &name);
    gl_bind_raw(name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (!tex16_upload(w, h, (const unsigned char *)rgba)) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, rgba);
    }
    tex_names[slot] = name;
    {
        const unsigned *px = (const unsigned *)rgba;
        int n = w * h, i;

        unsigned a = 255u;

        for (i = 0; i < n; i++) {
            a = px[i] >> 24;
            if (a != 255u) {
                break;
            }
        }
        if (i == n) {
            tex_opq[slot] = 1;
        } else {
            for (; i < n; i++) {
                a = px[i] >> 24;
                if (a != 255u && a != 0u) {
                    break;
                }
            }
            tex_opq[slot] = (unsigned char)(i == n ? 2 : 0);
        }
    }
    if (slot == tex_count) {
        tex_count++;
    }
    return slot;
}

/* a textura index nao tem texel com alfa: um material com USE_ALPHA e mistura alfa/1-alfa
 * que a usa sai opaco (o jogo liga USE_ALPHA em quase tudo) */
int zstrip_tex_opaque(int index)
{
    return index >= 0 && index < tex_count && index != movie_index ? tex_opq[index] : 0;
}

/* recorte (textura com alfa so 0 e 255): mistura alfa/1-alfa com z gravado e sem os texels
 * transparentes (teste de alfa > 0): nao depende da ordem, entao nao precisa da lista dos
 * translucidos */
static int sh_cut;

void zstrip_cutout(int on)
{
    opaque_flush();
    if (on) {
        gl_blend(1);
        gl_dmask(1);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_ALPHA_TEST);
        glAlphaFuncx(GL_GREATER, 0);
        sh_cut = 1;
    } else if (sh_cut) {
        glDisable(GL_ALPHA_TEST);
        glBlendFunc(st_src, st_dst);
        sh_cut = 0;
    }
}

/* devolve a textura GL de index (kmFreeTexture): o Ninja libera as texturas
 * de uma fase ao trocar de fase; sem isto a tabela enchia nos menus e as
 * texturas da fase seguinte ficavam sem superfície (cinza) */
void zstrip_tex_free(int index)
{
    opaque_flush();
    if (index >= 0 && index < tex_count && index != movie_index &&
        tex_names[index] != 0) {
        if (tex_current == tex_names[index]) {
            tex_current = 0;
        }
        glDeleteTextures(1, &tex_names[index]);
        sh_bound = 0xffffffffu;
        tex_names[index] = 0;
    }
}

void zstrip_tex_select(int index)
{
    tex_current = (index >= 0 && index < tex_count) ? tex_names[index] : 0;
}

/* textura dos filmes (zmovie.c): 512x256 RGB565, o quadro no canto */

int zstrip_movie_upload(const unsigned short *rgb565, int w, int h)
{
    opaque_flush();
    if (movie_index < 0) {
        GLuint name = 0;

        if (tex_count >= ZTEX_MOST) {
            return -1;
        }
        glGenTextures(1, &name);
        gl_bind_raw(name);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 256, 0, GL_RGB,
                     GL_UNSIGNED_SHORT_5_6_5, 0);
        tex_names[tex_count] = name;
        movie_index = tex_count++;
    }
    gl_bind_raw(tex_names[movie_index]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB,
                    GL_UNSIGNED_SHORT_5_6_5, rgb565);
    return movie_index;
}

void zstrip_tex_release_all(void)
{
    opaque_flush();
    movie_index = -1;
    if (tex_count > 0) {
        int i;

        glDeleteTextures(tex_count, tex_names);
        sh_bound = 0xffffffffu;
        /* os nomes apagados nao ficam na tabela: um indice velho nao pode apontar para a
         * textura de outro quadro depois que tex_count volta a subir */
        for (i = 0; i < tex_count; i++) {
            tex_names[i] = 0;
            tex_opq[i] = 0;
        }
    }
    tex_count = 0;
    tex_current = 0;
}

void zstrip_no_texture(void)
{
    opaque_flush();
    st_textured = 0;
    gl_tex2d(0);
}

/* brew_egl.c avisa o lado GL do tamanho real do painel; aqui só o viewport
 * importa. (O nome vem do contrato do sa1hello.) */
void Sa1Hello_Resize(int w, int h)
{
    static int probed;

    if (w > 0 && h > 0) {
        glViewport(0, 0, w, h);
    }
    if (!probed) { /* uma vez: o que o driver (zeebx ou Adreno) expoe */
        const unsigned char *e = glGetString(GL_EXTENSIONS);
        static char piece[72];
        int n = 0;

        probed = 1;
        if (e != 0) {
            for (;; e++) {
                if (*e == ' ' || *e == 0) {
                    if (n > 0) {
                        piece[n] = 0;
                        zlog("tgl", piece, 0);
                        n = 0;
                    }
                    if (*e == 0) {
                        break;
                    }
                } else if (n < (int)sizeof piece - 1) {
                    piece[n++] = (char)*e;
                }
            }
        }
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
    gl_bind_raw(texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    /* PRS -> PVM -> PVRT: a cadeia completa dos assets do jogo */
    if (zasset_prs_len != 0) {
        int unpacked = prs_unpack(zasset_prs, (int)zasset_prs_len, prs_out,
                                      (int)sizeof prs_out);

        if (unpacked > 0 &&
            zninja_load_scene_pvm(prs_out, unpacked) > 0) {
            /* o checker fica como textura de fallback (índice -1) */
            gl_bind_raw(texture);
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

/* estado GL comum ao applet e ao jogo */
void zscene_gl_init(void)
{
    gl_shadow_reset();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* x para a direita, y para baixo, como o jogo entrega. */
    glOrthox(0, 640 << 16, 480 << 16, 0, SCREEN_NEAR, SCREEN_FAR);
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
}

/* cena de teste do applet: textura procedural, cubo, sprites e áudio */
void zscene_init(void)
{
    texture_make();
    zscene_gl_init();
    zcube_init();
    zsprite_init();
    zaudio_init();
}

/* começo de um quadro do jogo: fundo preto. Profundidade: o jogo manda z =
 * 1/w (maior é mais perto). Na projeção 2D (glOrtho com perto -1 e longe 1) a
 * profundidade da janela é (1 - z) / 2: maior 1/w dá menor profundidade, então
 * o teste é LEQUAL com clear em 1. */
void zscene_game_frame(void)
{
    screen_depth_test = 0;
    gl_shadow_reset();
    glClearDepthx(1 << 16);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

void zscene_frame(uint32_t tick)
{
    screen_depth_test = 0;
    gl_shadow_reset();
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
    glOrthox(0, 640 << 16, 480 << 16, 0, SCREEN_NEAR, SCREEN_FAR);
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

    /* fim do passe 2D: a lista translúcida dele sai antes do 3D */
    zstrip_flush();
    /* passe 3D por cima, com depth próprio (clear 1, menor = mais perto) */
    glClearDepthx(1 << 16);
    glClear(GL_DEPTH_BUFFER_BIT);
    zcube_frame(tick);
}
