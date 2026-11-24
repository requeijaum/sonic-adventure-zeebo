# Roadmap: do estado atual até o título e o primeiro stage

Complementa `zeebo-port-roadmap.md` (estado, evidências, riscos). Este arquivo
é a **ordem de execução**: fases, entregas, gates e dependências. Estimativas
são de esforço de trabalho focado, não de calendário, e valem só como ordem de
grandeza (o que não foi medido está marcado).

Estado (2026-11-22, manhã; superado, ver a atualização de 2026-11-23 23:07 abaixo): o jogo (1ST_READ do decomp em ARM32) roda no
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

### Atualização 2026-11-23 23:07 — espelho de estado GL (1,9 M chamadas a menos) e gate de hash no `check.sh`; imagem ainda em 6 linhas de matriz

Verificado nesta rodada (cron de 2 h, 23:03-23:12; `HEAD` = `0c23e2e`, 23:03 —
passou a `fe22ce0` às 23:08:36). **Outra sessão buildava e editava a mesma
árvore durante a rodada**: `zstrip.c` e `app.c` gravados às 23:03:55, `make -f
game.mk mod DEBUG_TITLE=0` às 23:05 e, no log dela, `make: *** [game.mk:371:
mod] Error 1` às 23:04 — na mesma janela do `make mod` desta sessão (23:04:57).
Os dois makes escrevem `build/zeebo/sa1g.mod` e os 242 `.ovl`; nenhum dos dois
pede o `flock` do `tools/build.sh` (`build.sh:8-13`). No fim da rodada, a outra
sessão commitou `fe22ce0` (23:08:36): Boomerang (`zboom.*`, `zhid.*`,
`tests/boom_test.c`) e contadores do passe translúcido (`app.c`, `zstrip.c`).

- `make -C ports/zeebo -f game.mk mod` → **`game.o: 744 de 744 objetos`**,
  `duplicatas: 0  indefinidos: 689`; `sa1g.mod` = **36.765.820 B** (35,06 MiB) no
  run desta sessão. O `.mod` que está no disco agora é o da outra sessão, com
  `DEBUG_TITLE=0` (23:06:30): **36.765.868 B** (+48 B).
- `make -C ports/zeebo host-selftest` → **`PASSED (0 failures)`**.
- `PROGRAMS` = **244** (204 `ev*` + 40 módulos); `MISSING_PROGRAMS` **vazio**.
  A tabela `title_debug_stages` tem **73 linhas com `STAGE_*` + 3 zeros** para
  `DEBUG_STAGES` 76 (`title.c:202,384`).
- `arm-none-eabi-size` dos 244 `program-*.o` (igual a 20:56): 8.110.685 `.text` +
  60.544.322 `.data` + 1.937.874 `.bss` = **70.592.881 B** (67,3 MiB).
- 242 `.ovl` (23:06) = **32.120.035 B** no disco (eram 32.119.721 às 20:56).
- `build/zeebo/matrix_r` **sem linha nova**: as mesmas 6 linhas de 20:18
  (`summary.txt`, `fps.md`) mais a row 68 de 16:34. Com o `.mod` de 744 objetos,
  **70 das 76 linhas do menu** continuam sem medida.

Commits desde as 20:56 (14, 7 de código): `bb28875` 21:02 (docs da rodada
anterior), `b4de392` 21:42 (`keep_build.sh`: `.mod` **com** os `.ovl` do mesmo
build), `16e4790` 21:43 (docs: débito D1-D5, V1-V12), `de85276` 21:50
(`STATECACHE` no passe translúcido + contadores `vprof` de desenhos e estado
repetido), `d6575af` 21:57 (`HOTOPT=-O2` em `zstrip.c`/`zkamui.c`: −1,8% de
instruções), `dcd3756` 21:59 (V7 não é defeito), `0c2ac03`/`e52f7e3` 22:05
(docs: RE4/OpenCE, ATITC; V11), `f267090` 22:16 (**`estado hash` por quadro** e
`tools/hash_gate.sh`), `de8a5b8` 22:25 e `de5ba50` 22:27 (docs), `77c4865` 22:41
(**espelho de estado GL**: blend, máscara de profundidade, textura 2D, modo de
matriz, projeção, ponteiros), `47d0bc5` 22:50 (carimbo de flags no `build.sh`,
**gate do `estado hash` no `check.sh`**, `sa1z*.bmp` fora do git, `hash_gate`
compara quadros) e `0c23e2e` 23:03 (**espelho de `BindTexture`**, `HOTFILES` para
medir `-O2` em outros arquivos — sem ganho).

Ganho medido do espelho (docs da sessão, 100 s virtuais, 734 quadros, imagem
idêntica `md5 4e1d655324ad`): ≈ **1,9 M chamadas GL a menos** por run —
`VertexPointer`/`TexCoordPointer`/`ColorPointer` 173.678 → 1.331, `MatrixMode`
345.666 → 4.814, `LoadIdentity` 173.344 → 2.578, `LoadMatrixx` 341.532 →
171.106, `DepthMask` 369.073 → 139.932, `Enable` 344.958 → 74.776, `Disable`
285.976 → 74.421; `DrawArrays` (516.609) não muda. Instruções do guest:
+0,01%. O `BindTexture` de `0c23e2e` é o **único** commit do desenho sem número
publicado.

| Área | Estado real (23:07) |
|---|---|
| Objetos, duplicatas | **744 de 744**, 0 duplicatas, 689 indefinidos |
| Programas | **244 em `PROGRAMS`** (204 eventos + 40 módulos); `MISSING_PROGRAMS` vazio |
| `.mod` | **36.765.820 B** (run desta sessão) / **36.765.868 B** no disco (23:06:30, `DEBUG_TITLE=0` da outra sessão) |
| Overlays | 242 `.ovl` = **32.120.035 B** (23:06) |
| `program-*.o` | 70.592.881 B: 8,11 `.text` / 60,54 `.data` / 1,94 `.bss` |
| Selftest | `PASSED (0 failures)` |
| Build | `make mod` ~41 s; gate de objetos segue tolerante (`game.mk:334`) |
| Matriz | **6 linhas** de 20:18 + row 68; 70 linhas do menu sem medida com este `.mod` |
| Zero (row 39) / Cart (row 65) | 99% / 95% pretos, com HUD (20:18, sem mudança) |
| Rows 37 / 46 / 71 | 27/51/49 · 42/53/56 (466 cores) · 18/27/15 (sem céu) |
| Desempenho | **68,1 ms de jogo/quadro** no Emerald Coast (7,6 fps no zeebx) |
| Chamadas GL | espelho de estado (`77c4865`, `0c23e2e`): ≈1,9 M a menos por 100 s |
| Gate de desenho | `check.sh:67` compara o `estado hash` com `tools/estado_hash.ref` (11 hashes + 734 quadros) |
| Gate do cartão | `CHECK_GAME=1` ainda falha antes de `ava_vmssel` (`check.sh:74`) |
| Som | `AUDIO=0` (`game.mk:274`, `Makefile:70`); música de 40 s (`zaudio.c:315,559`); driver `sd*` é stub gerado (`build/zeebo/game/zstubs.c:183`, `sdLibInit` → 0) |
| Controle | IHID; Boomerang commitado (`fe22ce0`: `zboom.*`, `zhid.c:87`), falta testar no zeebx |
| Árvore | suja só com os **10** `sa1g_*.bmp` na raiz (o resto foi commitado em `fe22ce0`) |
| Concorrência | duas sessões no mesmo `build/zeebo`: `.mod`/`.ovl` compartilhados e `make` direto fora do `flock` |
| Matriz (tools) | `stage_matrix.sh:13` roda 29 linhas por padrão; o menu tem 76 |

**O que falta para o objetivo final (ordem de 23:07)**: (1) rodar as **70 linhas
do menu** que não têm medida com o `.mod` de 744 objetos — só 6 (20:18) + a row 68
(16:34) foram vistas; (2) as **cenas pretas com HUD** (Zero row 39, 99% preto;
Twinkle Circuit row 65, 95%) e os defeitos das rows 37/46/71 (esverdeada, escura
sem HUD, sem céu); (3) as **7 linhas** de chefe/stage ausentes do menu (Hot
Shelter, CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R) — IDs já existem
(`src/stage/stage.h:28,33-40`), a tabela não; (4) **conteúdo nunca rodado**: atos
2-4 de cada stage, os 6 campos (Station Square, Mystic Ruins, Egg Carrier, Egg
Carrier por dentro, Past, Chao Garden), chefes sem linha, Chao Garden/corrida
(rows 69-72) e os minijogos (Twinkle Circuit, Sand Hill, Sky Chase — os dois
últimos já rodam); (5) **cortar ~6 MB**: `.mod` 36,77 MB contra os 30-32 MB de
RAM, com 32,12 MB de `.ovl` no disco; (6) **som** (laço da música, efeitos e
vozes pelo driver `sd*` que hoje é stub) e filme com som; (7) **desempenho**
(68,1 ms/quadro, 7,6 fps no zeebx) e **pacote instalável**; (8) `CHECK_GAME=1`
voltar a passar no gate do cartão. A tabela do objetivo final (14:35) e os 20
quick wins X1-X20 abaixo refletem esta rodada.

#### 20 quick wins (abertos em 23/11 23:07; P ≤ 1 h, M ≤ meio dia)

