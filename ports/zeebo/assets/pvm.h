/*
 * sa1-zeebo — parser de containers PVM (PVMH) de textura Sega/Dreamcast.
 *
 * Um PVMH agrupa varios chunks PVRT. Diferente de pvr_scan(), que procura a
 * assinatura varrendo byte a byte, aqui o cabecalho informa a contagem e o
 * offset do primeiro chunk; cada chunk traz seu tamanho, e o proximo comeca
 * em offset + 8 + size. Isso e O(n) e deterministico.
 *
 * Freestanding: sem libc. O buffer fonte nao e copiado.
 */
#ifndef SA1_PVM_H
#define SA1_PVM_H

#include "pvr.h"

/* Numero maximo de texturas carregadas em um Pvm. O cabecalho guarda u16,
 * mas o alvo real tem folga; ajuste antes de incluir se necessario. */
#ifndef PVM_MAX_TEXTURES
#define PVM_MAX_TEXTURES 256
#endif

/* Uma textura do container: metadados de decodificacao (PvrTexture) mais a
 * fatia de payload que lhe pertence dentro do buffer PVM. */
typedef struct {
    PvrTexture pvr;             /* formato, dimensoes e campo size do PVRT */
    const unsigned char *data;  /* inicio do payload (PVRT + 16) */
    int data_len;               /* bytes disponiveis (size - 8) */
    unsigned int chunk_offset;  /* offset do chunk PVRT no buffer */
} PvmTexture;

/* Container parseado. `tex` guarda as texturas na ordem do arquivo. */
typedef struct {
    const unsigned char *buf;   /* buffer PVM (nao copiado) */
    int len;
    unsigned int flags;         /* campo flags do PVMH */
    int count;                  /* numero de texturas (= header u16) */
    PvmTexture tex[PVM_MAX_TEXTURES];
} Pvm;

/* Codigos de erro (todos negativos). */
#define PVM_ERR_ARG     (-1)    /* ponteiro nulo / buffer curto */
#define PVM_ERR_MAGIC   (-2)    /* assinatura PVMH ausente */
#define PVM_ERR_HEADER  (-3)    /* header_size invalido */
#define PVM_ERR_COUNT   (-4)    /* count invalido (0 ou > PVM_MAX_TEXTURES) */
#define PVM_ERR_CHUNK   (-5)    /* chunk truncado ou fora do buffer */
#define PVM_ERR_PVRT    (-6)    /* assinatura PVRT ausente no chunk */
#define PVM_ERR_DECODE  (-7)    /* pvr_decode_rgba8888 falhou */
#define PVM_ERR_SPACE   (-8)    /* destino pequeno para o conjunto */

/* Valida o PVMH e percorre os chunks por offset/size. Preenche `out` (o
 * buffer `buf` deve continuar vivo) e retorna o numero de texturas (>=1),
 * ou negativo em caso de entrada malformada. Em erro, buf/len/flags/count
 * ficam zerados; tex nao e valido sem count positivo. */
int pvm_parse(const unsigned char *buf, int len, Pvm *out);

/* Decodifica todas as texturas para RGBA8888, empacotadas em sequencia em
 * `dst` (precisa caber sum(w*h*4)). Se `offsets` nao for NULL, recebe o
 * offset de cada imagem decodificada (pvm->count inteiros). Retorna o total
 * de bytes escritos, ou negativo em erro. dst_cap deve ser positivo.
 *
 * Contrato de falha (sem saida parcial): o retorno so e valido se for o total
 * completo. A funcao primeiro faz um preflight de dimensoes e da capacidade
 * acumulada, sem escrever em `dst`; nesse estagio, qualquer erro (dimensao
 * invalida -> PVM_ERR_DECODE, ou soma > dst_cap -> PVM_ERR_SPACE) deixa
 * `offsets[0..count-1]` = -1 e `dst` intocado. Se o preflight passa mas um
 * decoder falha no meio, a funcao apaga exatamente o prefixo ja tentado em
 * `dst` (sem ultrapassar dst_cap) e novamente seta todos os offsets contados
 * para -1, de modo que nenhuma imagem parcial possa ser apresentada como
 * valida. Excecao: se `pvm` for NULL ou `pvm->count` for invalido, nao ha
 * count confiavel e `offsets` e deixado intacto. Em qualquer caso, dst_cap e
 * respeitado e o trabalho e limitado por PVM_MAX_TEXTURES.
 * Formatos desconhecidos falham no decoder. */
int pvm_decode_all(const Pvm *pvm, unsigned char *dst, int dst_cap,
                   int *offsets);

#endif /* SA1_PVM_H */
