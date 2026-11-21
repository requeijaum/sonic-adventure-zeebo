/* zgame.c — pontas do jogo que dependem de como os programas são carregados.
 *
 * O Dreamcast lê cada programa (stage, personagem, filme) por cima do
 * anterior. Aqui eles são linkados na imagem (ver o roadmap, fase D); até lá
 * estas pontas ficam vazias para o 1ST_READ chegar o mais longe possível. */

struct native_program;
const struct native_program *native_programs; /* tabela de programas: vazia */
int native_program_count;

/* entradas dos programas carregados por gdFsRead: ainda sem programa */
void zlog(const char *what, const char *name, int value);
void native_vsync(void);

/* LOADER.BIN: o jogo terminou e pediu o carregador (sair para o BIOS/Dream
 * Passport). Sem programa para carregar, loga e segue entregando quadros. */
void loader_entry(void)
{
    zlog("main", "loader_entry: fim do jogo", (int)__builtin_return_address(0));
    for (;;) {
        native_vsync();
    }
}
/* MOVIE.BIN: no slice do decomp é C do host (engine/sa1/movie.c); aqui, sem
 * vídeo ainda: a abertura é pulada e o jogo segue para o menu */
/* não zero: "a abertura, ou o Start do controle, segue para os menus" (main.c) */
int movie_entry(int list) { (void)list; return 1; }

/* fim do bss do 1ST_READ no DC; só alimenta o cálculo do heap do Shinobi */
char _BSG_END[4];