Numeração nova (**X**, de atualização; a lista U de 20:56 fica como histórico —
nenhum U fechou nesta janela). Todos os 20 estão **abertos**; o que fechou está
logo depois da tabela. Evidência é arquivo:linha ou medida desta rodada.

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| X1 | Rodar as 70 linhas do menu que faltam com o `.mod` de 744 objetos e colar `summary.txt` + `fps.md` | M | `matrix_r/summary.txt` = 6 linhas (20:18); `stage_matrix.sh:13` (29 linhas padrão); `title.c:202` (`DEBUG_STAGES 76`) |
| X2 | Separar o artefato por variante (um `build/zeebo` por conjunto de flags): hoje outra sessão reescreve `sa1g.mod`/`.ovl` no mesmo caminho | M | `.mod` 36.765.820 B (23:04:57, default) contra 36.765.868 B (23:06:30, `DEBUG_TITLE=0`); `.ovl` regerados às 23:06 |
| X3 | `check.sh` e o cron chamarem `tools/build.sh` (flock) em vez de `make` direto | P | `check.sh:51,61`; `build.sh:8-13`; `game.mk:371: mod Error 1` no log da outra sessão às 23:04 |
| X4 | Carimbar `DEBUG_TITLE` no `build/zeebo/.flags` (`build.sh:17` não o inclui): troca de variante deixa `title.o`/`main.o` velhos e o gate de hash acusa diferença falsa | P | `build.sh:17`; `game.mk:63-64` (`CFLAGS += -DSA1_DEBUG_TITLE=`) |
| X5 | Gate de objetos: `make mod` falhar quando N < M | P | `game.mk:334` (só `echo`); `check.sh:53` compara `GAME_MIN` 489, não N=M |
| X6 | Teto de tamanho no `check.sh` (`.mod` ≤ 32 MB e `.ovl` ≤ X): hoje nada impede o crescimento | P | `.mod` 36,77 MB + 32.120.035 B de `.ovl`; `check.sh:50-70` não mede tamanho |
| X7 | Medir o espelho de `BindTexture` (`0c23e2e`) e separar `estado repetido` por tipo (bind/ponteiros/matriz/projeção) | P | `zstrip.c:163-171` (só `gl_bind`); `app.c:197` imprime um total; incrementos em `zstrip.c:123,164,188,253,529` |
| X8 | Zero (row 39) e Twinkle Circuit (row 65) com HUD e cena preta: achar o que apaga o 3D | M | `matrix_r/row39.png` 0/0/0 e 99% escuro; `row65.png` 3/3/3 e 95% |
| X9 | Defeitos das rows 37 (esverdeada), 46 (sem HUD claro) e 71 (sem céu) | M | `row37.png` 27/51/49; `row46.png` 42/53/56 (466 cores); `row71.png` 18/27/15 |
| X10 | 7 linhas no menu de debug (Hot Shelter, CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R) | P | `title.c:202` (`DEBUG_STAGES 76`) e tabela em `:384` (73 linhas + 3 zeros); IDs em `stage.h:28,33-40` |
| X11 | Chao Garden e corrida: rows 69-72 nunca medidas com `al_main`/`al_garden00-02`/`al_race` linkados | M | `PROGRAMS` os tem; nenhuma row 69-72 em `matrix_r/summary.txt` |
| X12 | Save no campo: gravar/ler `sa1vmu.bin` dentro de Station Square (data já vem do RTC) | P | `zshinobi.c:210` (`VMU_FILE`); `zfile.c:77` (`_OFM_CREATE`); Fase E3 |
| X13 | Música em laço (o ADX toca 40 s e para) | M | `zaudio.c:315` (`MUSIC_SECONDS 40`), usado em `:559` |
| X14 | `AUDIO=1`: medir o custo em ms/quadro da música | P | `game.mk:274` e `Makefile:70` (`AUDIO ?= 0`); 68,1 ms/quadro sem áudio |
| X15 | Efeitos e vozes: o driver `sd*` (`sdLibInit`, `sdMidiPlay`, `sdMemBlk*`) é stub gerado que devolve 0; `src/sound/sound.c` compila mas não sai som | M | `build/zeebo/game/zstubs.c:183`; `sound.o` tem 25 indefinidos, quase todos `sd*` (`sdBankDownload`, `sdMidiOpenPort`, `sdMemBlkCreate`…) |
| X16 | Filme com som e ms/quadro | M | `zmovie.c:102` (`movie_run`), `:217`; SFD = MPEG-1 + ADX |
| X17 | `PROF=1` no Emerald Coast e atacar a função mais cara | M | `game.mk:120` (`PROF ?= 0`); 68,1 ms de jogo/quadro |
| X18 | Boomerang: testar no zeebx (volante, botões, HOME) e medir `ZBOOM_STEER_SIGN` — o commit já saiu (`fe22ce0`) | P | `zboom.c:135` (`* ZBOOM_STEER_SIGN`), `zboom.c:16` (sinal 1); `zhid.c:87` (VID/PID `0x15a2`/`0x0003`); `tests/boom_test.c:82` (`boom_test: ok`) |
| X19 | `.gitignore`: cobrir `sa1g_*.bmp` (só `sa1z*.bmp` está fora) — 10 BMPs sujam o `git status` | P | `ports/zeebo/.gitignore` (3 entradas `sa1z*`); `git status` com 10 `sa1g_*.bmp` |
| X20 | Pacote instalável: conjunto mínimo por área (Station Square + Emerald Coast: `.ovl`, texturas, sons) contra a eNAND | M | `extract/` = 1,2 GB; 242 `.ovl` = 32.120.035 B; Developer Guide 0.97 (heap de 32 MB) |

Fechados nesta janela (não entram na lista): **V1 e V8** (`de85276`:
`STATECACHE` no passe translúcido + contadores `vprof`), **V3** (`d6575af`:
`HOTOPT=-O2`, −1,8% de instruções, imagem idêntica), **V13** (`f267090`: `estado
hash` por quadro; `47d0bc5` o liga ao `check.sh` com `tools/estado_hash.ref`) e
**V14** (ATITC documentado; o zeebx já implementa). Segue aberto o **V2** (a
ordenação por inserção do passe translúcido, `zstrip.c:493`). Também ficou pronto
o `keep_build.sh` (`b4de392`), o `HOTFILES` (`0c23e2e`, medido sem ganho em
outros arquivos) e o commit do Boomerang (`fe22ce0`, 23:08:36: `zboom.*`,
`zhid.*`, `tests/boom_test.c` — falta só a medida no zeebx, virou X18).

### Atualização 2026-11-23 20:56 — build em 41 s, DYNRES pela extensão do SDK, FPSHUD; Zero e Cart ainda pretos

Verificado nesta rodada (cron de 2 h, 20:45-20:57; `HEAD` = `ce9e0ab`, 20:58 —
outro processo commitou docs durante a rodada e a árvore segue com o Boomerang
sem commit):

- `make -C ports/zeebo -f game.mk mod` → **`game.o: 744 de 744 objetos`**,
  `duplicatas: 0  indefinidos: 689` (eram 691 às 18:50); `sa1g.mod` =
  **36.763.516 B** (35,06 MiB, +309.248 B em 2 h), `PROF=0`; `gen_ovl`:
  **242 `.ovl` = 32.119.721 B** no disco, região de overlay 2.242.340 B.
- `make -C ports/zeebo host-selftest` → **`PASSED (0 failures)`**.
- `PROGRAMS` = **244** (204 `ev*` + 40 módulos); `MISSING_PROGRAMS` **vazio**.
- `arm-none-eabi-size` dos 244 `program-*.o`: 8.110.685 `.text` +
  60.544.322 `.data` + 1.937.874 `.bss` = 70.592.881 B (67,3 MiB).
- Build de `make mod` em **~41 s** (`63dadff`: `gen_traps` lê o ELF direto,
  `gen_ovl` com dicionário e cache de PRS, `flock`, `-MMD -MP`).
- Matriz: **só 6 linhas novas** (`build/zeebo/matrix_r/`, 20:18) — rows 3, 37,
  39, 46, 65 e 71, todas **`traps=0  ausente=0`** (o `TRAP READ` do Zero sumiu
  com `15c05e6`). A matriz principal continua parada em **30 linhas de 14:18**;
  as 76 linhas com o `.mod` atual não foram rodadas.
- `fps.md` (mesma rodada, zeebx): row 3 **68,1 ms de jogo/quadro** (7,6 fps),
  row 37 54,1 (8,9), row 39 59,0 (8,6), row 46 37,0 (10,5), row 65 26,7 (11,8),
  row 71 26,8 (11,5); RAM livre mínima 58.719 KB (heap de 64 MB do emulador).
- Conferência de pixels dos PNGs de `matrix_r` (medida nesta rodada): row 3
  média 170/170/181 e 3.486 cores (fase clara e completa); **row 39 média 0/0/0,
  99% escuro, 60 cores** (Zero segue preto, só HUD); **row 65 média 3/3/3, 95%
  escuro** (Cart segue preto, antes saía verde); row 37 27/51/49 (30% escuro,
  esverdeada), row 46 42/53/56 (466 cores, sem HUD claro), row 71 18/27/15 (360
  cores, sem céu); row 68 (Sand Hill, 16:34) 183/192/205 e 5.837 cores, ok.

Commits desde a atualização de 18:50 (fora docs): `63dadff` 19:00 (**build de
453 s para ~41 s**), `a79f8c7` 19:29 (**`kmLoadTexturePart` real** e
`TextureSize` na superfície: texturas `.PVR` de arquivo subiam vazias — caixa de
missão e letreiros em branco; Emerald Coast jogável com HUD), `15c05e6` 19:41
(**`bss_end` de `b_robo`/`b_e101_r`** de `bounds.src` como buffer real: fim do
`TRAP READ` no Zero/E-101R), `1254fef` 19:48 (`zlog` com cota por categoria — 60
linhas/24 categorias; probe de extensões GL; gate `novfp` no `check.sh`; achado
QW21: `CHECK_GAME=1` falha no gate do cartão) e `1770e94` 20:50 (**`DYNRES` via
`EGL_QUALCOMM_surface_scale`**, sem cópia de textura — a versão por
`glCopyTexSubImage2D` de `4e5d70b` foi removida; overlay **`FPSHUD=1`** em
`zhud.c`; `tools/fps_matrix.sh`). Docs corrigidos em `df64c4e` (sessão Qwen),
`679bc54`, `d0ef061`, `72cac2a`, `89b4428` (ressalva de binning do Developer
Guide: `DYNRES` provavelmente não ajuda no aparelho, só no fill rate) e
`1cfc061`/`ce9e0ab` (lições do port de Sonic Mania/Dreamcast).

Na árvore, **sem commit**: Boomerang (`zboom.c/h` novos, `zhid.c/h`
modificados, `tests/boom_test.c` → `boom_test: ok` no PC; `zhid.c:86` detecta
VID/PID `0x15a2`/`0x0003` e reabre o controle a cada ~60 leituras) e **8 BMPs
soltos na raiz** (`sa1g_audio.bmp`, `sa1g_debug.bmp`, …) que sujam o
`git status` e não pertencem ao repositório.

| Área | Estado real (20:56) |
|---|---|
| Objetos, duplicatas | **744 de 744**, 0 duplicatas, 689 indefinidos (eram 691) |
| Programas | **244 em `PROGRAMS`**, `MISSING_PROGRAMS` vazio (204 eventos + 40 módulos) |
| `.mod` | **36.763.516 B** (35,06 MiB), `PROF=0`; +309.248 B desde 18:50 |
| Overlays | 242 `.ovl` = 32.119.721 B no disco; região 2.242.340 B |
| `program-*.o` | 70.592.881 B: 8,11 `.text` / 60,54 `.data` / 1,94 `.bss` |
| Selftest | `PASSED (0 failures)` |
| Build | `make mod` em **~41 s** (`63dadff`), gate de objetos ainda não falha (U3) |
| Matriz | 6 linhas novas às 20:18 (3, 37, 39, 46, 65, 71), `traps=0`, `ausente=0`; 76 linhas não rodadas |
| Emerald Coast (row 3) | 68,1 ms de jogo/quadro, 7,6 fps no zeebx; quadro claro (170/170/181) |
| Zero (row 39) | TRAP some (`15c05e6`), 777 quadros, HUD e título; cena **99% preta** |
| Twinkle Circuit (row 65) | 0 traps, 879 quadros, HUD de volta; pista **95% preta** |
| Rows 37 / 46 / 71 | 37 esverdeada (27/51/49), 46 escura sem HUD claro (466 cores), 71 sem céu (18/27/15) |
| Sand Hill (row 68) | ok: 1206 quadros, 183/192/205, 5.837 cores (16:34) |
| DYNRES | `EGL_QUALCOMM_surface_scale` (`1770e94`), padrão 0; ganho só se mede no aparelho |
| FPSHUD | `FPSHUD=1` (`zhud.c`); `fps_matrix.sh` mede a emulação, não o aparelho |
| Gate do cartão | `CHECK_GAME=1` **FALHA**: não chega a `ava_vmssel` (QW21, origem desconhecida) |
| Som | `AUDIO=0` (`game.mk:274`); música de 40 s sem laço (`zaudio.c:315`) |
| Controle | IHID (`5f70e0d`); Boomerang só na árvore, sem commit e sem teste no zeebx |
| Árvore | `zboom.*`/`zhid.*` sem commit; 8 `sa1g_*.bmp` soltos na raiz |

**O que falta para o objetivo final (ordem de 20:56)**: (1) rodar a **matriz
inteira** (76 linhas) com o `.mod` de 744 objetos, o IHID novo e o `FPSHUD`, e
colar o `summary.txt` — hoje só 6 linhas foram medidas; (2) cortar **~6 MB**
(`.mod` 36,76 MB + 32,1 MB de `.ovl` contra os 30-32 MB de RAM relatados);
(3) as cenas pretas com HUD (**Zero row 39** 99% preto, **Cart row 65** 95%
preto) e os defeitos das rows 37/46/71; (4) as **7 linhas** de chefe/stage que
faltam no menu de debug (Hot Shelter, CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R);
(5) conteúdo nunca rodado — atos 2-4 de cada stage, os 6 campos (Station
Square, Mystic Ruins, Egg Carrier, Egg Carrier por dentro, Past, Chao Garden),
chefes sem linha, minijogos (Twinkle Circuit, Sand Hill, Sky Chase) e Chao
Garden/corrida; (6) som (laço, efeitos, vozes) e filme com som; (7) desempenho
(68 ms/quadro no Emerald Coast, 7,6 fps no zeebx) e pacote instalável; (8)
`CHECK_GAME=1` voltar a passar no gate do cartão. A tabela do objetivo final e a
lista de 20 quick wins abaixo refletem esta rodada.

