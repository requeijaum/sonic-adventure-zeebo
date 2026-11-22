# Roadmap: do estado atual até o título e o primeiro stage

Complementa `zeebo-port-roadmap.md` (estado, evidências, riscos). Este arquivo
é a **ordem de execução**: fases, entregas, gates e dependências. Estimativas
são de esforço de trabalho focado, não de calendário, e valem só como ordem de
grandeza (o que não foi medido está marcado).

Estado (2026-11-22, manhã): o jogo (1ST_READ do decomp em ARM32) roda no
zeebx v0.6.0 dentro de um `.mod` de ~20 MB com o applet e dois programas
(ADVERTISE, SUMMARY). Caminho jogável com o teclado: logo da SEGA (com fade)
→ aviso do cartão de memória → **tela de título** → menu principal (só o
fundo: 3D ausente) → resumo da história → entra em Station Square
(`game_mode` 5, texturas `SS_*` carregadas) e cai em `camera_change_act`
lendo dados do campo (`adv00`, programa ainda não carregado). Sem som, sem
filmes; o 3D ainda não aparece (ver atualização abaixo). Validador: zeebx v0.6.0 da tag upstream
(`~/projects/zeebx-v060`, branch `sa1-port-0.6.0` no fork, com `run --window`
traduzindo o controle em teclas e `ZEEBX_PC_SAMPLE`).

Como jogar: `ports/zeebo/play.sh` (ou `--build`). Z = A (e Start), X ou
Espaço = B, C = X, V = Y, Enter = Start, setas = direcional, Esc fecha.

### Atualização 2026-11-22 — 3D (Fase C)

Causa raiz do 3D quebrado (2026-11-22 01:41): os kernels de vértice de
`sdk/ninja/njModel.c` (`CalcModelMatrix`, `loop1..9_body`, `submit_packet`, ...)
só existem em asm SH-4 dentro de `#ifdef DREAMCAST`; no build ARM caíam em
`ztraps.c` (no-op), gerando vértice lixo ("Sonic desmontado"), sem luz e sem
bits de clip/cull. Confirmado no objeto: `njModel.o` pede `loop9_body` e
`ztraps.o` define.

Feito: kernels do `njModel.c` em C (`ports/zeebo/ninja/njmodel_arm.c`,
`3516b5f`; deslocamento pela normal do modelo, `414949a`; harness contra o asm,
`3c8cc65`) e as 23 calculadoras de luz do `njLtSrcs.c` + `lt_power` + fila em
`ports/zeebo/ninja/lt_arm.c` (`5d536c7`; `njSetLightMaterials` e
`njCalcLightMatrices`, `492168c`; stubs vazios removidos de `zlive.c`).

Aberto: a luz nova linka (503/503 objetos, 0 duplicatas) mas o quadro sai
idêntico (hash `b2b261a0fc07`) — a contribuição de luz ainda não chega ao pelo
(azul-marinho chapado). `draw_basic` retorna cedo (`points`/`meshsets` nulos ou
`cull`), então o log de `draw_meshset`/`njEndVertex` não sai. A pose do Sonic no
menu ("deitado e grande") tem matriz ortogonal válida e escala coerente com a
câmera do jogo (`dist`=457, FOV 70°), mas falta um print do original para
decidir se é a pose do jogo ou bug do port. `src/draw/model.c` /
`engine/dc/sa1/model.c` (`engine/dc/batch.h` é asm SH-4) segue pendente.

### Quick wins (2026-11-21, noite) — feitos

| # | Entrega | Commit |
|---|---|---|
| Q1 | `play.sh`: abre o jogo na janela, log em `~/.cache/sa1z-check/janela.log` | `9d6bea1` |
| Q2 | `CHECK_GAME=1 check.sh` aperta o botão 1 e exige o menu (`game_mode` 11) e a seleção do cartão | `9d6bea1`, `36e1859` |
| Q3 | Programa de campo/stage ausente → o jogo recomeça do logo (`zgame_reset`), sem cair | `d313111` |
| Q4 | "2D some no fundo azul": o menu principal desenha certo; o céu sem janelas é a transição da seleção de arquivo (o pedaço que falta lá é 3D, os personagens) | investigado |
| Q5 | Pad completo na janela (botões 3, 4 e HOME no zeebx `sa1-port-0.6.0`) | `de7e6fe` |
| Q6 | Música: `stream.c` → `zADXT_*` → ADX decodificado e tocado pelo IMedia (até 40 s, sem laço) | `a273b75` |
| Q7 | Dados dos programas linkados voltam ao estado inicial a cada leitura (`zprog_reset`) | `f049e06` |
| Q8 | Cartão de memória emulado em `sa1vmu.bin` (some o aviso; aparece a seleção de arquivo). O save ainda não foi exercitado: o jogo só grava dentro do campo | `cdd0c24` |

