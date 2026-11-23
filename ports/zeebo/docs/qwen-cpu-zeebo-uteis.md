# Sessão Qwen "CPU Zeebo sem VFP" — o que presta, conferido

Fonte: `qwen-cpu-zeebo-sem-vfp.json` (export do Open WebUI, 23/11/2026) e
`qwen-cpu-zeebo-sessao.md` (as 15 respostas na íntegra). O modelo (Qwen3.7-Plus,
3.8-Max e 4-Max) disse em vários turnos que estava sem busca na web, e na
auto-auditoria do turno 14 admitiu 7 erros. Cada item abaixo foi conferido contra
o repositório ou o Zeebo Developer Guide 0.97
(`zeebo-emulator/research/docs/sdk-extract/ZeeboDeveloperGuide0.97.md`).

Legenda: **CONFIRMADO** (visto no repo/Guide), **JÁ FEITO**, **AÇÃO**, **HIPÓTESE**
(plausível, não verificado), **DESCARTAR** (errado ou não se aplica).

## 1. Achados reais, com ação

| Item | Status | Evidência | Ação |
| --- | --- | --- | --- |
| O save do Zeebo tem limite de 64 KB; nossa imagem VMU tem 200 blocos × 512 B = **102 400 B** | **CONFIRMADO, risco real** | Guide §5.1: "at most 64KB for save game data"; `zshinobi.c:210-212` (`VMU_BLOCK 512`, `VMU_BLOCKS 200`) | gravar só os blocos usados (o save do SA1 ocupa poucos) ou reduzir `VMU_BLOCKS`; testar o limite no aparelho |
| Não havia gate automático contra instrução VFP no `.elf` | **AÇÃO FEITA** | nenhum `vfp` em `check.sh`/Makefile | `tools/novfp.sh` (falha se achar mnemônico VFP ou CP10/CP11) |
| Suspend/Resume exige salvar e recriar o estado GL | **CONFIRMADO** | Guide §10.1 ("state (including OpenGL ES states) would have to be saved"); `app.c:136,147` trata os eventos | conferir se o resume recria as texturas de `zstrip` (tabela GL) |
| Faixa do `GL_FIXED` (Q16.16, ±32767,99998) | **HIPÓTESE, vale auditar** | nós cacheamos pontos em `GL_FIXED` por modelo | medir o maior \|coordenada\| dos modelos e da geometria de fase |
| Play math × presentation: matemática que só gera pixel pode sair do `fast.c` bit-exato | **HIPÓTESE** | regra do `ENGINE.md` citada pelo modelo | auditar `njDrawObject`/projeção; ganho nos ~80 ms de game code não medido |
| `SMLAD`/`SMUAD` (SIMD do ARMv6) nos laços de `njMultiMatrix`/`FIPR` | **HIPÓTESE** | o modelo avisa de overflow com Q15 sem normalizar | só com `host-selftest` + fuzz como rede |
| O relógio do jogo, não o do áudio, decide a jogabilidade | **HIPÓTESE** | `ENGINE.md` (via modelo) | manter isso ao refazer `zaudio.c` em streaming (QW18 do roadmap) |
| Perfil preditivo: modelo de ciclos do ARM1136 dentro do zeebx | **HIPÓTESE** | ideia do modelo | só vale se o zeebx ganhar contadores por função |
| 3D em baixa resolução + HUD nativo | **HIPÓTESE** | GLES 1.1 sem FBO; o caminho seria viewport + `glCopyTexSubImage2D` + quad | CPU é o gargalo, não o fillrate: ganho só no fill |

## 2. Já estava certo no port

Conferido em `game.mk`/`Makefile`: `-mcpu=arm1136j-s -mfloat-abi=soft -marm`,
`-ffp-contract=off`, `-fno-strict-aliasing`, `-fwrapv`, `-O3 -fno-tree-vectorize`.
Não há `-ffast-math`, `-march=armv6k` nem `-mfpu=softvfp` (os erros 1, 5 e 9 da
auto-auditoria do modelo não se aplicam a nós). O `kmFreeTexture`, o `alpha_bleed`
e o T&L por `GL_FIXED` o próprio modelo reconheceu como já implementados.

## 3. Descartar (o modelo errou ou não se aplica)

- **Swizzle/Morton offline de texturas:** o GLES recebe dado linear no `glTexImage2D`; o driver decide o layout.
- **"ARM1136JF-S com o F desativado no fusemap":** invenção do modelo, ele próprio retratou. O fato: sem VFP.
- **ARM11 dual-issue:** errado, é single-issue de 8 estágios.
- **`-ffast-math` no play math:** quebra demos e decisões de jogo.
- **`-march=armv6k`, `-mfpu=softvfp`:** o primeiro habilita instrução que o núcleo não tem; o segundo é sintaxe do armcc.
- **`GET_TIME_US`:** não existe; o BREW tem `GETUPTIMEMS` (já usado em `app.c`).
- **IMEM como scratchpad da CPU:** a IMEM de 256 KB (128+64+64) é da GPU (Z-buffer, stencil, pixel buffers; Guide §3.1.1), não da CPU.
- **Rótulos "fast = 32 MB / slow = 128 MB":** o modelo admitiu ser hipótese. Qual pool atende o heap é desconhecido.
- **Tamanhos de cache (16 KB), tiling da Adreno 130, throttling térmico:** não verificados.

## 4. Não confirmado no Guide (nada de ISQRT/FADD/FMUL)

O modelo citou `FADD/FMUL/ISQRT` do `AEEStdLib.h` (soft-float otimizado da Qualcomm).
Buscas por `ISQRT`, `FADD`, `FMUL` e `FSQRT` no Guide e em `sdk/include` não acharam nada.
Confirmar nos headers do SDK BREW antes de usar. Idem para `IHEAP_GetMemStats` e
`ISHELL_Resume` (não aparecem no Guide).

## 5. Leituras sugeridas pelo modelo (não verifiquei se existem)

- ARM1136 TRM (DDI 0211): caches, `PLD`, latências, pipeline.
- ARM Architecture Reference Manual, capítulo ARMv6: `SMUAD`, `SSAT`, flag Q.
- ARM Application Note 34, "Writing Efficient C for ARM".
- Drepper, "What Every Programmer Should Know About Memory".
- Agner Fog, "Optimizing software in C++".
- Adreno OpenGL ES Developer Guide (a versão que circula pode ser de GPU mais nova que a 130).
- Para o Zeebo, a leitura mais barata continua sendo reler o Developer Guide inteiro (orçamento de heap, 64 KB de save, áudio).

## 6. Ligação com o roadmap

QW18 (áudio em segmentos) e QW20 (RAM do `.mod`) ganham os itens de áudio e RAM
acima. Novos candidatos: **save de 64 KB**, **recriar texturas no resume**,
**auditoria do alcance do `GL_FIXED`** e o **gate `novfp`**.
