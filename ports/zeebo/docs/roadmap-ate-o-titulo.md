# Roadmap: do estado atual até o título e o primeiro stage

Complementa `zeebo-port-roadmap.md` (estado, evidências, riscos). Este arquivo
é a **ordem de execução**: fases, entregas, gates e dependências. Estimativas
são de esforço de trabalho focado, não de calendário, e valem só como ordem de
grandeza (o que não foi medido está marcado).

Estado (2026-11-21, fim do dia): o jogo (1ST_READ do decomp em ARM32) está
linkado com o applet num `.mod` (`make -f game.mk mod`, ~20 MB) e roda no
zeebx v0.6.0: BOOT → LOGO → MOVIE, ~16 quadros/s de jogo, 1208 `glDrawArrays`
em 12 s, **tela preta** (texturas do jogo ainda são stubs). Para no menu: o
ADVERTISE não é carregado (fase D). Validador: zeebx v0.6.0 compilado da tag
upstream (`~/projects/zeebx-v060`, branch `sa1-port-0.6.0`).

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
| B4 | Medir uso real de pilha por thread | pendente |
| B5 | Remover/limitar logs temporários | pendente |

## Fase C — Desenho (2D e modelos 3D)

Decisão (2026-11-21): **opção E** — o desenho do jogo compila no sabor
Dreamcast (`-DDREAMCAST`, `DC_SET` em `game.mk`) e chega ao sink GLES pelo
`ninja/sa1/strip.h`; os pontos quentes do 3D vão para T&L na GPU. Motivo:
medição de `zbench.c` (float soft ~680 instruções por vértice, ponto fixo
~15) e o Developer Guide §3.2.2 (sem VFP; `--cpu ARM1136J-S` no SDK).

| # | Entrega | Estado |
|---|---|---|
| C0 | 2D no sabor DC: `njDraw*`, `njDrawLine2D/3D`, `njDrawPolygon2D`, `njDebug`, `engine/dc/sprite.c` | **feito** (compila e desenha: 1208 draws) |
| C1 | Texturas do jogo: `kmCreateTextureSurface`/`kmLoadTexture` → GL; `njSetkmTextureNumG` → textura certa | pendente (próximo) |
| C2 | `src/draw/model.c` + `engine/dc/sa1/model.c`: `engine/dc/batch.h` é asm do SH-4 (projeção e luz por vértice) | pendente: reescrever os kernels em C (ponto fixo) ou trocar por T&L na GPU |
| C3 | Um modelo real com T&L de hardware e matriz do Ninja em 16.16 | pendente |
| C4 | Iluminação mínima (cor por vértice + luz direcional) | pendente |
| C5 | `njEasy*`/`njModelE.c` (falha: backend SH-4 do SH4ZAM) | pendente |

## Fase D — Título e menus (gate do M7)

| # | Entrega | Gate |
|---|---|---|
| D1 | Carga de programas: `content.c` de cada programa, `gdFsRead` nas áreas `ZDC`, relocação dos ponteiros internos (`engine/native/loads.c` como referência) | o loader troca de programa sem corromper a arena |
| D2 | Texturas do menu (depende de C1) | menus com as texturas certas |
| D3 | Pad completo do jogo (A/B/X/Y/Start/D-pad) | navegar o menu |
| D4 | Áudio do jogo (`sd_*`/ADX) pelo IMedia, em streaming | música do título |
| D5 | Decisão sobre o passe de floats (`tools/floats.py`, `-double=float`) | comparação de uma função de física com o host |

Gate: **título + menu navegável no zeebx lendo assets pelo BREW FS** (gate
original do M7). Bloqueio atual: o menu é o programa ADVERTISE (`menu_begin`
salta para 0x58). D1 precisa de carga dinâmica (débito técnico em
`zeebo-port-roadmap.md`): ~250 programas não cabem linkados no `.mod`.

## Fase E — Primeiro stage (M8)

| # | Entrega | Gate |
|---|---|---|
| E1 | Emerald Coast: land, objetos, texturas, HUD sob demanda | stage carrega |
| E2 | Câmera, colisão (`polygon.c`/`mesh.c` já compilam), física do Sonic | mover o personagem |
| E3 | Save em arquivo (`bu*`/`save_*` -> IFileMgr) | salvar e recarregar |
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