#### 20 quick wins (abertos em 23/11 20:56; P ≤ 1 h, M ≤ meio dia)

Numeração nova (**U**, de atualização). Todos os 20 estão **abertos**; os que
fecharam desde as 18:50 estão listados logo depois da tabela. Evidência é
arquivo:linha ou medida desta rodada.

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| U1 | Rodar a matriz inteira (76 linhas do menu) com o `.mod` de 744 objetos + IHID + `FPSHUD` e colar o `summary.txt` | M | `tools/stage_matrix.sh:13` (ROWS padrão de 29); `build/zeebo/matrix_r/summary.txt` só 6 linhas (20:18), `matrix/summary.txt` 30 linhas de 14:18 |
| U2 | 7 linhas no menu de debug (Hot Shelter, CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R): `DEBUG_STAGES` 76 → 83 | P | `src/progress/title.c:202` (`#define DEBUG_STAGES 76`) e a tabela em `:384`; IDs já existem em `src/stage/stage.h:28,33-40` |
| U3 | `make mod` falhar quando "N de M objetos" vier com N < M | P | `ports/zeebo/game.mk:334` (só `echo`, sem `exit 1`); `:331` diz "tolera objetos que não compilaram" |
| U4 | Cortar ~6 MB do `.mod` (36.763.516 B contra 30-32 MB de RAM) | M | `.mod` 36.763.516 B; `.data` dos 244 programas = 60.544.322 B; `al_main` sozinho tem 633.358 B de `.bss` |
| U5 | Gate do cartão: `CHECK_GAME=1` falha ("não chegou a `ava_vmssel`") desde antes da correção de textura; achar por bisect (suspeita `b7865dd`) e ajustar gate ou jogo | P | `check.sh:50`; log de 19:48 em `docs/pdca-build.md`/roadmap QW21 |
| U6 | Zero (row 39) e Twinkle Circuit (row 65) saem com HUD e cena preta, agora sem trap: achar o que apaga o 3D | M | `matrix_r/row39.png` média 0/0/0 e 99% escuro (777 quadros, `c2eb03f3a477`); `row65.png` 3/3/3 e 95% (879 quadros, `52455310e0db`) |
| U7 | Defeitos visuais das rows 37 (esverdeada), 46 (sem HUD claro) e 71 (sem céu) | M | `matrix_r/row37.png` 27/51/49 (`70b135d59597`), `row46.png` 42/53/56 e 466 cores (`71b404f1d722`), `row71.png` 18/27/15 (`ea2227a685c3`) |
| U8 | Chao Garden: rows 69-70 (`al_main` + `al_garden00-02`) nunca medidas e o céu da row 71 | M | `PROGRAMS` tem `al_main`, `al_garden00-02`, `al_race`; nenhuma row 69-72 em `matrix_r/summary.txt` |
| U9 | Conteúdo nunca rodado: atos 2-4 de cada stage, os 6 campos, chefes sem linha, Chao Garden e minijogos | M | 30 linhas de matriz cobrem 1 ato por stage; `src/modules/` tem 40 programas; Hot Shelter sem linha (`title.c:384`) |
| U10 | Culling em software no `send_strip` (contagem de faces descartadas por quadro) | M | `zlive.c:340` (assinatura sem `cull`); as 3 chamadas em `zlive.c:416,421,426` |
| U11 | Save no campo: gravar/ler `sa1vmu.bin` dentro de Station Square (data já vem do RTC) | P | `b7865dd`; `IFILEMGR_OpenFile` relativo; Fase E3 do roadmap |
| U12 | Música em laço (o ADX toca 40 s e para) | M | `zaudio.c:315` (`#define MUSIC_SECONDS 40`), usado em `zaudio.c:559` |
| U13 | Efeitos e vozes (`sd_*` devolvem 0) | M | `tools/gen_stubs.py`; `zaudio.c` só tem `zaudio_play_adx` |
| U14 | `AUDIO=1`: medir o custo em ms/quadro da música | P | `game.mk:274` (`AUDIO ?= 0`); 68,1 ms/quadro sem áudio (row 3, `fps.md`) |
| U15 | Filme com som e ms/quadro | M | `zmovie.c:25` (`movie_files[10]`) e `:217` (`movie_run`); SFD = MPEG-1 + ADX |
| U16 | Triar os 689 indefinidos por frequência no run; os 10 mais chamados viram stubs reais | M | `build/zeebo/game/missing.txt` (1.132 nomes); `744 de 744` no link |
| U17 | `PROF=1` no Emerald Coast e atacar a função mais cara | M | 68,1 ms de jogo/quadro (7,6 fps) em `matrix_r/fps.md`; `game.mk:120` (`PROF ?= 0`) |
| U18 | Medir `DYNRES=50/75` (extensão EGL) no aparelho com `FPSHUD=1`, ou no zeebx depois de implementar a escala | P | `1770e94` (`EGL_QUALCOMM_surface_scale`); zeebx não conta custo de pixels (734 quadros nos dois casos) |
| U19 | Boomerang: commitar `zboom.*`/`zhid.*`, testar no zeebx (volante, botões, HOME) e limpar os 8 `sa1g_*.bmp` da raiz | P | `zboom.c` novo, `zhid.c:86` (VID/PID `0x15a2`/`0x0003`), `tests/boom_test.c` → `boom_test: ok` |
| U20 | Pacote instalável: conjunto mínimo por área (Station Square + Emerald Coast: `.ovl`, texturas, sons) contra a eNAND | M | `extract/` = 1,2 GB; 242 `.ovl` = 32.119.721 B; Developer Guide 0.97 (tudo no heap de 32 MB) |

Fechados nesta janela (não entram na lista): **QW1** `kmLoadTexturePart` +
`TextureSize` (`a79f8c7`), **QW4** cota do `zlog` (`1254fef`), **QW7** o
`TRAP READ` do Zero/E-101R (`15c05e6`) — a cena segue preta, virou U6 —,
**QW22** `DYNRES` pela extensão do SDK (`1770e94`) e **QW23** overlay `FPSHUD`
mais `fps_matrix.sh` (`1770e94`). **QW21** (gate do cartão) continua aberto como
U5. Seguem fechados de antes: **W1**, **W3**, **W7**, **N4**, **N18**, **N20**;
Sky Chase 1 e 2 e Sand Hill rodam (`941fd33`; row 68 em 16:34).

### Atualização 2026-11-23 18:50 — IHID (16 botões), RTC real, RAM em log; Boomerang na árvore

Verificado nesta rodada (cron de 2 h, 18:50-19:00; `HEAD` = `b7865dd`, 17:42;
outro processo tinha arquivos modificados na árvore e continuou mexendo durante
a rodada, ver o parágrafo "na árvore"):

- `make -C ports/zeebo -f game.mk mod` → **`game.o: 744 de 744 objetos`**,
  `duplicatas: 0  indefinidos: 691`; `sa1g.mod` = **36.454.268 B** (34,8 MiB),
  `PROF=0`; `gen_ovl: 242 overlays, 32.120.273 bytes no disco, regiao 2.242.340 B`.
  O `.mod` cresceu **3.488 B** desde 16:50 (IHID/`zhid.c` e o `zboom.c` da árvore).
- `make -C ports/zeebo host-selftest` → **`PASSED (0 failures)`**.
- `PROGRAMS` = **244** (204 `ev*` + 40 módulos); `MISSING_PROGRAMS` **vazio**.
- `arm-none-eabi-size` dos 244 `program-*.o`: 8.110.685 `.text` +
  **60.544.322 `.data`** + 1.937.874 `.bss` = 70.592.881 B (67,3 MiB). Cinco
  maiores (dec): `stg09` 2.348.984, `al_main` 2.332.106 (633.358 de `.bss`: o
  maior `.bss` entre programas), `stg06` 2.130.825, `stg12` 1.987.542,
  `al_race` 430.064.
- `zeebx run --seconds=10`: boot até `game_mode` 8 (MOVIE), `hid controle
  aberto`, `mem livre KB` 64.895 (heap de 64 MB do emulador, não do aparelho).
- **Matriz não foi re-rodada**: `build/zeebo/matrix/summary.txt` continua com 30
  linhas (29 de 13:2x + row 15 às 14:18). A rodada inteira com o `.mod` atual é
  a T1.

Commits desde a atualização de 16:50 (todos do port, fora docs): `5f70e0d`
17:04 (**controle pelo IHID**: 16 botões, 4 eixos e gatilhos somados ao
`EVT_KEY` — `zhid.c:42-61`, o W22), `6d5b4b2` 17:12 (**marca d'água da RAM livre**
a cada 2 s no log — `app.c:193-200`, o W24) e `b7865dd` 17:42 (**`syRtcGetDate`
real** por `GETJULIANDATE`, `zsys.c:261-275`: sem ele o id dos Chao e a data do
save liam lixo da pilha).

Na árvore, **sem commit** (outro processo, nesta janela; o `.mod` medido acima
já os inclui): suporte ao controle **Boomerang** (`zboom.c/h`,
`tests/boom_test.c` → `boom_test: ok` no PC; `zhid.c:86` detecta VID/PID
`0x15a2`/`0x0003` e reabre o controle a cada ~60 leituras; `docs/boomerang.md`),
o DIAG da textura **retangular linear** (`data_format` 0x09, `ztexture.c:154-157`)
e as mudanças de build da tabela abaixo (`JOBS ?= 8`, `-MMD -MP`, cache de PRS,
leitura direta das `.rel` em `gen_traps.py:15`). Nada disso entra no commit de
docs.

| Área | Estado real (18:55) |
|---|---|
| Objetos, duplicatas | **744 de 744**, 0 duplicatas, 691 indefinidos |
| Programas | **244 em `PROGRAMS`**, `MISSING_PROGRAMS` vazio (204 eventos + 40 módulos) |
| `.mod` | **36.454.268 B** (34,8 MiB), `PROF=0`; +3.488 B desde 16:50 |
| Overlays | 242 `.ovl` = 32.120.273 B no disco; região 2.242.340 B |
| `program-*.o` | 70.592.881 B (67,3 MiB): 8,1 `.text` / 60,5 `.data` / 1,9 `.bss` |
| Selftest | `PASSED (0 failures)` |
| Build | `make mod` em **~40-47 s** (era 3m52s às 14:35); `-MMD -MP` (`game.mk:60`) e `JOBS ?= 8` (`game.mk:33`) |
| Boot | 10 s → `game_mode` 8; `mem livre` 64.895 KB (zeebx) |
| Controle | IHID com 16 botões, 4 eixos e gatilhos (`5f70e0d`); **Boomerang** na árvore (teste de PC ok, zeebx pendente) |
| Relógio | RTC real (`b7865dd`): id de Chao e data de save deixam de ler lixo |
| Matriz | sem rodada nova; `summary.txt` com 30 linhas (14:18) |
| Sky Chase 1 e 2 | **rodam** (`941fd33`); sem log de matriz novo |
| Sand Hill (row 68) | **roda**: 1206 quadros, `8f52bbe305a4`, `ausente=0` (16:34, Ryzen) |
| Red Mountain (row 15) | não reseta (`ausente=0`); atos 2-3 não testados |
| Zero (row 39) e Cart (row 65) | pretas com HUD: `SimpleCalcModelMatrix` cai em armadilha |
| Rows 37 / 46 / 71 | 37 lavada em verde / 46 sem HUD / 71 com céu preto |
| Som | off por padrão (`AUDIO ?= 0`, `game.mk:274`); música de 40 s sem laço (`zaudio.c:315`) |
| Orçamento | `.mod` 36,45 MB contra os 30-32 MB de heap: **~6 MB a cortar** |
| Armadilha de build | dois builds no mesmo `build/zeebo` se misturam (esta janela viu um `PROF=1` de outro processo); medir em série |

