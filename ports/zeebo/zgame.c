/* zgame.c — pontas do jogo que dependem de como os programas são carregados.
 *
 * O Dreamcast lê cada programa (stage, personagem, filme) por cima do
 * anterior. Aqui eles são linkados na imagem (ver o roadmap, fase D); até lá
 * estas pontas ficam vazias para o 1ST_READ chegar o mais longe possível. */

struct native_program;
const struct native_program *native_programs; /* tabela de programas: vazia */
int native_program_count;

/* entradas dos programas carregados por gdFsRead: ainda sem programa */
void loader_entry(void) { for (;;) { } }
void movie_entry(void) { for (;;) { } }

/* fim do bss do 1ST_READ no DC; só alimenta o cálculo do heap do Shinobi */
char _BSG_END[4];
