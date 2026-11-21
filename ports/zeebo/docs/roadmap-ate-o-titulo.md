# Roadmap: do estado atual até o título e o primeiro stage

Complementa `zeebo-port-roadmap.md` (estado, evidências, riscos). Este arquivo
é a **ordem de execução**: fases, entregas, gates e dependências. Estimativas
são de esforço de trabalho focado, não de calendário, e valem só como ordem de
grandeza (o que não foi medido está marcado).

Estado de partida (2026-11-21): applet de bring-up desenha no zeebx com o
Ninja/sprite reais do decomp, texturas PVM, ADX, arquivos por IFileMgr/GDFS,
timer, pad, arena de RAM e threads. O C do jogo compila em ARM32 (509/510
objetos), mas **o jogo ainda não foi linkado nem executado**.

## Fase A — Linkar o jogo com o applet (sem executar)

Objetivo: um `.mod` único que contém o jogo e o applet e linka sem símbolos
indefinidos.

| # | Entrega | Gate |
|---|---|---|
| A0 | Avaliar o BREW Simulator do SDK 4.0.2 (instalador `BMP_BREWPLATFORM_4.0.2.20_SETUP_0.exe` + device pack Zeebo `ZeeboSDKInstaller.msi` + extensão OpenGL ES) sob Wine. Ele roda **DLL x86**, não o `.mod` ARM: só vale se houver um alvo x86 (mingw i686) do applet | decisão registrada: instala/roda, e se o alvo x86 compensa |
| A1 | Resolver as 69 definições duplicadas (ADX, `njDraw`, `njSprite`, `nj_globals`, `memcpy/memset/cos`, `njLoadTexturePvmMemory`): o jogo é o dono do que é jogo; o applet fica só com o que é plataforma | `ld` sem `multiple definition` |
| A2 | Shims de Kamui (`kmRender`, `kmFlipFrameBuffer`, `kmResetPointer`, `kmi*`) ligados ao fim de frame e ao `zgame_step`/`native_vsync` | símbolos `km*` resolvidos |
| A3 | `gdFs*` do jogo -> `zgdfs.c` (já existe); libc que os shims de host pedem (`malloc/free/fopen/...`) -> BREW `MALLOC`/IFileMgr | `gdFs*`, `malloc` e família resolvidos |
| A4 | `bu*` (backup/VMU) e `save_*`: stubs que falham de forma explícita; arquivo real fica para a Fase E | `bu*` resolvidos |
| A5 | Nomes por endereço do `symbols.txt` (244): script de linker ou `ZDC` | sem `0x8cXXXXXX` indefinido |
| A6 | Tirar as pontas fracas de `zthreads.c` (usar `thread_current/next/exit` do jogo) | `zthreads_selftest` ainda passa |
| A7 | `native_programs`, `native_hook_*`, `loader_entry`, `movie_entry`: valores mínimos | link fecha |

Gate da fase: `make -f game.mk` + link final gera `sa1z.mod` e `check.sh`
continua verde. Risco: tamanho do `.mod` (hoje ~10 MB) e bss; medir ao fechar.

## Fase B — Primeira execução do jogo (boot)

Objetivo: `sa1_main` roda no zeebx e vai até a primeira falha, registrada.

| # | Entrega | Gate |
|---|---|---|
| B1 | `zgame_start(sa1_main)` no start do applet; `zgame_step()` por frame do timer | o jogo executa N instruções sem travar o BREW |
| B2 | Log por chamada de shim (`DBGPRINTF`), `zthreads_check` por frame | trace legível do caminho de boot |
| B3 | Corrigir falhas em ordem: heap (`syMalloc` sobre arena), `sbinit`, carga do primeiro programa por `gdFsRead` | cada falha tem commit próprio |
| B4 | Medir uso real de pilha por thread e ajustar os 32 x 32 KB | canários intactos em 60 s |

Gate: o jogo passa do `sbinit` e do loader e desenha qualquer coisa
(tela de aviso/logo) no zeebx. Segunda opinião no GEC quando disponível.

## Fase C — Desenho de modelos 3D (o maior item)

Objetivo: os modelos do jogo (Ninja CNK) aparecem pelo sink GLES1.

| # | Entrega | Gate |
|---|---|---|
| C1 | `src/draw/model.c` compilando: ligar `engine/dc/sa1/{live,model}.c` ao sink (`sa1_live_model/nested/land`) | `draw_*` resolvidos |
| C2 | `NJM_SET_*_STRIP`, `loop*_body`, `pack/unpack_argb_body`, `view_*`, `writeback_line`, `submit_*`: asm do SDK por `asm2c.py` ou C | objetos do Ninja sem indefinidos |
| C3 | Um modelo real (ex.: Sonic ou um objeto de stage) desenhado com T&L de hardware e matriz do Ninja em 16.16 | quadro no zeebx comparado com o PC/DC |
| C4 | Iluminação mínima (o Adreno 130 não tem hardware de luz do SA1): vertex color + luz direcional do `draw_set_light` | modelo legível, sem custo por vértice na CPU |
| C5 | `njEasy*` e `njModelE.c` só se o jogo os chamar no caminho de boot/stage | decisão registrada |

Risco: soft-float por vértice. Manter vértices em espaço de objeto e `GL_FIXED`.

## Fase D — Título e menus (gate do M7)

| # | Entrega | Gate |
|---|---|---|
| D1 | Carga de programas: `content.c` de cada programa, `gdFsRead` nas áreas `ZDC`, relocação dos ponteiros internos (`engine/native/loads.c` como referência) | o loader troca de programa sem corromper a arena |
| D2 | Texturas do jogo (PVM do menu) pelo `njLoadTexturePvmMemory` com índices do jogo | menus com as texturas certas |
| D3 | Pad completo do jogo (A/B/X/Y/Start/D-pad) | navegar o menu |
| D4 | Áudio do jogo (`sd_*`/ADX) pelo IMedia, em streaming | música do título |
| D5 | Decisão sobre o passe de floats (`tools/floats.py`, `-double=float`) | comparação de uma função de física com o host |

Gate: **título + menu navegável no zeebx lendo assets pelo BREW FS** (gate
original do M7).

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

1. Tamanho máximo de `.mod` e de bss (hoje ~10 MB): sem hardware, só o GEC pode
   dizer algo; até lá, tratar como orçamento a reduzir.
2. Heap de um app (no zeebx é 64 MB, escolha do emulador; o GEC pode reportar outro).
3. Custo do soft-float no ARM11 por quadro de stage.
4. Se o jogo exige o passe de floats do SHC para a física ficar fiel.
5. Se a carga de programas por relocação dos ponteiros é suficiente ou se o
   `content.c` de cada programa precisa ser linkado em endereços próprios.