**O que falta para o objetivo final (ordem de 18:50)**: (1) rodar a matriz
inteira (76 linhas) com o `.mod` de 744 objetos e o IHID novo; (2) cortar ~6 MB
(`.mod` 36,45 MB + bss 13,6 MB contra 30-32 MB de heap); (3) as cenas que saem
pretas ou erradas (Zero row 39, Cart row 65, rows 37/46/71); (4) as 7 linhas de
chefe/stage que faltam no menu de debug; (5) conteúdo nunca rodado — atos 2-3 de
cada stage, os 6 campos (Station Square, Mystic Ruins, Egg Carrier, Egg Carrier
por dentro, Past, Chao Garden), chefes sem linha, minijogos e o Chao Garden;
(6) som (laço, efeitos, vozes) e filme com som; (7) desempenho (~80 ms/quadro) e
pacote instalável. A tabela do objetivo final e a lista de 20 quick wins abaixo
já refletem esta rodada.

### Atualização 2026-11-23 16:50 — 744 objetos, nenhum programa ausente; Sky Chase e Sand Hill rodando

Verificado nesta rodada (cron de 2 h, 16:27-16:50; `HEAD` = `3b5e2af`; a árvore
tinha `ports/zeebo/zaudio.c` modificado por outro processo — não entrou no commit
de docs):

- `make -C ports/zeebo -f game.mk mod` → `game.o: 744 de 744 objetos`,
  `duplicatas: 0  indefinidos: 691`; `sa1g.mod` = **36.450.780 B** (34,8 MiB),
  `PROF=0`; 242 `.ovl` = 32.119.828 B no disco, região de overlay 2.242.340 B.
- `make -C ports/zeebo host-selftest` → **`PASSED (0 failures)`**.
- `PROGRAMS` = **244** (contado pelo `make`: 204 `ev*` + 40 módulos de
  `modules/`) e **`MISSING_PROGRAMS` = 0** — nenhum programa de `config/` fica
  fora do `.mod` (era 202 às 14:35). Dos 41 diretórios de `modules/`, só `movie`
  não é `PROGRAMS`: ele é `zmovie.c`.
- `arm-none-eabi-size` dos 244 `program-*.o`: **67,3 MB** (8,1 `.text`,
  **60,5 `.data`**, 1,9 `.bss`). O `.data` é o que o overlay tira do `.mod`:
  32,1 MB comprimidos em `.ovl` no disco.
- Row 68 (Sand Hill) medida às 16:34 no Ryzen (`build/zeebo/matrix_r/row68.log`;
  o `make` da mesma rodada usou `DEBUG_TITLE=0 PROF=1`): **quadro 1206, hash
  `8f52bbe305a4`, `ausente=0`** — antes era tela preta (`be11bba75cea` com
  `ausente=2`). Sky Chase 1 e 2 rodam desde `941fd33` (mesma correção), ainda sem
  log de matriz no laptop.

Commits desde a atualização de 14:35, fora de docs: `e7f11c8` 15:11
(`zshooting_fix` reloca os ponteiros de `shtpath*.PRS`; **não** resolveu sozinho),
`941fd33` 15:29 (**os programas compilam das cópias reescritas (ZDC) do
`REBASE_F`** — era o endereço cru do DC em `shooting.c`; Sky Chase 1 e 2 rodam)
e `3b5e2af` 16:17 (instrumentação: o `gdFs` loga leitura acima de 50 ms e o
`zADXT_StartFname` separa o tempo de `play_file` do `ADXT_StartFname` real).

| Área | Estado real (16:46) |
|---|---|
| Objetos, duplicatas | **744 de 744**, 0 duplicatas, 691 indefinidos (eram 614 com 542 objetos) |
| Programas | **244 em `PROGRAMS`**, `MISSING_PROGRAMS` = **0** (204 eventos + 40 módulos) |
| `.mod` | **36.450.780 B** (34,8 MiB), `PROF=0`; a marca de 35,5 MB da seção de 15h está desatualizada |
| Overlays | 242 `.ovl` (32,1 MB no disco); região 2.242.340 B dentro do `.mod` |
| `program-*.o` | 67,3 MB (8,1 `.text` / 60,5 `.data` / 1,9 `.bss`) |
| Selftest | `PASSED (0 failures)` |
| Matriz | sem rodada completa; `summary.txt` tem 30 linhas (29 de 13:2x com 518 objetos + row 15 às 14:18) |
| Sky Chase 1 e 2 | **rodam** (`941fd33`); sem log de matriz no laptop |
| Sand Hill (row 68) | **roda**: 1206 quadros, `8f52bbe305a4`, `ausente=0` (16:34, Ryzen; `.mod` do run com `DEBUG_TITLE=0 PROF=1`) |
| Red Mountain (row 15) | não reseta mais com os eventos linkados (`ausente=0`, 15h); atos 2-3 não testados |
| Zero (row 39) e Cart (row 65) | pretas com HUD: `SimpleCalcModelMatrix` cai em armadilha (13:2x) |
| Rows 37 / 46 / 71 | 37 sai lavada em verde / 46 sem HUD / 71 com céu preto (13:2x) |
| Som | off por padrão (`AUDIO ?= 0`, `game.mk:271`); música de 40 s sem laço (`zaudio.c:315`) |
| Orçamento | `.mod` de 36,45 MB contra os 30-32 MB de heap dos relatos: **~6 MB a cortar** |
| Armadilha de build | dois builds no mesmo `build/zeebo` (esta rodada viu um `PROF=1` concorrente) podem deixar o `.mod` de um ou de outro: medir em série |

A tabela do objetivo final e a lista de 20 quick wins abaixo refletem a rodada
das 18:50 (a lista N1-N20 foi substituída pela T1-T20).

### Atualização 2026-11-23 14:35 — os 40 módulos de programa linkados

Verificado nesta rodada (cron `9f2e68aa0549`, 14:09-14:20) na árvore de trabalho
(base `e32d165`; o link dos 24 programas restantes e o `gen_dups.py` de função
duplicada entraram como `4a9047c` às 14:16, sobre o mesmo conteúdo que foi
medido; um segundo processo mexia em `zgdfs.c`/`zgame.c`/`gen_ovl.py` — o
overlay — durante a rodada):

- `make -C ports/zeebo -f game.mk mod` → `game.o: 542 de 542 objetos`,
  `duplicatas: 0  indefinidos: 614`, `sa1g.mod` = 65.476.524 B (62,4 MiB), 3m52s.
- `make -C ports/zeebo host-selftest` → `PASSED (0 failures)`.
- `PROGRAMS` (`game.mk:137-148`) = 42 entradas: **os 40 módulos** de `modules/`
  (2 menus, 12 `stg` — 11 stages + Hedgehog Hammer —, 11 chefes, 7 campos, 5 de
  Chao, 3 mini-jogos) **+ `ev0001` e `ev002b`**. `MISSING_PROGRAMS` = **202,
  todos `ev*`** (204 eventos no decomp).
- `arm-none-eabi-size` dos 42 `program-*.o`: 45,5 MB (3,8 `.text`, **39,8
  `.data`**, 1,9 `.bss`). O `.data` dos programas é o que o overlay por área tem
  de tirar do `.mod`.
- Matriz anterior (13:2x, build de 518 objetos): 12 linhas rodaram, **17
  resetavam** com `programa ausente` (`build/zeebo/matrix/summary.txt`).
- Commits desde a atualização anterior (`2f0b1ad`, 12:21): `ffb7f8f` 13:27
  (`prscomp`, `gen_ovl.py`, `remote_matrix.sh`, docs da matriz de stages),
  `e32d165` 13:52 (arena de 6 MB e pilhas no heap, menu de debug abrindo nos
  stages, `remote_matrix.sh` no Ryzen) e `4a9047c` 14:16 (os 24 programas
  restantes em `PROGRAMS`; `gen_dups.py` trata função em dois programas).

| Área | Estado real (14:20) |
|---|---|
| Objetos, duplicatas | **542 de 542**, 0 duplicatas, 614 indefinidos (viram armadilhas) |
| `.mod` | 65.476.524 B (62,4 MiB): 42 programas + applet + jogo, **sem** overlay |
| Programas de stage/campo/chefe/jardim | **todos os 40 linkados** (eram 18 no `game.mk` de 12:21: 2 menus, 14 de stage/campo, 2 eventos) |
| Eventos | **2 de 204** (`ev0001`, `ev002b`); 202 ausentes → `gdFs` reseta ao ler |
| Símbolo em dois programas | `cart_step` em `minicart` e `stg03`: `gen_dups.py` gera `P__F` + trampolim `zfn_*`, trocado em `zprog_enter` (`build/zeebo/game/gen/zdups.c`) |
| Arena e pilhas | no heap (`zlive.c:450`, `zthreads.c:102`): −7 MB de `.bss` dentro do `.mod` |
| Menu de debug | abre direto na lista de stages (`src/progress/title.c:626`, `SA1_DEBUG_TITLE`) |
| Rodando | 12 linhas rodaram com 518 objetos (0/3/6/9/12/18/21/24/28/32/35/40); com 542 objetos só a row 15 foi medida: **Red Mountain ato 1 ainda para em `programa ausente`** (quadro 814, hash `613e298dc464`) |
| 3D in-game | Emerald Coast renderiza pelo `zlive` em `GL_FIXED` (areia, céu e mar 0,99-1,08 do PC; hotel 0,91), sombra suave, ~80 ms de jogo por quadro |
| Mistura | fatores do Ninja são códigos do PVR (`zstrip_blend_pvr`); `DST_ALPHA` vale 1 (RGB565) |
| Palmeiras | **resolvidas** (handoff §19): `alpha_bleed` nos texels transparentes |
| Som / filmes | som off por padrão (`AUDIO ?= 0`, `game.mk:251`); música 40 s sem laço (`zaudio.c:315`); filmes SFD decodificam em `pl_mpeg` (`zmovie.c:208`) |
| Save | cartão emulado em `sa1vmu.bin` (`cdd0c24`); não exercitado no campo |
| Overlays | `prscomp` + `gen_ovl.py` prontos (`ffb7f8f`); implementação em curso em `zgdfs.c`/`zgame.c` (`docs/wip/overlay-dados.patch`, não aplicado) |
| Superados | Q9–Q13 (a luz do `lt_arm.c` não é usada; o `zlive` tem a sua), Q14 (clip no near feito), Q17–Q19 (`frame_hash.sh`, limites e limpeza feitos) |

Compressão (792 `.PRS`, 251,1 MB brutos): PRS 127,7 MB; deflate -9 116,4 MB;
UCL (nrv2e) 118,1 MB; lz4 HC 132,3 MB; zstd -19 103,4 MB; xz -9 92,2 MB.
DECISÃO (23/11): manter PRS. No host o decodificador de PRS é tão rápido ou
mais que o inflate (313-457 contra 273-322 MB/s) e deflate rende só 5-10%
menos; texturas PVM já são VQ e quase não comprimem. O ganho real vem dos
overlays por área, não do formato.

Ordem restante (16:50): re-medir a matriz inteira com o `.mod` de 744 objetos →
cortar ~6 MB para caber nos 30-32 MB → som e filmes → desempenho e pacote. Os
programas e os eventos deixaram de ser o gargalo (`MISSING_PROGRAMS` = 0): o que
ainda quebra uma cena é bug de porta específico (ponteiro cru do DC, armadilha de
`SimpleCalcModelMatrix`, HUD, céu), não arquivo ausente.

#### OBJETIVO FINAL (pedido do Rafael, 23/11): port completo do SA1 para o Zeebo

"Linkado" quer dizer que compila e entra no `.mod`; só a coluna Estado diz o que
foi **visto** rodando no zeebx. **Nenhum** programa falta (`MISSING_PROGRAMS` =
0, 18:50), então "sem linha no menu" ou "preto" agora é bug, não arquivo ausente.
As linhas medidas com 518 objetos cobrem 1 ato por stage; desde então foram
medidas as rows 15 (Red Mountain), 66-68 (Sky Chase, Sand Hill); o resto dos
atos, campos e chefes nunca rodou.

