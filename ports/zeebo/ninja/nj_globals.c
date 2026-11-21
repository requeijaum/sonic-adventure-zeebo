/* nj_globals.c — estado global do Ninja para o Z2.
 *
 * No jogo estas variáveis vêm de sdk/ninja/njSystem.c. Enquanto o 1ST_READ
 * ainda não está linkado, o bring-up define as mesmas. A pilha conta o nível
 * base: começa em 1, como engine/tests/dc_mesh.c faz. */

#include <ninja.h>

static NJS_MATRIX nj_stack[8];

NJS_MATRIX _unit_matrix_ = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                            0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};
NJS_MATRIX *_nj_current_matrix_ptr_ = nj_stack;
Int _nj_matrix_flag_;
Sint32 _stack_size_ = 1;
Sint32 _max_stack_size_ = 8;