### Quick wins candidatos (2026-11-22, reavaliados) — abertos

Esforço: P ≤ 1 h, M ≤ meio dia. Ordem sugerida: Q17 → Q9 → Q10/Q11 → Q12 → Q13.
Hipótese principal: `zlive.c` tem luz própria (Lambert com `draw_set_light`,
`zlive.c:10-12`); se o Sonic do menu vai por `sa_present_live`, a luz de
`lt_arm.c` não entra nesse caminho (daí o quadro idêntico).

| # | Entrega | Esforço | Por quê / evidência |
|---|---|---|---|
| Q9 | Contar por qual caminho o Sonic é desenhado: `sa_present_live` (zlive) × `njModel` | P | a luz nova não mudou o quadro (`b2b261a0fc07`) |
| Q10 | Logar qual early-return de `draw_basic` dispara (`points`/`meshsets` nulos × `model_in_view`) | P | `zlive.c:241-246`; `draw_meshset` nunca loga |
| Q11 | `model_in_view` usa `njCalcPoint(NULL, center)`: a matriz corrente já é a do modelo? | P | `zlive.c:228-235` |
| Q12 | Log de 1 vértice do Sonic: RGB final e `_nj_light_queue_.count` | P | separa "luz zerada" de "luz não chamada" |
| Q13 | Ligar `njCalculateLightsRGB` ao `zlive` quando houver luzes na fila | M | depende de Q9/Q12 |
| Q14 | Clip no near em vez de descartar o triângulo | M | `zlive.c:11-12` |
| Q15 | Backface culling com `glCullFace` | P | `zlive.c:12`; `kmSetCullingRegister` stub (`zkamui.c:51`) |
| Q16 | Paleta do SA1 no `zlive` (`kmSetPaletteMode` stub) | M | `zlive.c:10`, `zkamui.c:52` |
| Q17 | `tools/frame_hash.sh`: run curto, 1 quadro, hash | P | o run de 62 s / 563 BMPs coincidiu com o travamento |
| Q18 | `ulimit -v` + `timeout` + `nice` em todo run do zeebx | P | RAM de 9,6 GB |
| Q19 | Limpar `~/.cache/sa1z-check/glc*` ao fim de cada run | P | disco em 98% |
| Q20 | `VACUUM` do banco do opencode (3,9 GB) com ele fechado | P | I/O na mesma máquina |
| Q21 | `CHECK_GAME=1 check.sh` como gate de commit no 3D | P | `check.sh:50-62` |
| Q22 | `kmSetUserClipping` → `glScissor` | P | `zkamui.c:54` |
| Q23 | `kmSetFog*` → `GL_FOG` | P–M | `zkamui.c:55-57` |
| Q24 | `kmCreateTextureSurface` devolve a textura GL real | P | `ztexture.c:90` |
| Q25 | `cache_ram_init` real (armadilha no log da janela) | P | `janela.log` |
| Q26 | Música em laço | P–M | D4/Q6 "sem laço" |
| Q27 | `pad_jump_buttons_signed` em C | P | `zshinobi.c:71` |
| Q28 | Triar os 963 indefinidos de `uu.txt` por frequência no run | M | armadilhas escondem comportamento |
| — | Referência da pose do menu (print do original) | — | decide se "deitado e grande" é bug |

## Fase A — Linkar o jogo com o applet (sem executar)

Objetivo: um `.mod` único que contém o jogo e o applet e linka sem símbolos
indefinidos. Estado (2026-11-21): **0 duplicatas**, 939 símbolos únicos
indefinidos (`make -f game.mk integrate`, lista em `build/zeebo/game/uu.txt`).