| Conteúdo | Programa(s) | Estado |
|---|---|---|
| Programas/eventos no `.mod` | 244 | 40 módulos + 204 eventos; `MISSING_PROGRAMS` = 0 (20:56) |
| Emerald Coast | stg01 | ato 1 rodou (row 3, 680 quadros, `fc050a4575d8`); atos 2 e 3 não testados |
| Windy Valley | stg02 | ato 1 rodou (row 6); atos 2 e 3 não testados |
| Twinkle Park | stg03 | ato 1 rodou (row 9); atos 2 e 3 não testados |
| Speed Highway | stg04 | ato 1 rodou (row 12); atos 2 e 3 não testados |
| Red Mountain | stg05 | não reseta mais (evento linkado, `ausente=0`); atos 1-3 não medidos |
| Sky Deck | stg06 | ato 1 rodou (row 18); atos 2 e 3 não testados |
| Lost World | stg07 | ato 1 rodou (row 21); atos 2 e 3 não testados |
| Ice Cap | stg08 | ato 1 rodou (row 24); atos 2 a 4 não testados |
| Casinopolis | stg09 | ato 1 rodou (row 28); atos 2 a 3 não testados |
| Final Egg | stg10 | ato 1 rodou (row 32); atos 2 e 3 não testados |
| Hot Shelter | stg12 | linkado; **não tem linha no menu de debug** (`title.c:202,384-461`) |
| Hedgehog Hammer | stg00 | rodou (row 0) |
| Station Square (6 áreas) | adv00 | linkado; row 40 rodou em 13:2x (câmera num pilar, sem HUD); rows 41-45 não medidas |
| Mystic Ruins (4 áreas) | adv02 + tikal_prog | linkado; rows 58-61 nunca rodaram com os programas linkados |
| Egg Carrier (6 áreas) | adv01obj + adv0100 | linkado; row 46 não reseta mais, sai escura e **sem HUD claro** (`71b404f1d722`, 466 cores, 20:18); rows 47-51 não medidas |
| Egg Carrier por dentro (6) | adv01obj + adv0130 | linkado; rows 52-57 não medidas |
| Past (3 áreas) | adv03 + tikal_prog | linkado; rows 62-64 não medidas |
| Chaos 0 / 2 / 4 | b_chaos0 / b_chaos2 / b_chaos4 | Chaos 0 rodou (row 35); row 37 (Chaos 2/4) agora sai escura e esverdeada com HUD (`70b135d59597`, 20:18), antes lavada em verde |
| Chaos 6 / Perfect Chaos | b_chaos6 / b_chaos7 | linkados; **sem linha no menu** |
| Egg Viper 1-3 | b_egm1 / b_egm2 / b_egm3 | linkados; 1 tem linha (row 38), 2 e 3 **sem linha no menu** |
| Zero (robô do Eggman) | b_robo | linkado; `TRAP READ` resolvido (`15c05e6`): 777 quadros com HUD e título "ZERO", **cena 3D ainda preta** (média 0/0/0) |
| E101 / E101-R | b_e101 / b_e101_r | linkados; **sem linha no menu** |
| Chao Garden | al_main + al_garden00-02 | linkados; rows 69-70 nunca rodaram; row 71 com **céu preto** |
| Corrida de Chao | al_race | linkado; row 72 não medida |
| Twinkle Circuit | minicart | linkado; row 65 com HUD de volta (tempo, VOLTA 1/3) e pista **95% preta** (`52455310e0db`, 20:18); era preta com `SimpleCalcModelMatrix` em armadilha |
| Sky Chase 1 e 2 | shooting | **rodam** (`941fd33`; rows 66-67), sem log de matriz no laptop |
| Sand Hill / snowboard | sboard | **roda**: row 68, 1206 quadros, `8f52bbe305a4`, `ausente=0` (16:34) |
| Filmes (SFD) | zmovie.c + pl_mpeg | imagem decodifica (log `movie slh448_1.sfd`, 22/11); som do filme só com `AUDIO=1` |
| Som | zaudio.c | música 40 s sem laço (`zaudio.c:315`); efeitos e vozes são stubs (`sd_*` = 0) |
| Controle | zhid.c + zboom.c | IHID: 16 botões, 4 eixos e gatilhos somados ao `EVT_KEY` (`5f70e0d`); Boomerang na árvore (teste de PC ok, zeebx pendente) |
| Relógio | zsys.c | RTC real por `GETJULIANDATE` (`b7865dd`); id de Chao e data de save deixam de sair da pilha |
| Save | bu* / `sa1vmu.bin` | cartão emulado; save no campo não exercitado (a data do save agora vem do RTC) |
| Desempenho | — | **68,1 ms de jogo por quadro** no Emerald Coast (7,6 fps no zeebx; row 3, `matrix_r/fps.md`); PROF ainda 0 |
| Pacote | — | `.mod` de **36.763.516 B** (35,06 MiB) + 32.119.721 B de `.ovl`; **~6 MB acima** do orçamento de 30-32 MB; nada instalável |

#### Atualização 23/11 15h — W1, W3, W7 fechados (`32033db`, `HEAD`, em parte superada pela de 16:50)

- W1: os 204 eventos linkados (744 de 744 objetos, 0 duplicatas). A row 15
  (Red Mountain) deixou de resetar: era o evento de abertura (`ausente=0`).
- W3: overlays de dados feitos. Uma regiao por area do loader (modulo, field,
  extra, tikal, evento); `.ovl` em PRS no disco (242 arquivos, 32 MB), `.mod`
  de 65,8 para 35,5 MB com tudo linkado (31,0 MB antes dos eventos). Gate: hashes
  identicos ao build sem overlay nas rows 36-39, 52, 62, 65-72; 3, 6, 46 e 58
  mudam por 1 quadro (tempo de leitura do `.ovl`).
- W7: nome do arquivo ausente nao precisou mais de log (so sobravam eventos).
- Matriz remota (Ryzen, 28 linhas): todas passam do menu. Abertos: rows 66-68
  (Sky Chase/Sand Hill) pretas: `invalid access 0x0cb80000` em `shooting_shot_step`
  (leitura de `SHTPATH` em EXTRA_AREA); rows 39 e 65 (Zero, Cart) pretas com HUD
  (`SimpleCalcModelMatrix` em armadilha); row 37 lavada em verde; row 46 sem HUD;
  row 71 ceu preto.
- Rows 66-68 (Sky Chase): `zshooting_fix` (reloca os ponteiros de `shtpath*.PRS`)
  NAO resolveu; o acesso invalido em `0x0cb80000` continua, agora em `bullet_draw`
  (`shot.c`, `ldr r3,[r3,#0x28]` sobre `task->work`), chamado por
  `shooting_act1_step`. Hipotese aberta: `work` de uma tarefa de tiro aponta para
  o DC cru. Proximo passo: logar `task`/`task->work` na entrada de `bullet_draw`.
- Orcamento de memoria (relatos de devs do Zeebo, 23/11; nao medido por nos):
  RAM livre varia de 30 a 40 MB conforme o aparelho/revisao; projetar para
  30-32 MB, com folga. NAND = sistema BREW; eNAND = jogos (1 GB, lenta); com
  adaptador microSD ate 32 GB. Um port de RE1 gastou 40 MB so de audio na RAM.
  Consequencia: `.mod` + bss + texturas + audio < 30 MB; o `.mod` atual
  (36 MB) nao cabe.
- Relatos de devs (23/11, 15h30): (1) o Zeebo nao tem VFP; binario com VFPv2 da
  watchdog. Nosso `.mod`: `-mfloat-abi=soft`, 0 instrucoes VFP no ELF (conferido
  por `objdump`). (2) escrever na NAND (sistema) pode matar o aparelho; save so em
  area do jogo na eNAND. Nosso `sa1vmu.bin` e relativo (`IFILEMGR_OpenFile`):
  confirmar no hardware que cai na pasta do modulo. (3) risco de RAM no audio:
  `zaudio.c` decodifica ate 40 s de ADX para WAV na RAM (~7 MB a 44,1 kHz
  estereo); trocar por stream em blocos antes de ligar `AUDIO=1`.
- Simulador BREW (`~/Downloads/Telegram Desktop/brew_sdk.zip`, SDK generica com
  `Simulator.exe`, `OpenGLESExtension.dll`, DevicePack1-4; sem device do Zeebo):
  roda DLL x86, nao o `.mod` ARM. Validacao secundaria possivel com
  `i686-w64-mingw32-gcc` + `wine` (ambos instalados), mas exige build nativo x86
  do applet e do jogo. Nao iniciado; W21 se as telas pretas e a RAM ficarem sob
  controle.
