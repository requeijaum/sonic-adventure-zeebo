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
| O save do Zeebo tem limite de 64 KB; a imagem VMU em RAM tem 200 blocos × 512 B = 102 400 B | **CONFIRMADO o limite; o risco é pequeno** | Guide §5.1: "at most 64KB for save game data". Mas `zshinobi.c:302-303` grava só o cabeçalho + os blocos usados (`sizeof vmu - sizeof vmu.data + vmu_used() * VMU_BLOCK`), não os 200 | medir o tamanho real do `sa1vmu.bin` depois de um save completo; só ameaça o limite se o save ocupar mais de ~120 blocos |
| Não havia gate automático contra instrução VFP no `.elf` | **AÇÃO FEITA** | nenhum `vfp` em `check.sh`/Makefile | `tools/novfp.sh` (falha se achar mnemônico VFP ou CP10/CP11) |
| Suspend/Resume exige salvar e recriar o estado GL | **CONFIRMADO** | Guide §10.1 ("state (including OpenGL ES states) would have to be saved"); `app.c:136,147` trata os eventos | conferir se o resume recria as texturas de `zstrip` (tabela GL) |
| Faixa do `GL_FIXED` (Q16.16, ±32767,99998) | **HIPÓTESE, vale auditar** | nós cacheamos pontos em `GL_FIXED` por modelo | medir o maior \|coordenada\| dos modelos e da geometria de fase |
| Play math × presentation: matemática que só gera pixel pode sair do `fast.c` bit-exato | **HIPÓTESE** | regra do `ENGINE.md` citada pelo modelo | auditar `njDrawObject`/projeção; ganho nos ~80 ms de game code não medido |
| `SMLAD`/`SMUAD` (SIMD do ARMv6) nos laços de `njMultiMatrix`/`FIPR` | **HIPÓTESE** | o modelo avisa de overflow com Q15 sem normalizar | só com `host-selftest` + fuzz como rede |
| O relógio do jogo, não o do áudio, decide a jogabilidade | **HIPÓTESE** | `ENGINE.md` (via modelo) | manter isso ao refazer `zaudio.c` em streaming (QW18 do roadmap) |
| Perfil preditivo: modelo de ciclos do ARM1136 dentro do zeebx | **HIPÓTESE** | ideia do modelo | só vale se o zeebx ganhar contadores por função |
| 3D em baixa resolução + HUD nativo | **HIPÓTESE** | GLES 1.1 sem FBO; o caminho seria viewport + `glCopyTexSubImage2D` + quad | CPU é o gargalo, não o fillrate: ganho só no fill |

## 2. Já estava certo no port

Conferido em `game.mk`/`Makefile`: `-mcpu=arm1136j-s -mfloat-abi=soft` (o `-marm` só aparece no `.o` de traps, `game.mk:369`),
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

## 4. Funções do BREW citadas pelo modelo (conferidas no SDK BREW 4.0.2 SP19)

Headers em `zeebo-emulator/research/docs/sdk-extract/BREW-4.0.2-SP19/sdk/inc/`
(a primeira versão deste digest procurou só em `sdk/include` do port e no Guide, e
concluiu errado que não existiam).

- `FADD`, `FSUB`, `FMUL`, `FDIV`, `FSQRT`, `FLOATTOWSTR`: **existem** em `AEEStdLib.h` (macros que chamam `GET_HELPER()->f_op/f_calc`, isto é, via tabela de helpers do sistema). Que sejam "assembly otimizado da Qualcomm" e mais rápidos que o `__aeabi_*` do libgcc **não está verificado**: medir antes de trocar.
- `ISQRT`: **não existe** (o que há é `FSQRT`, de ponto flutuante).
- `IHEAP_GetMemStats(p)`: existe em `AEEHeap.h`, mas devolve **um** `uint32` (não total/livre/maior bloco como o modelo disse).
- `ISHELL_Resume`: existe (`AEEShell.h`, `AEESource.h`).
- `GETUPTIMEMS`: existe (e o port já o usa em `app.c`).

## 4b. Itens do modelo que o primeiro digest omitiu

- **Watchdog × frame de 80 ms** (auto-auditoria, item 12): medir o pior frame; o BREW pode matar o app que segura o laço de eventos. `ISHELL_Resume` existe (acima) para fatiar o carregamento.
- **Ponteiros de função e jump tables em dados no rebase/ZDC** (item 15): o `_njloadtexture` (tabela de funções) funciona no port, mas vale auditar as outras tabelas.
- **`int64 /` é caro** (`__aeabi_ldivmod`): evitar em laço quente.
- **Probe de extensões GL** (`glGetString(GL_EXTENSIONS)`, ~5 min): procurar `ATITC`/`ATC`, `ETC1` e paletizada. O Guide já cita ATITC (4 bpp opaco, 8 bpp com alfa), então a ideia de RAM de textura comprimida é real.
- **Demoscene** (turno 9): o túnel por matriz de textura (um quad, `glLoadMatrixx`, scroll) custa quase zero de CPU. Só se aplica a efeitos novos, e o roadmap atual proíbe features.
- **Divisão por constante** (Granlund-Montgomery) e Hacker's Delight (turno 12) para os ~80 ms de game code: ainda não avaliado.
- **Licões de console** (turno 13: GTE do PS1, IWRAM do GBA, ARM7 de áudio do NDS): contexto, sem ação direta.

## 5. Leituras sugeridas pelo modelo (não verifiquei se existem)

- ARM1136 TRM (DDI 0211): caches, `PLD`, latências, pipeline.
- ARM Architecture Reference Manual, capítulo ARMv6: `SMUAD`, `SSAT`, flag Q.
- ARM Application Note 34, "Writing Efficient C for ARM".
- Drepper, "What Every Programmer Should Know About Memory".
- Agner Fog, "Optimizing software in C++".
- Adreno OpenGL ES Developer Guide (a versão que circula pode ser de GPU mais nova que a 130).
- GBATEK (GBA/NDS) e a especificação do PS1 do nocash (GTE) também foram sugeridos. Para o Zeebo, a leitura mais barata continua sendo reler o Developer Guide inteiro (orçamento de heap, 64 KB de save, áudio).

## 6. Ligação com o roadmap

QW18 (áudio em segmentos) e QW20 (RAM do `.mod`) ganham os itens de áudio e RAM
acima. Novos candidatos: **medir o `sa1vmu.bin` real** (limite de 64 KB), **recriar
texturas no resume**, **auditoria do alcance do `GL_FIXED`**, **probe de extensões GL**
e ligar o **gate `novfp`** ao `make check` (o script existe, falta ligar).