| # | Entrega | Estado |
|---|---|---|
| A0 | Simulador BREW 4.0.2 sob Wine (x86/DLL) | adiado: validar só no zeebx |
| A1 | Duplicatas entre jogo e applet | **feito**: `tools/integrate.py` enfraquece 48 símbolos no jogo (o applet é dono do sink, dos shims do Ninja e da string); ADX e `port_shim` saem do applet |
| A2 | Kamui | **feito**: `*_kamui_lib.c` fora; `zkamui.c` com 40 stubs (fog, clipping e render-para-textura marcados TODO) |
| A3 | libc e GDFS | **feito**: `zlibc.c` (malloc/free no BREW, stdio descartado), `zgdfs.c` completo para o loader; `pads/settings` do slice fora |
| A4 | `bu*`, `fm*`, `kb*`, `pd*`, `sd*`, `sy*` | **feito como stubs** (`tools/gen_stubs.py` gera 86 a partir dos headers; `zshinobi.c`: heap do BREW, cache no-op, dados). Comportamento real: save na Fase E, som na D4 |
| A5 | Driver de som do DC (`sd*`) | stubs que devolvem 0 (sem som); implementação sobre IMedia/ADX na D4 |
| A6 | Modelo 3D (`draw_*`, `loop*_body`, `NJM_SET_*_STRIP`, `pack/unpack_argb_body`, `njEasy*`, `_nj_light_*`, `_njm_col*`, `njCalcLightMatrices`) | Fase C |
| A7 | `overlay_*`, `sa_present_*`, `uf*`, `qsort`, `fill_bytes`, `port_stop`, `sprite_*` | **feito** em `zshinobi.c` (`sprite_draw_3d`, `ufPlay*` e `pad_jump_buttons_signed` são TODO) |
| A8 | Programas: `native_programs`, `native_program_count`, `loader_entry`, `movie_entry`, tabelas `*_land/_action/_object_list/...` (dados dos outros programas) | Fase D |
| A9 | Nomes por endereço do `symbols.txt` e `_BSG_END` | `_BSG_END` feito; o resto vira armadilha (`tools/gen_traps.py`: 87 funções logam o nome, 839 dados viram zeros) |
| A10 | Ligar `zthreads.c` ao `thread.c` (pontas fracas) | pendente |

Gate da fase: `integrate` com 0 duplicatas e 0 indefinidos fora das
tabelas de outros programas, e `check.sh` verde. **Estado:** 0 duplicatas; os
indefinidos restantes viram armadilhas no `.mod` de teste (não é o gate final).

## Fase B — Primeira execução do jogo (boot) — FEITA

Como rodar: `make` (applet), `make -f game.mk mod` (jogo + applet em
`build/zeebo/sa1g.mod`), disco por `tools/stage_disc.sh <raiz extraída>
build/zeebo` (hard links, nomes minúsculos sem `;1`; o zeebx v0.6.0 ignora
links simbólicos). Disco JP extraído com `gdrom_extract.py` (2106 arquivos,
973 MB, fora do git). Rodar com `~/projects/zeebx-v060/target/release/zeebx
run build/zeebo/sa1g.mod --seconds=N`.

Caminho percorrido: `sbinit`, `njInitSystem` (`zninjasys.c`) + `njInitGlobal`
do `njSystem.c` real, `masaru.drv` por `gdFsOpen`, som (stubs), `njInitPrint`,
`display_reset_full`, `vmu_init`, `card_init`, `game_mode` 0 → 1 (LOGO) → 8
(MOVIE) → menus (para: ADVERTISE não carregado).

O que precisou mudar (ordem em que apareceu):
- `native_asm_init()` antes do jogo (tabelas de dados do `asm.c`).
- Tabelas `_nj_*`/`kmi*` em zeros (`zshinobi.c`, como `stubs.txt` do decomp).
- `njSystem.c` real no link com as funções de vídeo do DC enfraquecidas em
  favor de `zninjasys.c`; `njInitGlobal` chamado por ele.
- Esperas de vsync (`frame.c`, `njInitVertexBuffer`, `waitVsync`) chamam
  `native_vsync()` (SITES do decomp, via `gen_patch.py`); `syChainAddHandler`
  real, rodado a cada vsync.
- TMU (`src/system/timer.c`) emulado sobre `syTmrGetCount`.
- Nomes sem definição viram armadilhas (`gen_traps.py`).
- `movie_entry` devolve 1: a abertura é pulada (vai aos menus).

Armadilhas de depuração encontradas: com `.SECONDARY` no `Makefile` do applet,
apagar um `.o` não força recompilar (use `touch` no fonte); um `.o` velho em
`build/zeebo/` entra no link do jogo pelo wildcard (`APPLET_OBJS`). Foi isso
que produziu um falso "232 quadros" anterior.

Depuração: `zlog()` (texto montado à mão; 150 linhas, `jsr` 40, `m*`/`v*` sem
cota) e `HOT` para laços de espera. Logs temporários: `main` (`src/main.c`),
`modo` (`zthreads.c`), `vsyncset/vsyncfn` (`zninjasys.c`),
`tree_list_nodes` (`util.c`), `jsr` (`asm.c`).