- Port de GTA III da Esther (`github.com/SterTheStar/gta3zeebo`, estudado em
  23/11 16h; so ler, nao copiar codigo — a licenca e do GTA3/re3): tecnicas a
  aproveitar (W22-W27): (W22, FEITO 23/11: `zhid.c`; abre o controle no zeebx; botoes e analogico somam ao EVT_KEY; analogico nao validado, o zeebx so injeta botoes) controle completo pelo **IHID** (16 botoes do Z-Pad
  por UID `0x0106c3fe..0x0106c40d` + 4 eixos `0x0106c4ce..d1`, polling de
  `GetNextButtonEvent`/`GetPositionState`; hoje so chega EVT_KEY, sem analogico nem
  botoes 2-4/gatilhos; o simulador inverte Y); (W23) **purga na falha de malloc**:
  `malloc` -> `pfnMalloc`; se `NULL`, libera cache de audio e modelos menos usados e
  tenta de novo (a ideia de "limpar a RAM"); (W24, FEITO 23/11: log `mem livre/minimo livre/maior bloco` a cada 2 s em `app.c`; sem painel na tela; zeebx tem heap de 64 MB, o real e 30-32 MB) painel de memoria/perf
  (`pfnGetRAMFree(&total,&largest)`, marca d'agua de minimo livre, toggle Start+Down);
  (W25) orcamento de audio deles: cache de SFX 1 MB, 32 slots, max 384 KB por SFX,
  12 vozes, musica/radio como MP3/WAV pelo `IMedia`; (W26) filmes MPEG-1 com
  `pl_mpeg` (MIT) direto em textura RGB565 com skip por botao — o SFD do SA1 e
  MPEG-1 + ADX; (W27) EGL RGB565 + DEPTH16 sem stencil (o nosso pede stencil 4:
  testar sem). Aviso: o README deles diz "VFPv2 softfp"; devs dizem que VFPv2 da
  watchdog no Zeebo real — o nosso e soft. `zeebx` so mapeia 1 MiB de bss alem do
  arquivo (`MODULE_BSS_SLACK`); eles materializam o bss no `.mod` (`.bss_pad`).
- Linha 68 (Sand Hill, preta e lenta, ate 6,7 s/quadro): descartados heap
  (7 de 64 MB), leitura de arquivo (nenhuma > 50 ms) e musica (`ADXT_StartFname`
  nao e chamado); sobra CPU numa tarefa filha de `stream_step`. Pausado em 23/11.
  Armadilha: `make` nao rastreia `DEBUG_TITLE`; apague `title.o` e `main.o` ao
  trocar o valor, senao o menu de debug nao entra (tela de titulo no lugar).
- Otimizacoes a partir da sessao DeepSeek (conferidas no Developer Guide): ver
  `roadmap-otimizacoes-deepseek.md`; transcricao em `deepseek-sa1-zeebo-sessao.md`.
- Medida de outros jogos (fullset, 62 jogos): maior `.mod` = 8,7 MB (Quake II),
  mediana 0,5 MB. Developer Guide 0.97: o binario inteiro vai para o heap de 32 MB.
  Meta de `.mod`+bss residente: ~15 MB (hoje 36,45 MB + bss 13,6 MB).

#### 20 quick wins (abertos em 23/11 18:50; P <= 1 h, M <= meio dia) — superada pela lista U1-U20 de 20:56

Numeracao nova (**T**, de tarefa). A lista N1-N20 das 16:50 fica para tras: N4
(os maiores programas), N18 (IHID) e N20 (RAM no log) estao fechados — ver o
paragrafo depois da tabela. Os 20 abaixo estao **abertos** e medidos nesta
rodada.

| # | Entrega | Esforco | Evidencia |
|---|---|---|---|
| T1 | Rodar a matriz inteira (76 linhas do menu; so 30 no summary) com o `.mod` de 744 objetos e o IHID novo, e colar o `summary.txt` | M | `tools/stage_matrix.sh:13` (ROWS padrao de 29); `build/zeebo/matrix/summary.txt` (29 linhas de 13:2x + row 15 as 14:18) |
| T2 | 7 linhas no menu de debug: HOT SHELTER, CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R (subir `DEBUG_STAGES` de 76 para 83) | P | `src/progress/title.c:202` e `384-461` (73 linhas + 3 `{0,0}`); os IDs ja existem: `src/stage/stage.h:28,33-40` |
| T3 | `make mod` falhar quando "N de M objetos" vier com N < M | P | `ports/zeebo/game.mk:334` (`echo` de `ls $(wildcard $(OBJS))` + `wc -l`, sem `exit 1`; `game.mk:331` diz "tolera objetos que nao compilaram") |
| T4 | Cortar ~6 MB do `.mod` (36.454.268 B contra 30-32 MB de heap) | M | medido 18:51; `.data` dos 244 programas = 60,5 MiB; `al_main` sozinho tem 633.358 B de `.bss` |
| T5 | `bullet_draw`: logar `task` e `task->work` (a Sky Chase lia `0x0cb80000`) | P | `modules/shooting/shot.c:97-100` (`work = task->work` antes de `work->pos`) |
| T6 | Zero (row 39) e Twinkle Circuit (row 65) pretos com HUD: achar quem chama `SimpleCalcModelMatrix` com ponteiro invalido | M | 13:2x; `build/zeebo/matrix/row39.png`, `row65.png` |
| T7 | Defeitos visuais da matriz: row 37 lavada em verde, row 46 sem HUD, row 71 com ceu preto | M | 13:2x; `row37.log`, `row46.log`, `row71.log` |
| T8 | Chao Garden: rows 69-70 (ALIFE_SS/EG, `al_main` + `al_garden00`) e o ceu preto da row 71 | M | rows 69-70 nunca medidas; `modules/al_main`; row 71 no summary de 14:18 |
| T9 | Culling em software no `send_strip` com quadro fixo (`FRAME=`) e contagem de faces descartadas | M | `zlive.c:340` (sem culling); as 3 chamadas (`zlive.c:416,421,426`) precisam passar `cull` |
| T10 | Save no campo: gravar/ler `sa1vmu.bin` dentro de Station Square (a data agora vem do RTC) | P | `cdd0c24`; `b7865dd` (RTC); Fase E3 |
| T11 | Musica em laco (o ADX toca 40 s e para) | M | `zaudio.c:315` (`MUSIC_SECONDS 40`), usado em `zaudio.c:559` |
| T12 | Efeitos e vozes do jogo (`sd_*` devolvem 0) | M | `tools/gen_stubs.py`; A5 |
| T13 | `AUDIO=1`: medir o custo em ms/quadro da musica | P | `game.mk:274` (`AUDIO ?= 0`) |
| T14 | Filme com som e ms/quadro | M | `zmovie.c:25` (`movie_files`), `zmovie.c:217`; SFD = MPEG-1 + ADX |
| T15 | Triar os 691 indefinidos por frequencia no run; os 10 mais chamados viram stubs reais | M | `build/zeebo/game/missing.txt` (1132 nomes) |
| T16 | `PROF=1` no Emerald Coast e atacar a funcao mais cara | M | ~80 ms/quadro; `game.mk:120` (`PROF ?= 0`) |
| T17 | Boomerang no zeebx: porta com dispositivo "Boomerang" e conferir volante/botoes/HOME (teste do Rafael) | P | `docs/boomerang.md:44-46`; `zboom.c`; `tests/boom_test.c` ja da `boom_test: ok` |
| T18 | Purga na falha de `malloc` (libera cache de audio/modelos e tenta de novo) | M | `zlibc.c:12` (`return MALLOC(n)`, sem retry) |
| T19 | Triagem da textura retangular linear (tipo 0x09): o DIAG ja loga tipo/largura/alfa/indice; falta rodar a cena e decidir o caminho | P | `ztexture.c:148-158` (`retangulo *`), na arvore sem commit |
| T20 | Medir o conjunto minimo por area (Station Square + Emerald Coast: `.ovl`, texturas, sons) contra a eNAND — base do pacote instalavel | M | `extract/` = 973 MB; `.ovl` = 32,1 MB; Developer Guide 0.97 (tudo no heap de 32 MB) |

Fechados desde 16:50: **N4** (`.text`/`.data` por programa: 244 objetos, 67,3
MiB, maiores `stg09` 2.348.984 e `al_main` 2.332.106), **N18** (IHID com 16
botoes, 4 eixos e gatilhos — `5f70e0d`) e **N20** (RAM livre/minimo/maior bloco
no log a cada 2 s — `6d5b4b2`, `app.c:193-200`). Fechados antes: **W1** (os 204
eventos; `744 de 744 objetos`), **W3** (overlays por area) e **W7** (`gdFs` loga
o `fname`), e Q1, Q2/Q3/Q4, Q18, Q20. Fora da lista: **Sky Chase 1 e 2** e
**Sand Hill** rodam (`941fd33`; row 68 medida em 16:34).

### Atualização 2026-11-22 (noite) — pose do Sonic: dois XMTRX (resolvido)

Causa raiz do "deitado e grande"/membros fora do lugar: o .mod tinha **dois
XMTRX**. Os `njm-*.o` do applet (SH4ZAM em software) vencem `njRotate*`,
`njUnitMatrix`, `njSetMatrix`, `njGetMatrix`, `njMultiMatrix`, `njCalcVector`;
o asm traduzido (`build/engine/native/asm.c`, banco `X`) fica com
`njTranslate`, `njScale`, `njPush/PopMatrix`, `njCalcPoint`. Rotação ia para um
e translação/projeção saíam do outro. Correção: `ports/zeebo/ninja/zxmtrx.c`
faz `shz_xmtrx_state_()` devolver `native_xmtrx()` (mesmo layout XF0..15 em
colunas); `game.mk` enfraquece o da cópia do applet e linka `zxmtrx.o` forte.
Hash da seleção `7657ff96fe78` → `4e7887e2d0f7` (quadro 1186): Sonic em pé, de
frente, membros no lugar. Resta: modelo escuro e rosto borrado (textura/UV ou
luz no `zlive`). Também em `zlive.c`: quads NJD_MESHSET_4 vão em 0 1 2 3, como
`engine/dc/sa1/model.c` (não exercitado nesta cena). A hipótese do giro de 180°
em Y fica descartada; o Q15 deve ser remedido com o XMTRX único.

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

### Resultado da investigação (2026-11-22, tarde)

- **Q9/Q12 — caminho:** na seleção de personagem o Sonic é desenhado pelo
  `zlive` (`live.present` = `live.desenha` = 4096, `live.meshset` = 16384);
  `njm.*` e `lt.*` têm **zero** chamadas. A luz portada em `lt_arm.c`
  (`5d536c7`, `492168c`) não atua aqui; o `zlive` tem Lambert próprio via
  `draw_set_light`.
- **Q10/Q11 — descartados:** `draw_basic` nunca retorna cedo (nenhum
  `ret_null`/`ret_cull`); `model_in_view` está ok.
- **Q12 — normais:** `model->normals` existe mas é `(0,0,0)`;
  `meshset->normals` é `NULL`; `shade` fica fixo em 0,45. Esse preview não
  passa por `model_calc_face_normals`. Correção em andamento: o `zlive.c`
  calcula a normal de face na hora (`face_shade`); hash da cena
  `8527bc0913e4` → `7657ff96fe78` e o sombreado aparece, mas a **forma/pose**
  do Sonic continua errada (precisa de referência).
- **Q15 — culling via GL não dá:** `glEnable(GL_CULL_FACE)` com `GL_BACK`
  remove o Sonic inteiro; `GL_FRONT` dá o mesmo resultado (o argumento é
  ignorado no IGLES11 do zeebx) e `glFrontFace` não existe no SDK. Culling
  teria de ser em software (área assinada por triângulo no `send_strip`) ou
  por `kmSetCullingRegister`.
- **Q15 revisado — software: teste inválido, refazer:** a chamada do caso
  *quad* (multi-linha) não recebeu o parâmetro `cull`, então a comparação de
  sinais não vale. Reverter; refazer atualizando as **3** chamadas e usando
  quadro fixo (`FRAME=`) + contagem de faces descartadas. Culling fica de fora
  até lá.
- **Disco:** `--dump-gl` grava ~1,1 GB por run de 62 s; o
  `~/.cache/sa1z-check` estava com **7,6 GB** e o disco em 98%. Um run falhou
  com `No space left on device`. A limpeza de 7,3 GB levou o disco a 96%
  (21 GB livres) — provável contribuinte do travamento junto com o run longo.
  O `tools/frame_hash.sh` apaga o dump no fim.

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
| C3 | Um modelo real com T&L de hardware e matriz do Ninja em 16.16 | pose corrigida (dois XMTRX unificados, `zxmtrx.c`); falta textura/luz do preview |
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

## Atualização 2026-11-23 19:48 (noite) — 20 quick wins QW1-QW20 (superada pela lista U1-U20 de 20:56)

Objetivo fixo: port completo do Sonic Adventure 1 no Zeebo, todas as fases e
todo o conteúdo, validado no zeebx. O roadmap é atualizado a cada 2 h (cron
`9f2e68aa0549`). Estado de partida: 744 de 744 objetos, `duplicatas: 0`,
`host-selftest PASSED`, build de código em ~41 s (`docs/pdca-build.md`).
Esforço: P ≤ 1 h, M ≤ meio dia.

| # | Entrega | Esf. | Estado / evidência |
|---|---|---|---|
| QW1 | `kmLoadTexturePart` real + `TextureSize` na superfície (texturas de arquivo `.PVR` subiam vazias) | P | FEITO (`a79f8c7`): Emerald Coast jogável com HUD e caixa de missão da Tails |
| QW2 | Letreiro da fase e textos do CHAOS contra a polícia (quadrados brancos) | P | caixa de missão corrigida; falta achar o letreiro antes do cronômetro e testar a cena da chuva |
| QW3 | `TRAP OPEN` em Emerald Coast: o loader pede `am_sea83a_32.PRS` / `ea77b_32.PRS` (nome muda entre runs, parece lixo) | M | achar o chamador de `loader_must_read_prs` |
| QW4 | `zlog`: cota por categoria | P | FEITO (23/11): 60 linhas por categoria, 24 categorias |
| QW5 | Stubs de `zkamui.c` que afetam imagem: `kmSetUserClipping`→`glScissor`, `kmSetFog*`→`GL_FOG`, culling | P–M | Q22/Q23/Q15 antigos |
| QW6 | `kmGetTexture`, `kmGarbageCollectTexture`, `kmGetFreeTextureMem` com valores reais | P | ainda STUB |
| QW7 | Linha 39 (EGGMANROBO/Zero, tela preta) | M | `TRAP READ` CORRIGIDO (23/11): `eggmanrobo_bss_end`/`e101_r_bss_end` (de `bounds.src`, asm do SH-4) eram dados zerados e o loader lia 300 KB no endereço 0; agora apontam um buffer em `zgame.c`. Roda 865 quadros com HUD e título "ZERO", mas a cena 3D segue preta |
| QW8 | Linha 65 (verde) | M | MELHOROU (23/11): pista escura com HUD (tempo, VOLTA 1/3); falta luz/cor |
| QW9 | Linha 37 (sem HUD) | M | MELHOROU (23/11): arena de chefe com HUD (anéis, "Target") |
| QW10 | Linha 46 | M | segue escura, sem HUD claro; retângulo pálido no canto superior direito |
| QW11 | Linha 71 (céu preto) | M | segue: cena parcial sem céu/HUD; traps `NJM_SET_TEX_X_STRIP_START/END` |
| QW12 | Linha 68 (Sand Hill) | M | pausada por pedido do usuário |
| QW13 | Recompilação a frio para ativar os `.d` (header rastreado) | P | pede autorização para apagar os `.o` |
| QW14 | Build: tirar o `make all` recursivo do `link` (11 s) e a passagem dupla de `ld -r` (15 s) | P | `docs/pdca-build.md`, ciclo 2 |
| QW15 | Build: `sa1g_debug`, `sa1g_normal`, `sa1g_audio` num comando só | M | hoje são 3 builds |
| QW16 | Analógico do DS4: sinal do "cima" no Z-Pad real | P | não validado |
| QW17 | Boomerang: medir `ZBOOM_STEER_SIGN` e a escala do volante | P | só testado no PC |
| QW18 | Áudio em segmentos (2 `IMedia` alternados): música 7 MB e filme 21 MB não cabem inteiros | M | bloqueia o som no aparelho |
| QW19 | Formato de `SONICADV_INT` (dump US) × `SONICADV_SYS` (JP) para importar o VMU | M | não verificado |
| QW20 | `.mod` de 36,4 MB → 30–32 MB (orçamento do aparelho) | M | overlays já cortaram 65,8 → 31,0 MB de RAM residente; falta o pacote |

Regras desta rodada: sem features nem cheats; só correção e desempenho.
Gates a cada mudança: `integrate` com `duplicatas: 0`, `host-selftest PASSED`
e "744 de 744". Build por `ports/zeebo/tools/build.sh` (serializa com `flock`).

Feitos fora da lista em 23/11 (noite): probe de extensões GL no log (`tgl`; o zeebx
expõe ATITC), gate `novfp` ligado ao `check.sh` (CHECK_GAME=1), `bss_end` de
b_robo/b_e101_r (QW7).

Achado: `CHECK_GAME=1 ./check.sh` FALHA em "jogo: não chegou à seleção do cartão
(ava_vmssel)". O `sa1g_normal.mod` de antes da correção de textura falha igual
(o jogo chega ao menu, `ava_gtitle0`, mas não abre `ava_vmssel` com as teclas
14000/18000/22000/26000 ms nem com mais 3 botões até 38 s), então não vem das
mudanças de hoje. Suspeita: o caminho do menu mudou com o RTC (`b7865dd`) ou com o
cartão emulado. Fica como QW21: achar o commit (bisect) e ajustar o gate ou o jogo.

QW22 (pedido do usuário, 23/11): resolução dinâmica do 3D (`DYNRES=50|75`, padrão 0).
  Ressalva do Zeebo Developer Guide 0.97 (cap. 3 e 7): o Adreno 130 e um renderizador por *binning* (8 bins no VGA; Z, stencil e pixel buffers na SRAM de 256 KB; cor/Z em EBI). `glCopyTexSubImage2D` no meio do quadro obriga a resolver o color buffer (copiar da SRAM para a EBI) e depois ler como textura: um segundo passe de cor, so para esticar. Alem disso o gargalo do SA1 deve ser a CPU de 528 MHz (clipping e transformacao por software, 1,6 M tri/s), nao o fill rate (63 M px/s). Conclusao provisoria: `DYNRES` provavelmente nao ajuda no aparelho; manter em 0 e medir com `FPSHUD=1` (DYNRES=0 x 50) antes de qualquer decisao. No zeebx em software o `DYNRES=50` ficou mais lento (a cópia custa mais do que os pixels poupados). Doc recomenda RGB565 na copia (ja usado) e textura <= 1024.
Implementada em `zstrip.c` (`dr_begin`/`dr_present`, `Z_DYNRES`): o quadro do jogo vai
para um viewport de 50%/75% de 640x480; os strips até z=1/3 (o 2D do jogo tem z >= 1/3
e ordena por último) saem ali, a região é copiada com `glCopyTexSubImage2D` para uma
textura 512x512 RGB565 e esticada sobre o painel; o 2D restante vai por cima em
resolução cheia. Uso: `DYNRES=50 ports/zeebo/tools/build.sh` (apagar `zstrip.o` antes
de trocar o valor, o make não rastreia a flag; `touch zstrip.c` basta).
NÃO validada: o zeebx não implementa `glCopyTexSubImage2D` (não há a chamada em
`src/machine/gl.rs`), então com `DYNRES=50` o 3D sai branco no emulador (o HUD sai
nítido, em resolução cheia). Para validar, falta (a) acrescentar essa chamada ao zeebx
ou (b) testar no aparelho. O ganho de desempenho também só se mede no aparelho: o
gargalo medido no zeebx é a CPU (~56 ms de game code por quadro), e a resolução só
alivia a fill rate. Build padrão (`DYNRES=0`) confere: 744/744, 734 quadros, `host-selftest PASSED`.


### Atualização 2026-11-23 20:50 — DYNRES pela extensão do SDK e overlay de FPS

- QW22 refeito: `DYNRES=50|75` agora usa `EGL_QUALCOMM_surface_scale` (Developer Guide 0.97, 8.3; `platform/brew_egl.c`, via `eglGetProcAddress`): o jogo renderiza num recorte menor e o display escala no swap, sem cópia de textura. A versão por `glCopyTexSubImage2D` (4e5d70b) foi removida: no zeebx era mais lenta e gerava um `TRAP OPEN` na carga da fase. Escala o quadro inteiro (HUD inclusive). No zeebx (v0.6.0 + correção) o Emerald Coast aparece inteiro, na orientação certa, 734 quadros e sem TRAP; o zeebx não mede o ganho (o relógio virtual não conta o custo de pixels: 734 quadros nos dois). O ganho so se mede no aparelho. Padrão continua 0.
- Bug achado no zeebx: `IEGLSurfaceManip::SetSurfaceScale` escrevia o `ret` no argumento 4 (o `dst`) em vez do 5; com `dst` nulo nada era gravado e o jogo via lixo e descartava a extensão. Corrigido em `src/machine/misc.rs:69` com teste.
- QW23 (feito): overlay `FPSHUD=1` (`zhud.c`; FPS, ms de jogo/quadro, RAM livre; fonte 3x5 propria; S e 5 saem iguais) e `tools/fps_matrix.sh` (tabela por fase no zeebx; mede a emulação, não o aparelho).
- Nota de ambiente: o zeebx usa a GPU local com `EGL_PLATFORM=surfaceless` (a plataforma GBM padrão falha neste shell); `run` é sempre software.
- Pendente: medir `DYNRES=0` x `50` no aparelho com `FPSHUD=1`; no upstream/development o port não passa do menu de debug (entrada), a investigar.

## Atualização 2026-11-24 (madrugada) — débito técnico e quick wins novos

**QW3 (`TRAP OPEN` com nome de arquivo lixo) não era bug do jogo.** O nome lixo
era texto Shift-JIS: um `.mod` copiado sozinho rodava com o `.ovl` de um build
mais novo (`build/zeebo/*.ovl` é reescrito a cada build) e os ponteiros eram
relocados errado. Fechado com `tools/keep_build.sh NOME` (`b4de392`), que guarda
`.mod` + `.ovl` + hardlinks dos dados. Nenhuma comparação entre builds vale sem
isso: os `TRAP OPEN` atribuídos ao `DYNRES` antes de 24/11 têm essa origem.

### Débito técnico (decidido: fica registrado, sem prazo)

| # | Débito | Por quê | Estado |
|---|---|---|---|
| D1 | Captura de quadro no zeebx (`--capture-frame=N`: chamadas GLES do jogo, estado, binds, vértices por draw; mapa de overdraw do rasterizador de software) | Ferramentas FOSS (RenderDoc, apitrace) só veriam o GL do host, não as chamadas GLES 1.1 do jogo; o zeebx já tem `--trace` e `--dump-gl` | só avaliado |
| D2 | Contadores por quadro no port (draws, strips, vértices, trocas de textura, estado GL redundante) no `FPSHUD` e no log | mede o ganho de cachear estado e de reduzir strips sem aparelho | só avaliado |
| D3 | Adreno Profiler (QXProfiler beta, Win32) do SDK: `ZeeboSDKPackage-1.2.4/AdrenoProfiler/` | única ferramenta que vê o Adreno 130 real; exige aparelho com build de métricas | não testado, sem aparelho |
| D4 | `.ovl` com identificação do build (o `.mod` recusa um `.ovl` de outro build) | a checagem pela distância `gdFsOpen`–`zovl_region` não pegou o caso | tentativa descartada |
| D5 | `session --gpu` do CLI passa `None` como contexto GL (`frontends/cli/src/main.rs`) e, com contexto, o `session` fica não determinístico (o mesmo trecho de 60 s terminou em ~2 s ou em ~21 s; nas de ~2 s não vi `game_mode 4` e **não investiguei se o processo caiu ou se só termina antes**) | nenhuma comparação de custo na placa é confiável | patch local, fora dos PRs |

### Quick wins novos (abertos em 24/11; P ≤ 1 h, M ≤ meio dia)

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| V1 | Cache de estado GL no passe translúcido (`glEnable/glBindTexture/glBlendFunc` só quando mudam) | P | `zstrip.c:378-387`; o passe opaco já agrupa (`opaque_flush`, `zstrip.c:217`) |
| V2 | Ordenação do passe translúcido: inserção O(n²) até 1024 strips → algo melhor | P | `zstrip.c:361-376` (laço de inserção em `zstrip_flush`); medir o tempo do `zstrip_flush` antes (V8) |
| V3 | `-O2/-O3` em `zstrip.c`/`zkamui.c` (o jogo usa `-O3`, o port `-Os`) | P | `game.mk:275` contra `Makefile:83` |
| V4 | Texturas em 16 bpp (RGB565/RGBA4444) em vez de RGBA8888 | M | `zstrip.c:578` (`glTexImage2D(GL_RGBA)`); o Developer Guide 0.97 (linha 1270) diz que converter para RGB565 de 16 bits é desejável para desempenho; muda a imagem: pede aprovação; guia 0.97 recomenda RGB565 |
| V5 | `GL_ALPHA_TEST` nos recortes (alpha 0/1) em vez de blend | M | só aparece desabilitado em `zstrip.c:774`; descobrir onde caem os recortes |
| V6 | Descarte antecipado (frustum) por strip antes de enviar | M | = U10, mesma ideia vista no Mania/DC |
| V7 | Ruído `gdFsOpen MISS SL_00B.bin / PL_00B.bin` (GPU, sessão): são ausentes de verdade ou o jogo pede arquivos opcionais? | P | log de 24/11; vistos só nas sessões com GPU |
| V8 | Medir o custo dos strips/frame: tempo do `zstrip_flush` por quadro no `FPSHUD` | P | pré-requisito de V1, V2, V5 (D2) |
| V9 | Tornar o `session` determinístico (o mesmo trecho em tempo virtual fixo), ou usar `--frames=N` | P–M | D5; hoje bloqueia medir `DYNRES` na placa |
| V10 | PR no zeebx: `session --gpu` do CLI cria o contexto offscreen | P | patch local em `~/projects/zeebx-emu-v060check`; falta rebase em `upstream/development` e depender de V9 |
| V11 | Acompanhar os PRs ZeebxTeam/zeebx-emu #136 e #137 (revisão, rebase) e investigar por que o upstream não passa do menu de debug | M | `game_mode 2` no `upstream/development`; v0.6.0 vai a `game_mode 4` |
| V12 | Cópias soltas `sa1g_def.mod`/`sa1g_dr50.mod`/`sa1g_miss.mod`/`sa1g_chk.mod` em `build/zeebo/`: não servem mais sem `.ovl`; trocar pelas pastas `keep/` | P | nada é apagado sem pedido; só registrar |

- **V8 e V1 feitos (24/11):** a linha `vprof` do log ganhou `desenhos` e `estado repetido` (por janela de 2 s), e o passe translúcido só troca o estado GL quando muda (`STATECACHE=1`, padrão; `STATECACHE=0` desliga). Medido no zeebx (v0.6.0 + correção do `SetSurfaceScale`, mesmo trecho de 100 s virtuais, 734 quadros nos dois, builds guardados com `keep_build.sh`): `BlendFunc` 344.148 → **5.358** (−98%), `Enable` 687.787 → 344.958 (−50%), `BindTexture` 600.859 → 450.025 (−25%), `DrawArrays` igual (516.609). O último quadro é idêntico byte a byte (`md5` `4e1d655324ad`). O ganho em ms só se mede no aparelho: o zeebx em CPU não paga o custo da troca de estado do driver do Adreno.

- **V3 feito (24/11):** `HOTOPT ?= -O2` em `zstrip.c` e `zkamui.c` (`Makefile`). Zeebx, mesmo trecho de 100 s: `-Os` 31.047 M instruções, `-O2` 30.483 M (−1,8%), `-O3` 30.459 M (−1,9%); 734 quadros, 0 traps e último quadro idêntico (`md5` `4e1d655324ad`) nos três; `.mod` +1 KB (`-O2`) e +2,1 KB (`-O3`). Gate: 744 de 744, `duplicatas: 0`, `novfp` 0 instruções VFP. O ms/quadro real só se mede no aparelho.

- **V7 fechado, não é defeito (24/11):** `gdFsOpen MISS SL_00B.bin / PL_00B.bin` (e `SL_01B`/`PL_01B`) vêm de `light_read_disc_sources/palettes` (`src/save/card.c:2635-2700`), que tenta `SL_<fase><ato>B.bin` e, se falhar, o ato 0. O disco não tem nenhum arquivo `SL_0*`/`PL_0*` (a fase 0 não tem luz própria; os da fase 1 em diante existem, ex. `pl_10b.bin`). `light_read_disc` abre uma vez e devolve `FALSE` sem repetir nem esperar, então não custa tempo. O ruído só aparece no log; nenhuma ação.

- **V11 causa achada (24/11):** o `upstream/development` não trava por regressão do emulador. O `zeebx run` dele só ajusta o pad e não traduz o controle em teclas BREW; o menu de debug do port lê `EVT_KEY` e fica em `game_mode 2`. O branch do fork `sa1-port-0.6.0` (3 commits de CLI: `run` traduz o controle em teclas BREW, `ZEEBX_PC_SAMPLE`, botões 3/4/HOME como SOFT2/SEND/SOFT1) resolve. Confirmado: os mesmos 3 commits sobre `upstream/development` (branch local `feat/cli-run-controller-keys` em `~/projects/zeebx-emu-copytex`, não publicado) levam o port a `game_mode 4` (734 quadros, sem trap). Para testar o port no zeebx de desenvolvimento use esse branch; um PR ao upstream é opcional.
- **Novos (24/11, `docs/licoes-re4dc-opence.md`):** V13 hash de estado por quadro como gate de otimização; V14 texturas ATITC (Developer Guide 0.97, 8.2; o zeebx já implementa); V15 pular `disp` em ticks atrasados (hipótese).

- **V13 feito (24/11):** `estado hash` no log (hash rolante FNV do que o jogo mandou desenhar: contagem, 1º vértice e textura de cada strip; registrado a cada 64 quadros em `app.c`) e `tools/hash_gate.sh A B` compara dois builds guardados com `keep_build.sh`. Verificado: duas execuções do mesmo build dão a mesma sequência; `STATECACHE=0` e `1` dão a mesma (11 pontos); uma entrada diferente muda o valor. Cobre o que é enviado ao GL, não o estado de lógica fora do desenho.

- **Correção das referências (24/11, auditoria):** as linhas de `zstrip.c` citadas em V2, V4 e V5 estavam desatualizadas depois das mudanças do V1/V8/V13 (+~50 linhas) e foram corrigidas; V1 não tem mais referência (feito). No V3 a comparação é `game.mk:275` (`OPT ?= -O3`, o jogo) contra o `-Os` do port.

- **Quick wins 2–6 e 8 feitos (24/11):**
  - Espelho de estado GL em `zstrip.c` (`gl_blend`, `gl_dmask`, `gl_tex2d`, `gl_mm`, `gl_arrays`, projeção do modelo): só emite a chamada quando o valor muda; zerado no começo de cada quadro (`gl_shadow_reset`). `glLoadIdentity` antes de `glLoadMatrixx` removido (redundante). `STATECACHE=0` mantém as chamadas antigas (A/B).
  - Medido no zeebx (100 s, 734 quadros nos dois): `VertexPointer` 173.678 → 1.331 (idem `TexCoordPointer`/`ColorPointer`), `MatrixMode` 345.666 → 4.814, `LoadIdentity` 173.344 → 2.578, `LoadMatrixx` 341.532 → 171.106, `DepthMask` 369.073 → 139.932, `Enable` 344.958 → 74.776, `Disable` 285.976 → 74.421; `DrawArrays` igual (516.609). Total ≈ 1,9 M chamadas GL a menos. Último quadro idêntico (`md5` `4e1d655324ad`) e mesma sequência de `estado hash`. Instruções do guest: 30.499 M → 30.503 M (+0,01%): o ganho é em chamadas que cruzam para o driver, **não** em instruções do ARM; ms no aparelho não medidos (U18).
  - `build.sh` guarda as flags de ambiente em `build/zeebo/.flags` e força a recompilação dos `.c` do applet quando mudam (um build com `STATECACHE=0` tinha deixado o `zstrip.o` antigo no build seguinte).
  - `tools/hash_ref.sh [check|update]` + `tools/estado_hash.ref` (11 hashes + contagem de quadros); ligado ao `check.sh` com `CHECK_GAME=1`.
  - `sa1z*.bmp` deixaram de ser versionados (são saída das execuções).
- **Correção:** `hash_gate.sh` removia o número de quadros da comparação (o `sed` apagava `734`); agora compara.

- **Pendências tratadas em 24/11 (balanço):**
  - Feitos: espelho do `BindTexture` (450.025 → 371.360 chamadas, imagem e hash iguais); hash de estado inclui os modelos 3D (matriz + 1º vértice; `estado_hash.ref` regravado); PR #138 (`run` traduz o controle em teclas BREW, só o 1º commit, em inglês, testado: `game_mode 0→1→2→4`); corpo do PR #137 sem a referência ao port; `zboom.*`/`zhid.*`/`tests/boom_test.c` comitados (o Makefile já os referenciava e estavam fora do git); `gen_ovl.py` e `hash_ref.sh` deixam de acumular diretórios temporários (eram 6 GB).
  - **TEX16=1** (opt-in, padrão 0): texturas em 16 bpp (opaca → 565, alfa só 0/255 → 5551, resto → 4444). Em 25 s de jogo a imagem difere do RGBA8888 em média 0,016/255, máx. 9 níveis, 0,35% dos pixels (4444 que vira 565 perde 1 bit); 459 contra 460 quadros. Memória de textura cai à metade por construção; ms no aparelho não medidos (U18).
  - Sem ganho (medido): `-O2` em `ztexture/zthreads/zfile/zsys/zgdfs/zrebase` (+0,02% instruções); V2, a ordenação por inserção do passe translúcido (~0,4% das instruções, 16 M deslocamentos em 100 s).
  - `OPT=-O2 -fno-tree-vectorize` no código do jogo (build frio, `.mod` 32,27 MB contra 33,54 MB; ~8% menos ms por quadro em gameplay no zeebx: 80,7 → 74,2): **não adotado**, porque o tempo de jogo muda com a velocidade e os hashes divergem a partir do início da fase (os 4 primeiros pontos, do menu, são iguais); não há prova de equivalência de gameplay.
  - `session --gpu`: execuções terminam com `rc=0` em ~3 s de relógio para 60 s virtuais e só ~31 quadros; o tempo virtual avança sozinho. Não é queda do processo, mas não serve para medir custo (V9/V10 explicados em parte).
  - Não feitos, com motivo: V14 (ATITC) exige um codificador/`QXTextureConverter` que não temos no host; V5 (`GL_ALPHA_TEST`) pode mudar a imagem e não há medida possível sem o aparelho; V16 depende de um profile por função confiável (a simbolização de `PROF=1` repete `+0x1b8` em vários símbolos e não é confiável); `ava_vmssel`, `.mod` acima do orçamento e as cenas pretas (39, 46, 71) seguem abertos.
  - Incidente: `build/` do decomp (`sa1-zeebo/thirdparty/sonic-adventure/build`, alvo do symlink `build/gen`) foi apagado por engano às 23:59; o build do port saiu com `741 de 744 objetos`. Restaurado com `make pc -j4` (12 min 44 s); depois: `744 de 744`, `duplicatas: 0`, `novfp` 0 VFP, `host-selftest` PASSED, `hash_ref` igual. O `.mod` do padrão ficou em 36,96 MB (era 36,77 MB antes, 33,54 MB durante o incidente, saída idêntica): o tamanho depende do estado do `build/gen`, causa exata não identificada.

- **U6 / T6 / T7 (24/11, `2364e53`):** a causa das cenas pretas era o `njModelS.c` (desenhador "simple" do Ninja): ele escreve cada vértice na store queue do SH-4 por 12 emissores em asm (`NJM_SET_*_STRIP`, `SimpleCalcModelLight`) que aqui eram armadilhas, e `SimpleCalcModelMatrix` devolvia 0 sem escrever a saída. Só `njSimpleDrawModel`/`njSimpleDrawObject` são chamadas de fora do arquivo. `njModelS.c` saiu do build (como o `njModelE.c`) e `zlive.c` define as duas funções pelo caminho dos "easy": 743 de 743 objetos, armadilhas de função 29 → 10, indefinidos 686 → 559. Fase padrão: último quadro idêntico pixel a pixel (734 quadros). Efeito medido (frame vistos): row 39 (Zero) deixou de ser preta mas a geometria ainda está errada (campo teal listrado, sem o robô); row 71 (Chao Garden) ganhou céu, grama e pedra; row 46 mostra Sonic e a pista (canto superior esquerdo ainda preto); row 65 parou no menu de debug com a sequência de teclas usada (a sequência para a corrida de carrinho não foi refeita). Iluminação "simple" não é a original (usa a do `draw_basic`).
- **Release (24/11):** `~/zeebo-roms-fullset/Sonic Adventure (Port)/` (pasta lida pelo zeebx standalone 0.6.0): `sa1g.mod` + `sa1g.mif` + `.ovl` copiados, arquivos do disco em hard links (o zeebx 0.6.0 ignora symlinks), build de `2364e53` com `AUDIO=1` (música; sem efeitos nem vozes). Rodou de dentro da pasta: 1324 quadros em 90 s, ADX e `ava_vmssel`.

- **Auditoria do áudio (24/11):** histórico: `a273b75` música pelo IMedia (ADX decodificado para WAV em memória), `15893c8` som dos filmes (stream 0xC0), `27ce555` filmes mais rápidos, `187ce77` **som desligado por padrão** (`ZAUDIO_OFF` quando `AUDIO=0`), `3b5e2af`/`0771bba`/`b7865dd` só logs. Defeito achado: `build.sh` não recompilava `zaudio.c` quando `AUDIO` mudava; o `zmovie.o` (que o `game.mk` força a cada build) saía com som ligado e o `zaudio.o` (applet) ficava com `ZAUDIO_OFF` do build anterior: o log mostrava `movie us audio extrai` e `adxt start`, mas `--dump-audio` dava 80 s de silêncio (`peak 0.000`) e nenhuma chamada a `IMedia::`. Corrigido (`build.sh` toca `zaudio.c`/`zlive.c`; `app.c` loga `audio ligado`/`audio DESLIGADO`). Depois da correção: `peak 1.000`, áudio do filme em ~4–16 s e música do título em ~40–56 s, silêncio depois; 3 `IMedia::Play`.