| # | Entrega | Estado |
|---|---|---|
| B1 | `zgame_start(main)` no start e `zgame_step()` por frame | **feito** |
| B2 | Log por chamada de shim | **feito** (`zlog`) |
| B3 | Corrigir falhas em ordem até o laço principal | **feito**: BOOT → LOGO → MOVIE |
| B4 | Medir uso real de pilha por thread | **feito** para a corrotina do jogo (marca d'água: 16,7 KB de 128 KB até o MOVIE; logada); threads do jogo: pendente (A10) |
| B5 | Remover/limitar logs temporários | **feito** (ficam `modo`, pilha, armadilhas, `HOT` a 3000 chamadas) |

## Fase C — Desenho (2D e modelos 3D)

Decisão (2026-11-21): **opção E** — o desenho do jogo compila no sabor
Dreamcast (`-DDREAMCAST`, `DC_SET` em `game.mk`) e chega ao sink GLES pelo
`ninja/sa1/strip.h`; os pontos quentes do 3D vão para T&L na GPU. Motivo:
medição de `zbench.c` (float soft ~680 instruções por vértice, ponto fixo
~15) e o Developer Guide §3.2.2 (sem VFP; `--cpu ARM1136J-S` no SDK).

| # | Entrega | Estado |
|---|---|---|
| C0 | 2D no sabor DC: `njDraw*`, `njDrawLine2D/3D`, `njDrawPolygon2D`, `njDebug`, `engine/dc/sprite.c` | **feito** (compila e desenha: 1208 draws) |
| C1 | Texturas do jogo: `kmCreateTextureSurface`/`kmLoadTexture` → GL; `njSetkmTextureNumG` → textura certa | **feito** (`ztexture.c`; 64 texturas; o logo "PRESENTED BY SEGA" aparece no zeebx). Lista translúcida ordenada por profundidade no fim do quadro (`zstrip_flush`): o fade do logo entra e sai |
| C2 | `src/draw/model.c` + `engine/dc/sa1/model.c`: `engine/dc/batch.h` é asm do SH-4 (projeção e luz por vértice) | **parcial**: kernels do `sdk/ninja/njModel.c` em C (`njmodel_arm.c`, `3516b5f`, `414949a`, `3c8cc65`); falta o caminho `src/draw/model.c`/`engine/dc/sa1/model.c` |
| C3 | Um modelo real com T&L de hardware e matriz do Ninja em 16.16 | pendente: o Sonic aparece "deitado e grande" — matriz ortogonal válida e escala coerente com a câmera; falta referência da pose original |
| C4 | Iluminação mínima (cor por vértice + luz direcional) | **parcial**: 23 calculadoras + `njSetLightMaterials`/`njCalcLightMatrices` em C; linka (0 duplicatas), mas o quadro sai idêntico — a luz ainda não chega ao pelo |
| C5 | `njEasy*`/`njModelE.c` (falha: backend SH-4 do SH4ZAM) | pendente |

## Fase D — Título e menus (gate do M7)

| # | Entrega | Gate |
|---|---|---|
| D1a | ADVERTISE (menus) e SUMMARY (resumo) linkados no `.mod` (`game.mk PROGRAMS`; main.c põe a tabela ARM em MODULE_AREA) | **feito**: título, menu principal, resumo |
| D1 | Carga de programas: `content.c` de cada programa, `gdFsRead` nas áreas `ZDC`, relocação dos ponteiros internos (`engine/native/loads.c` como referência) | o loader troca de programa sem corromper a arena |
| D2 | Texturas do menu (depende de C1) | **feito** (fundo e janelas do menu) |
| D3 | Pad do jogo | **feito** parcial: `support = PDD_DEV_CONTROLLER`, botão 1 (`AVK_CONFIRM`) = A+Start, `AVK_CLR` = B, setas; X/Y e analógico não |
| D4 | Áudio do jogo (`sd_*`/ADX) pelo IMedia, em streaming | **música feita** (Q6, sem laço e até 40 s); efeitos e vozes pendentes |
| D5 | Decisão sobre o passe de floats (`tools/floats.py`, `-double=float`) | comparação de uma função de física com o host |

Gate: **título + menu navegável no zeebx lendo assets pelo BREW FS** (gate
original do M7). Estado: menus e título feitos (sem 3D, sem som). Próximo bloqueio: os
programas de campo e stage (`adv00`... para Station Square). Linkar todos não
cabe (~250 programas; débito técnico em `zeebo-port-roadmap.md`): carga
dinâmica, ou linkar só os do caminho escolhido como prova.

Travas encontradas e corrigidas no caminho: `buInit` sem a função de início
(o menu esperava), pad com suporte 0 (o jogo ignorava a porta), flash do
console (`fmMng*`) que nunca terminava (`src/save/mail.c`), `menus_set_*` (do
`trial.c`).

Problemas abertos vistos jogando: elementos 2D somem sobre o fundo azul do
menu (não investigado: pode ser 2D atrelado ao 3D que falta, ou a ordem dos
translúcidos/profundidade).

## Fase E — Primeiro stage (M8)

| # | Entrega | Gate |
|---|---|---|
| E1 | Emerald Coast: land, objetos, texturas, HUD sob demanda | stage carrega |
| E2 | Câmera, colisão (`polygon.c`/`mesh.c` já compilam), física do Sonic | mover o personagem |
| E3 | Save em arquivo (`bu*`/`save_*` -> IFileMgr) | **feito** (Q8); falta exercitar o save no campo |
| E4 | Filmes (M6): SFD decodificado e apresentado como textura | uma cutscene por N s |

## Fase F — Desempenho, pacote e validação cruzada (M9, M10)

| # | Entrega | Gate |
|---|---|---|
| F1 | Medir custo no zeebx (contagem de instruções/chamadas por quadro, `--perfil`) e, se o GEC der tempo, nele; perfil por função (soft-float) | tabela de custos relativos |
| F2 | Reduzir CPU: cull, listas de draw, VBOs, menos matrizes por modelo | ~35 fps no stage |
| F3 | Pacote: `.mod` + `.mif` + pacote de assets por stage (sem GD-ROM) | instalável |
| F4 | Validar no GEC (simulador oficial): carrega o `.mod`, caminho dos arquivos, heap reportado | título + stage em execução contínua no zeebx **e** no GEC |

## Dependências

```
A (link) -> B (boot) -> C (modelos) -> D (título) -> E (stage) -> F (perf/GEC)
                \-> M6 (SFD) em paralelo, entra no E4
```

A e B são sequenciais e curtas em relação a C. C é o caminho crítico.

## Alvos de validação (sem hardware por ora)

- **zeebx** (primário): roda em CI local (`check.sh`), dumps de GL/áudio,
  `--teclas`, `--trace`, `--perfil`. Mapa de memória e limites são escolhas do
  emulador.
- **BREW Simulator** (secundário, oficial, via Wine): **não** executa o `.mod`
  ARM; carrega um build **x86 (DLL)** do app, com o device pack do Zeebo e a
  extensão OpenGL ES. Serve para achar bugs de lógica (não de ABI ARM, soft-float,
  alinhamento). O `Simulator.exe` do `Toolset 7.10` e o `a1Sim.exe` do BREW MP
  SDK **não são** este: são do BREW MP 7.x (outra plataforma que o BREW 4.0.2 do
  Zeebo). O GEC não foi localizado. Sem um alvo x86 do applet, o único
  validador é o zeebx.
- Sem console: nada do que depende de hardware (tempo real, limite de `.mod`,
  memória física) é afirmado como verdadeiro; entra como risco com margem
  conservadora. Um resultado só vale como "funciona" se o zeebx o mostra; o simulador x86, se existir, entra como segunda opinião da lógica.

## Rotina de trabalho

- Um commit por incremento verificável; `ports/zeebo/check.sh` verde antes do
  commit (com `CHECK_GAME=1` quando mexer no `game.mk`).
- Push automático ao fork a cada 4 h (`~/.local/bin/zeebo-push.sh`).
- Antes de dizer "verificado": o build mostra `Created` e o teste falha quando
  forçado a falhar.
- Nada de assets, ROM ou `1ST_READ.BIN` no git.

## Incertezas que mudam o plano (ainda não medidas)

1. Tamanho máximo de `.mod` e de bss (hoje ~20 MB com o jogo): sem hardware, só
   o GEC pode dizer algo; até lá, tratar como orçamento a reduzir.
2. Heap de um app (no zeebx é 64 MB, escolha do emulador; o GEC pode reportar outro).
3. Custo do soft-float no ARM11 por quadro de stage (medido por vértice: ~45x
   o ponto fixo; falta o quadro inteiro).
4. Se o jogo exige o passe de floats do SHC para a física ficar fiel.
5. Se a carga de programas por relocação dos ponteiros é suficiente ou se o
   `content.c` de cada programa precisa ser linkado em endereços próprios.
