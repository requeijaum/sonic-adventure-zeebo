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
| Q23 | `kmSetFog*` → `GL_FOG` | **feito (24/11, primeira versão)** | `zkamui.c`, `zstrip.c` |
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

- **Subsistema de som (24/11):** música e áudio dos filmes tocam como MP3 pelo DSP (`tools/adx2media.py` gera `nome.mp3` e `nome_lp.mp3` do laço, e `event_adx.zva` com as vozes em IMA ADPCM; `zaudio.c` encadeia faixa->laço no `MM_STATUS_DONE`; `zADXT_StartAfs/SetOutVol/Stop` via objcopy em `game.mk`; sem os arquivos cai no mixer). `zmix.c` = mixer por software (ISource + IMediaPCM, ADX em streaming, 16 vozes PCM) para efeitos e fallback. Medido (`--dump-audio`): `title2.mp3` inteiro (RMS ~9000, igual ao ffmpeg); filmes tocam. Por que não só o mixer: o zeebx guarda 0,5 s por stream e, com o jogo a ~5 quadros/s, descarta o atraso acumulado (2 s de 13 s audíveis); o gravador do zeebx também só liga depois dos primeiros quadros (stream aberto no início sai mudo). Não verificado: vozes (`.zva`; rows 15/40 não chamam `ADXT_StartAfs`), laço `_lp` numa faixa de stage, volume do `ADXT_SetOutVol`. Pendente: efeitos (MLT `SMLT/SMPB/SMSB` + `masaru.drv`, só os cabeçalhos reconhecidos), analógico (sem como injetar no `zeebx run`), ATITC (sem codificador).

- **Efeitos sonoros (24/11, estratégia do backend de PC):** o `engine/pc/sound.cpp` (HLE do MASARU.DRV: bancos SMPB/SMSB, ADPCM do AICA, envelopes, DSP) roda no host: `tools/sfx_render.cpp` (g++ com `engine/pc/sound.cpp` + `dsp.cpp`) carrega cada MLT (registros `SMLT`: tipo, número, endereço, deslocamento, tamanho) e renderiza uma nota (tecla 60, vel. 127) por programa e cada sequência, mono 22,05 kHz IMA ADPCM, até o silêncio (máx. 12 s / 30 s), gravando `<mlt>.zsf`. 113 MLT, 226 MB (`build/zeebo_sfx`, fora do git; copiar para `build/zeebo`). No jogo, `zaudio.c` define o `sdMidi*` (OpenPort/SetMes/SendMes/Play/Stop/SetVol) e `sdMultiUnitDownload` (registra o .zsf do último MLT aberto, via `zgdfs_last_name`); notas de canal (CC32 banco, PC programa, note on) e sequências viram clipes tocados por um pool de 3 IMedia (WAV em memória). Verificado em zeebx: row 3 e o menu tocam clipes (`RIFF` de 3-134 KB nos `SetMediaParm`), áudio no `--dump-audio`. Limites conhecidos: a tecla do note-on e o pitch bend são ignorados (um clipe por programa), pan não aplicado, 17/179 clipes "sustentados" repetem o clipe inteiro (emenda audível), polifonia 3, volume só no início e nos CC7/`sdMidiSetVol`. Falta: validar clipe a clipe contra o PC (`play`), e conferir se os `sdMidiSetMes` com argumentos variáveis chegam certos em jogo longo.

- **Verificação de efeitos e vozes (24/11):** os logs `tsfx` funcionam (o `head` do teste anterior cortou as linhas). Row 3 e row 40: 5 e 20 clipes tocados, nenhum `sem clipe` (todo (tipo, banco, número) pedido existia no `.zsf`). Voz: no menu normal o jogo pediu `ADXT_StartAfs` flid 0 e o `.zva` tocou nativo; o trecho achado no `--dump-audio` a 61,1 s tem correlação 0,88 e ganho 0,98 com a decodificação da entrada 0 do `event_adx.zva` (ffmpeg), com outro áudio misturado por cima. Falta: comparar um clipe de efeito do mesmo jeito, vozes de evento longo, e o pitch por tecla.

- **Efeitos sustentados (24/11):** `sfx_render` parte a nota que ainda soa aos 12 s em introdução (4 s) + trecho de laço (comprimento de melhor correlação entre 0,5 e 5 s, fade de 20 ms para o material anterior ao laço), entrada extra `kind|0x80`; o jogo toca a introdução e, no `MM_STATUS_DONE`, o mesmo IMedia passa ao laço com `MM_PARM_PLAY_REPEAT` 0 (visto em zeebx: 44 860 bytes aos 17,4 s, 17 212 / 53 308 aos 21,7 s). `.zsf` de 226 para 166 MB. A tecla do note-on não precisa de clipes por tecla: `midi_key` do jogo vale sempre 60; o que varia é o pitch bend (`sound_slot_set_pitch`), que o IMedia não tem.

- **Controle analógico (24/11), verificado no zeebx:** o `zeebx run` injeta o analógico: `--keys=ms:x+:hold_ms` (também `x-`, `y±`, `z±`, `rz±`, extremo do eixo; o `hold` padrão é 120 ms). Cadeia medida: o IHID entrega `lx` = 127; o `pdExecPeripheralServer` põe em `pad.x1`; o `pad.c` do jogo calcula `pad_sticks[0].stroke` = 0x3f7d8571 (~0,99) e `ang` = 0xac15 (relativo à câmera); e com `x+` de 70 s a 140 s os quadros divergem do teste sem eixo a partir do quadro 1000 (89 % dos pixels diferentes no último), com o Sonic correndo para a direita no HUD (TIME 00:17:50). Um teste anterior com o eixo só entre 100 s e 130 s deu quadros idênticos: nessa janela o jogo quase não lia o controle (7 leituras com eixo em 30 s), então um A/B só vale se o eixo cobrir uma fase em que o jogo roda. Não testado: Z-Pad físico, direita analógica (`rx/ry`), zona morta a 16/128 do `norm`.

- **Instrumentação por log (24/11):** a cada janela de 2 s (`vprof_report`), `zaudio_report` e `zhid_report` imprimem só o que mudou, em decimal (`zlogd`, sem cota, em `zlibc.c`). `sa1z: tau ...` (som): sfx tocados/sequências/ausentes/roubados/emendas de laço, erros por causa (abrir, ler, instância, dados, tocar, clipe grande), último clipe tocado e último ausente (`tipo<<16|banco<<8|número`), pool ocupado (de 3), conjuntos `.zsf` carregados, voz pedidas/nativas/mixer, música nativa/mixer/emendas de laço, filme nativo, mixer ativo. `sa1z: tin ...` (entrada): total de leituras do controle e com eixo (a cada 8 janelas), eixo ativo 0/1, mudanças e máscara de botões, último lx/ly/rx/ry, e o que o jogo calculou (`pad_sticks`: stroke x1000 e ângulo). Exemplo real (row 3, `x+` de 70 s): eixo ativo = 1, lx = 127, stroke = 990, ângulo = 44053; ao soltar: stroke 0, eixo ativo 0. Como ler: `grep -a 'sa1z: \(tau\|tin\) ' log`; linha ausente = contador em 0 ou sem mudança.

- **Instrumentação de desempenho, modo e texturas (24/11):** `zinst.c`. `sa1z: tperf` (por janela de 2 s): quadro mínimo/médio/máximo em ms, quadros acima de 100 e 250 ms, e % da janela no jogo, no GL e no swap. `sa1z: tjogo`: entrada em cada modo (`BOOT/LOGO/TITLE/.../MOVIE/MENU_RUN`, em ms desde o início), duração de cada um ao sair (ms e quadros), visitas repetidas e, no `MOVIE`, o `movie_request`. `sa1z: ttex`: texturas vivas e a memória nominal em RGBA8888 (KB), o pico, o equivalente em 16 bpp, ATITC 8 bpp e ATITC 4 bpp (linha de base para decidir o ATITC), uploads por formato e por modo PVR, falhas de decodificação, recargas sem liberar, pedaços de `kmLoadTexturePart` e ms gastos subindo. Exemplo (primeiro minuto): 64 texturas, 1797 KB nominais no logo, 261 KB no filme; ATITC 4 bpp daria 224 KB e 32 KB. Não cobre a textura do filme (`zstrip_movie_upload`).
- **Auditoria (24/11), achados e correções:** (1) 18 das 2088 falas do `event_adx.zva` saíam mudas: o `ffmpeg` recusa o ADX direto para IMA nessas ("frame_size was not respected"), e o `adx2media.py` as tratava como "vazias"; agora decodifica para PCM e recodifica (0 vazias; o ADX delas é válido, 4 a 55 KB). (2) O pool de efeitos tinha 3 IMedia + música + voz = 5 objetos codificados, e o guia do Zeebo (9.2) fala em no máximo 4 ao mesmo tempo; o zeebx não impõe o limite, o aparelho impõe. Agora são 2. (3) `sfx_render` somava `off + len` em 32 bits: 1501 registros de banco têm `data_size` = 0xFFFFFFFF (slot sem dados neste MLT) e o teste de limite estourava; agora em 64 bits e ignora 0xFFFFFFFF (a contagem de clipes não mudou: 4310 programas, 3991 sequências). (4) `hash_ref` estava DIFERENTE por três coisas independentes das minhas mudanças de som: a 4ª marca (quadro 256) cai dentro do filme, cujo quadro mostrado depende da velocidade de decodificação (a referência era de uma build mais lenta: 734 quadros em 100 s contra 2117); ligar e desligar os `sdMidi*` dá modos e marcas idênticos; a referência foi regravada (33 linhas) e `check` deu IGUAL duas vezes.
- **Filme da abertura (24/11):** o log do usuário (`~/Downloads/sa1g.log`) termina em `gdFsOpen MISS MOVIE.BIN` + `TRAP OPEN` no `game_mode` 8, e o zeebx lista `files not found: MOVIE.BIN, movie.bin`: a pasta de onde ele rodou não tinha o `movie.bin`. Reproduzi apagando o arquivo de uma cópia por hard link: mesmo relatório do zeebx. Com o arquivo, a abertura toca (sem teclas: SEGA, Sonic Team, depois o filme com céu, cidade, título, enchente e personagens). O jogo lê o `MOVIE.BIN` por cima de `movie_entry` (função em ROM) e ignora o conteúdo, então só o tamanho importa: `gdFsOpen` agora devolve um arquivo virtual de 185 976 bytes (0x2d678) quando ele falta. Não sei por que o arquivo não estava na pasta dele.

- **PRS não é o gargalo (24/11):** `sa1z: tprs` mede `prs_decompress`: entrando na Station Square foram 34 arquivos, 8719 KB descomprimidos em 380 ms (~23 MB/s; 1373 KB em 42 ms no começo). O que travava era outra coisa: `ADXF_LoadPartition` lendo a tabela do `event_adx.afs` (um quadro de 20 s a cada campo/evento, corrigido) e o upload de texturas (~6 s no total). `ava_*.prs` são os 20 arquivos do menu ADVERTISE (título, seleção de cartão, de arquivo, opções, teste de som...).
- **Vídeo por IMedia (24/11):** o guia do Zeebo (0.97) só lista QCP, ADPCM, PCM e MP3 como objetos codificados e não fala de vídeo; o SDK do BREW 4.0.2 declara `AEECLSID_MEDIAMPEG4` (+7), mas o zeebx trata essa classe como "o mesmo objeto de mídia, que termina na hora o que nenhum decodificador lê" (`src/machine/mod.rs`): não há decodificador de vídeo no zeebx, então AVC/MPEG-4 por IMedia não tem como ser testado nele e não há como saber se o aparelho aceita. Os filmes seguem em MPEG-1 decodificado no ARM (`zplm.c`).

## Atualização 2026-11-24 (tarde) — névoa, frexp e o que sobrou

- **Névoa (Q23/QW5), primeira versão:** `kmSetFogDensity/Table/TableColor` e `njFogEnable/Disable` (`zkamui.c`) guardam o estado em `zstrip.c`; os modelos da via GL (`zlive`) saem com `GL_FOG` linear (`glFogx` mode inteiro, `glFogxv` com 4 componentes e ponto fixo, como o zeebx exige). A densidade guarda a distância longe F (mantissa de 8 bits x 2^expoente; `0x8010` = 65536) e a entrada i da tabela vale a distância F*16 / (2^(i/16)*(16+i%16)); o início e o fim da reta são as entradas onde a tabela passa de 2% e de 98%. Tudo em inteiro (a tabela de floats é lida pelos bits). Verificado no zeebx (Windy Valley, `DEBUG_TITLE=6`): com uma faixa de teste 0-300 o fundo some em branco e o personagem fica nítido (a distância de olho e as unidades batem); com os valores reais (início 135, fim 6496, branco) o efeito é discreto. `sa1z: tfog` mostra F, início/fim e quantos modelos saíram com e sem névoa.
- **Limites da primeira versão:** só os modelos opacos da via GL levam névoa (translúcidos projetados, partículas e o 2D não); a curva da tabela vira uma reta; o céu (fundo) não leva névoa, como no console. Falta olhar Sky Deck, Ruin e Casino, e decidir se a reta de 2%-98% serve ou se vale uma curva (por exemplo, três trechos).
- **Bug achado de passagem: `frexp` do SDK:** `sdk/runtime/frexp.c` lê o `double` como float de 32 bits (no SH-4 com `-fpu=single` o double é float); no ARM o double tem 64 bits e saía lixo. O `njGenerateFogTable3(-7000, -8000)` gerava a densidade `0xff81` em vez de `0xfa0c`. `zlibc.c` ganhou um `frexp` com a largura certa (o do SDK é fraco). Pendência: auditar o resto de `sdk/runtime/` por contas que assumem double de 32 bits.
- **Gate `hash_ref`:** regravado de novo (a ordem dos quadros não mudou: as 33 marcas antigas continuam idênticas, e a corrida agora faz 2720 quadros em vez de 2117 por causa do `ADXF_LoadPartition` ignorado).
- **Pendências em aberto:** filmes em 448 linhas inteiras (hoje pula metade), `mp4probe` num Zeebo real, desempenho do 3D por função, linhas diagonais no fundo do título só no seu zeebx (não reproduzidas), menu de opções voltando ao "Press Start", botão 1 sem Start no teclado.
- **Conferido contra a parte PC do decomp (`engine/pc/shaders/draw.frag`, `sa_engine.h`):** o `fog_alpha()` do PC usa `1/w x densidade`, preso em 1..256, e escolhe a entrada pelo expoente e pelos 4 bits altos da mantissa, ou seja, a entrada i fica na distância F/(2^(i/16) x (1 + (i%16)/16)): é a mesma conta do `zstrip.c`, e o valor da tabela é a quantidade de névoa (mix com a cor da névoa). As tabelas dos dois geradores do SDK (`njGenerateFogTable` e `njGenerateFogTable3`) são lineares na distância, então o `GL_LINEAR` as reproduz exatamente; só tabelas de estágio com outra forma perdem fidelidade. Modos do PVR que a primeira versão não cobre: névoa por vértice (modo 1, `offset.a`) e tabela modo 2 (cor da névoa com alfa da tabela). O flycast não foi aberto: a referência do próprio projeto já respondia.
- **Auditoria do `sdk/runtime`:** além do `frexp` (corrigido), `atan2.c` e `cos.c` também leem o resultado como float de 32 bits; ali só o teste de errno (NaN/infinito) fica errado no ARM, e o valor devolvido está certo. Sem correção por ora.

## Atualização 2026-11-24 (noite) — desempenho do 3D, medido com amostragem de PC
- **Ferramenta:** um sampler de PC no zeebx (`ZEEBX_PCSAMPLE=N`, `ZEEBX_PCSAMPLE_OUT=arquivo`; alteração local no zeebx, fora do repo do port) mostrou, em 75 s de Windy Valley, 52-59% das instruções do guest em soft-float (`__aeabi_fadd`, `__mulsf3`, `__nesf2`, `__aeabi_fcmpgt`...). Chamadores: `project_model` 14-18%, a matemática exata do SH-4 (`math.c` de referência) ~17%, `zstrip_flush` ~8%, colisão (`polygon_*`) ~12%, `pvr_untwiddle_index` 4%.
- **Feito:** radix estável em `zstrip_flush` (mesma ordem); `pvr_untwiddle_index` com caminho rápido para texturas quadradas; luz com `1/|l|` em cache; `fast.c` (mesmos bits, em inteiros, testado contra a referência no host) no lugar de `math.c`; meshsets `USE_ALPHA` com alfa final 255 e textura sem alfa saem como opacos, os de recorte (alfa 0/255) com teste de alfa, e os translúcidos de verdade passam pelo caminho GL e são adiados e ordenados por 1/z do centro do modelo (`zstrip_defer`, `ZLIVE_TDEFER=0` volta ao caminho de CPU).
- **Medido (Windy Valley, início da fase, zeebx):** quadro médio 183 ms para 116 ms; 624 para 811 quadros em 75 s. Muda a imagem: a luz desses meshsets passou da fórmula por face (0,45+0,55) para a por vértice dos opacos (0,70+0,30), por isso o `hash_ref` foi regravado.
- **Pendências:** os 116 ms não são só instruções do guest (o guest executou menos instruções que o orçamento de 75 s): falta separar custo da API GL, espera de vsync e rasterização do zeebx; colisão e `sqrtf`/`seno_base` seguem em soft-float; medir Station Square e a briga com o Chaos; conferir visualmente grama, água e efeitos contra o console.
- **Ritmo do timer (achado depois da amostragem):** `sa1z_Timer` armava o próximo tique com 28 ms depois do trabalho do quadro, então cada tique pesado levava 28 ms de espera por cima (o guest ficava em ~38% de uso do console: 14,9 de 39,6 bilhões de instruções em 75 s). Agora o período conta do início de um tique ao início do seguinte (`sa1z_next_delay`). Windy Valley: quadro médio 116 para 64 ms, 811 para 1228 quadros em 75 s; nos menus, 2726 para 3475 quadros em 100 s (as transições ficam mais rápidas). O `hash_ref` foi regravado de novo (o alinhamento dos quadros mudou, não o desenho).
- **Tique a 60 Hz (`SA1Z_FPS` 35 para 60):** o jogo espera 2 vsyncs por quadro (30 fps) e cada espera é um tique do timer; a 35 Hz o teto era 17,5 fps e o relógio do jogo andava a ~58% do real. Com o período relativo e 60 Hz, Station Square (zeebx, 75 s): quadro médio 63 para 49 ms, 1231 para 1694 quadros, console a 69% de uso. `hash_ref` regravado (59 linhas). Perfil de Station Square: `shz_xmtrx_ftrv_` (16 mul + 12 add em soft-float, cópia estática em cada objeto, vem do SHZAM) ~13%, `dot` do `fast.c` 7%, `sqrtf` 7%, `draw_meshset_gl` 9%, `polygon_collect_near` e `world_list_near` ~9%. Próximos alvos: FTRV (pular o termo w nos pontos, ou uma versão em ponto fixo), `sqrtf` e colisão.
- **Rotacoes e raiz (Station Square, zeebx, 75 s):** `njRotateX/Y/Z/XYZ` gastavam ~12% das instrucoes porque `shz_xmtrx_rotate_*_u16` montava a rotacao e multiplicava a 4x4 inteira (4 FTRV de 16 multiplicacoes e 12 somas em soft-float). `zam_turn` (`ports/zeebo/ninja/ninja_zam.h`) agora mexe so nas duas colunas afetadas (p' = c p + s q, q' = c q - s p; eixos X, Y, Z = colunas 1-2, 2-0, 0-1); conferido no host contra o SHZAM, bit a bit, em 2000 matrizes por eixo. `sqrtf` (`platform/softfloat.c`) virou raiz por digitos em inteiros, arredondada: bit a bit igual ao libm em 2 milhoes de floats. `mat_color` devolve o difuso antes de converter as quatro cores para float. Resultado: 27,2 para 24,9 bilhoes de instrucoes em 75 s (-8%); o quadro continua em 44-49 ms porque agora o jogo esta perto do teto do ritmo (2 vsyncs a 60 Hz = 33 ms). Sem regravar o `hash_ref`. Nota de build: o Makefile nao rastreia `ninja_zam.h`; apos mexer nele, dar `touch engine/dc/ninja/*.c`.
- **Proximos alvos (perfil):** `draw_meshset_gl` ~10% (um `zstrip_model_vertex` por vertice), `dot`/`sa_ftrv`/`sa_fipr` (FTRV/FIPR exatos do `fast.c`) ~13%, colisao (`polygon_collect_near`, `world_list_near`, `polygon_find_heights`, `polygon_sweep`) ~16%, `surface_upload` 2,7%.

## Atualização 2026-11-24 (noite, 2) — o código do jogo estava em -O0
- **Achado:** `game.mk` definia `CFLAGS :=` (expansão imediata) antes do `OPT ?= -O3`, então `$(OPT)` era vazio e todo o código do jogo (decomp, asm traduzido, collision, ninja) saía **sem nenhum `-O`**, ou seja, -O0. Agora `OPT ?= -O2 -fno-tree-vectorize` vem antes do `CFLAGS` (e o carimbo do `build.sh` mostra o OPT). O `asm.c` (546 funções `f__nj*`, o SH-4 traduzido para C com banco de registradores) caiu de 521 KB para 217 KB e o `enter`/`leave`/`narrow` de cada chamada passou a ser inline. Station Square, 75 s de zeebx: 24,9 para 19,1 bilhões de instruções (-23%), `.mod` 37,40 para 37,38 MB. O registro de 24/11 dizia "`OPT=-O2` não adotado porque o hash diverge": a divergência é só de alinhamento de quadros (o tempo de jogo muda com a velocidade); a imagem dos quadros conferidos está igual. **Trocar o OPT exige apagar `build/zeebo/game/obj`** (o make não rastreia flags).
- **`zfpu.c`:** `sa_ftrv`, `sa_fipr`, `sa_fmac`, `sa_fsqrt`, `sa_fsrra` e as comparações viram contas diretas em float (o `fast.c` os fazia em inteiros bit a bit com o console; o `game.mk` renomeia os do `math.o` para `exact_*` com `objcopy`). `sa_fsca` (tabela do console) passa a ser o seno/cosseno de `zam_fsca`/`zam_turn`: o `shz_sincosu16` do SHZAM chamava `sinf`/`cosf` (Taylor com redução de quadrante em soft-float, ~5%); erro da tabela contra libm 1,1e-7. O forte de `sa_fsca` está em `ninja/zxmtrx.c` (o `math.o` do jogo é fraco no .mod); o applet sozinho usa o fraco de `platform/softfloat.c`. `host-selftest` agora linka o `fast.c`.
- **Ritmo do tique (30 fps de verdade):** com o log por tique (`inicio`/`passo`), um quadro eram dois tiques: o que não apresenta (7 ms de passo) esperava 16 ms **antes** da conta pesada (19 ms) e o swap ainda esperava o vsync (34 ms no total): 50 ms com 26 ms de CPU. O tique que não apresenta agora espera só o que falta para a conta pesada (média móvel dos passos que apresentam) terminar 3 ms antes do fim do período do quadro, contado do último swap (`sa1z_idle_delay`, `frame_interval` vsyncs de 16,67 ms). Station Square: quadro médio 49 para 34-36 ms, 1708 para 2282 quadros em 75 s (teto de 30 fps = 2250 + a abertura), console a 65% de uso.
- **Perfil depois disso (-O2):** `sa_ftrv` naive ~10%, colisão (`polygon_collect_near`, `world_list_near`, `polygon_sweep`, `polygon_find_heights`) ~18%, `seno_base` (agora fora das rotações), `draw_meshset_gl` ~4%, `surface_upload` 3,5%, `memcpy` 2,5% (`f__njCalcPoint`).
- **Chamadores do FTRV e colisao (instrumentando `sa_ftrv` e `njCalcPoint` por endereço de retorno):** 67% das chamadas de `sa_ftrv` vinham de `njCalcPoint`, e dessas, `polygon_sweep` (~41%), `draw_in_view` (~50%) e `player_update_welds`. `zfpu.c` agora define `njCalcPoint`/`njCalcVector` para a matriz corrente (9 produtos; com matriz explicita chama o original, renomeado `*_asm` no `asm.o`), `draw_in_view_any_step` (`src/draw/model.c`, só em NONMATCHING) testa o z da esfera antes de transformar x e y, e `model_in_view` (`zlive.c`) calcula só o z. `memcpy`/`memset` (`platform/string.c`) copiam por palavras quando alinhados (eram byte a byte, ~2,5%). Station Square: 25,6 para 24,7 bilhões de instruções em 75 s; `sa_ftrv` de 12,4% para 3,6% do perfil.
- **Sem mudança:** `surface_upload`/`pvr_expand`/`prs_run` (só carregamento, ~5%; `-O2` já medido sem ganho em 24/11), `draw_meshset_gl` (~5%, inteiro) e as buscas espaciais da colisão (`polygon_collect_near` ~10%, `world_list_near` ~6%, `polygon_sweep` ~5%): são um laço de soft-float por entrada, sem laço redundante óbvio; o ganho passa por reduzir as chamadas por quadro (7 `world_list_near` por jogador) ou por uma grade espacial.

## Atualização 2026-11-24 (noite, 3) — Chaos 0 (relato: quadros caindo)
- **Log do usuário** (`~/Downloads/Sonic Adventure 1 (Zeebo Port).log`, run com Chaos 0): quadro medio 33-39 ms com maximo de 50 ms em quase toda janela de 2 s: o jogo perdia o vsync de 33 ms e alternava 2 e 3 vsyncs (30/20 fps), o que se ve como queda de quadros e esconde movimentos rapidos do chefe. Reproduzido no zeebx (`DEBUG_TITLE=35` = Chaos 0): janelas de 2 s com ate 19-28 quadros de 3 vsyncs de ~52.
- **Novo contador** `tperf quadros de 2 / 3 / 4+ vsyncs na janela` (`zinst.c`): mostra no log quantos quadros cumpriram os 33 ms.
- **Causas e correcoes:** (1) `zlog` era 6,5% das instrucoes no chefe: os stubs de desenho (`kmProcessVertexRenderState`, `kmSetVertexRenderState`) chamam `zlog` por chamada e ele varria 256 entradas de contagem de "HOT"; agora a busca e por hash. (2) `sprite_view` (`engine/dc/sa1/sprite.c`, so com `-DZEEBO_PORT`, definido em `DCFLAGS` e no `game.mk`) usa 3 linhas da matriz em vez do FTRV de 4 colunas: sprites/efeitos eram ~9%. Nota: o `njSprite.o` nao depende do `sprite.c` no Makefile; apague `build/zeebo/njSprite.o` ao mexer nele. (3) A estimativa do ritmo (`p_step_ms`) ignorava o flush dos translucidos antes do swap; agora soma passo + flush, com +17% de folga.
- **Medido (Chaos 0, 80 s de zeebx):** quadros de 3 vsyncs 162 para 20 de ~2380 (0,8%), instrucoes 23,1 bi; o `hash_ref` foi regravado de novo. O que continua pesado ali: `shz_xmtrx_ftrv_` nas multiplicacoes de matriz, `sa_ftrv`, `f__njInvertMatrix` (welds), `polygon_find_heights`.
- **Cache da modelview (24/11, noite):** `LoadMatrixx` 968.657 -> 635.484 por 80 s (-34%), instrucoes 23.100 -> 23.139 M. A uniao de strips translucidos economiza 8,1% dos draws mas desenha o triangulo de ligacao: medida e desligada (`Z_TJOIN`), documentada em `docs/otimizacoes-propostas.md`.
- **Relogio e tique (24/11, noite):** o `ISHELL_SetTimer` so aceita ms inteiros e `1000/60` truncado dava 16: a fracao agora vai num acumulador (16,17,17,16...) e `p_frame_ms` sai com media de 16.667. Medido no zeebx headless (deterministico, 60 s): **3589 tiques em 60.001 ms = 16,718 ms/tique** (+0.3% do teorico) e 1888 quadros = 31.78 ms/quadro (31.5 fps, ~4.7% acima dos 33.33 ms de 2 vsyncs, que e a folga do `sa1z_idle_delay`). Com freio (`run --window`, Xvfb) o tempo do guest fecha **100.0% do tempo real**; sem janela o zeebx roda ~5x mais rapido de proposito (o relogio virtual avanca com as instrucoes). O `tfun` loga `uptime ms`, `janela ms`, `periodo medio pedido us` e `tiques na janela`. Para comparar, o `run` do zeebx agora imprime `X virtual ms in Y real s (Z% of real time)`.
- **Proximas otimizacoes propostas:** lista priorizada (evidencia, risco, como medir) em `docs/otimizacoes-propostas.md`. O `sprite_view` tirou o FTRV dos sprites: `shz_xmtrx_ftrv_` caiu de 12,1% para 3,5% das amostras; o que sobrou de maior sao `sa_ftrv` 7,7%, o proprio `sprite_view` 7,3% (agora multiplicacao direta), `f__njInvertMatrix` 5,4% e a colisao somada ~10%.
- **Limite da medicao:** os runs nao sao deterministas (o ritmo depende de `GETUPTIMEMS`), e antes da correcao do ritmo o mesmo trecho deu 94, 154 e 162 quadros de 3 vsyncs em tres rodadas. O 20 vem de uma rodada; repetir 3x antes de tratar como numero final. A equivalencia do `sprite_view` foi provada por teste no host (2 M matrizes, 0 diferencas de bit contra o corpo do `sa_ftrv` com w=1), nao por olho.

## Atualização 2026-11-24 15:18 — build íntegro (744/744) e a árvore ocupada: A1/A2/A3 de `otimizacoes-propostas.md` sem commit

Rodada do cron de 2 h, 15:18–15:30. `HEAD` = `1201937` (24/11 14:42), branch
`zeebo-port`. **Nenhum commit novo desde a atualização de 24/11 (noite, 3)**: os
três últimos (`74d283a`, `648cfbd`, `1201937`) já estão registrados lá. O que
mudou foi na árvore, não no histórico.

| Item | Medido agora | Observação |
|---|---|---|
| `make` do applet | nada a fazer (objetos de 15:18:27) | `app.c`/`zstrip.c` do applet já recompilados pela outra sessão |
| `make -f game.mk mod` | **744 de 744 objetos**, `duplicatas: 0`, **indefinidos: 560** | `link` + `integrate`; nenhum `FALHOU`, `falhas.txt` vazio |
| `sa1g.mod` | **37.379.612 B** (35,65 MiB) | 242 `.ovl`, 32.114.321 B no disco; acima do orçamento de 30–32 MB |
| `host-selftest` | **PASSED (0 failures)** | inclui `fast.c`/`zfpu.c` |
| `PROGRAMS` | **244** = 40 módulos + 204 eventos | `modules/` tem 41 diretórios; o de fora é `movie` (o filme é lido por `zgdfs` sobre `movie_entry`, `zgdfs.c:331-334`); `config/b_mot` também não entra |
| Indefinidos | 560 contra 559 em `2364e53` | uma referência a mais; **não** é de `zstrip`/`zs_*` (conferido em `build/zeebo/game/integrate.err`); símbolo não identificado |
| Disco | `/` **100%**, 4,0 GB livres | `build/zeebo` = 13 GB: `keep/` 5,5 GB (72 builds), `game/` 5,3 GB, `game_o2/` 1,2 GB |
| Árvore | `app.c` (15:03) e `zstrip.c` (15:18) modificados, sem commit | não commitei (fora de docs) e não validei |

- **Outra sessão trabalha na mesma árvore nesta janela:** o diff é a A1/A2/A3 de
  `docs/otimizacoes-propostas.md` — contadores de bind por janela
  (`zs_texbinds`/`zs_texdistinct`, `zstrip.c:134`, lidos por
  `zstrip_text_janela`, `zstrip.c:1116`, e logados em `app.c`), cache da
  modelview contra o que está no GL (`m_model_changed`, `zstrip.c:128,536-545,642`)
  e junção de strips 2D translúcidos vizinhos (`Z_TJOIN`, `zstrip.c:26-31`,983).
  A junção está **desligada**: sem vértices degenerados ela desenha o triângulo de
  ligação (10.659 pixels diferentes no mesmo quadro, medido pela própria sessão).
  Um `make -f game.mk mod DEBUG_TITLE=35` dela rodou às 15:18 e terminou antes do
  desta rodada; os logs de medição (`~/.hermes/cache/scratch/d/`, 15:05–15:23)
  ainda não têm conclusão. Pela armadilha de 23:07 (dois `make` sem `flock`), não
  há número novo de quadro/imagem aqui: **a última medida válida segue a de
  24/11 (noite, 3)**, Chaos 0 com 3 vsyncs em 162 → 20 de ~2380.

#### O que foi feito desde a atualização anterior

Nada de código entrou no git. O que esta rodada fechou é o **estado**, que estava
espalhado: os 244 programas continuam todos no `.mod`, o gate `744/744` +
`duplicatas: 0` passou, o `host-selftest` passou, e o `.mod` de 35,65 MiB segue
6 MB acima do orçamento. Achados novos: o `/` está **100%** (4,0 GB), com 13 GB
em `build/zeebo` e 72 builds em `keep/`; e o contador de indefinidos subiu de 559
para 560 (um símbolo, não identificado).

#### O que falta para o OBJETIVO FINAL (port completo, todo o conteúdo)

| Conteúdo | Estado em 15:18 | Falta |
|---|---|---|
| Fases de ação (stg00–stg10, stg12) | 12 programas linkados; **1 ato medido** em 9 deles (rows 3, 6, 9, 12, 18, 21, 24, 28, 32, em builds anteriores; com o `.mod` atual, 6 linhas no total); Red Mountain (rows 15–17) não medido | atos 2–4 de todas; Hot Shelter (stg12) **sem linha no menu** (`title.c:202`, tabela em `:384-461`) |
| Station Square | `adv00` linkado; row 40 rodou (câmera num pilar, sem HUD) | rows 41–45 nunca medidas |
| Mystic Ruins | `adv02` + `tikal_prog`; **nenhuma** das rows 58–61 medida | as 4 áreas |
| Egg Carrier / por dentro | `adv01obj`+`adv0100`/`adv0130`; row 46 sai escura e sem HUD claro | rows 47–57 nunca medidas |
| Past | `adv03` + `tikal_prog`; rows 62–64 nunca medidas | as 3 áreas |
| Chefes | Chaos 0 (row 35), Chaos 2/4 (row 37, escura/esverdeada), Zero (row 39, geometria errada) | **Chaos 6, Perfect Chaos, Egg Viper 2/3, E101, E101-R: sem linha no menu**; 7 programas linkados inalcançáveis |
| Chao Garden / corrida | `al_main`+`al_garden00-02`+`al_race`; rows 69–71 e 72 nunca medidas com este `.mod` | as 3 áreas e a corrida |
| Minijogos | Twinkle Circuit (row 65, pista 95% preta), Sky Chase 1/2 (rodam), Sand Hill (roda) | Circuit visual e a sequência de teclas da corrida |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1` | 448 linhas inteiras (hoje pula metade, `zplm.c:147-151`) |
| Som | música e áudio de filme tocam; efeitos por clipe; vozes | clipes sustentados (17/179), pitch bend (`sound.c:757`), vozes de evento longo, efeitos clipe a clipe |
| Save | cartão emulado, data do RTC | save no campo não exercitado |
| Desempenho | Chaos 0 quase sem quadro de 3 vsyncs (20 de ~2380); Station Square ~34-36 ms | ms no Adreno 130 não medidos (sem aparelho) |
| Pacote | `.mod` 35,65 MiB + 242 `.ovl` | cortar ~6 MB; instalador |

#### 20 quick wins (K1–K20, abertos em 24/11 15:18; P ≤ 1 h, M ≤ meio dia)

Substituem as listas V1–V15 e U1–U20. Fechados de antes, para referência: V1
(cache do passe translúcido), V3 (`HOTOPT=-O2`), V7 (é o `SL_*B.bin` opcional),
V8 (`vprof` com desenhos/estado repetido), V13 (`estado hash` + `hash_gate.sh`),
V2 (medido, sem ganho), V11 (causa do upstream achada), V4 (`TEX16=1` opt-in,
falta decidir o padrão).

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| K1 | Rodar a matriz inteira (76 linhas do menu) e guardar quadro + log por linha com o `.mod` atual | M | só 6 de 76 medidas; `tools/stage_matrix.sh` faz 29 linhas por vez |
| K2 | 7 linhas no menu de debug (Hot Shelter, CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R) | P | `src/progress/title.c:202` (`DEBUG_STAGES 76`) e a tabela em `:384-461`; os 7 programas estão linkados e sem linha |
| K3 | Inventariar e liberar espaço: `keep/` com 72 builds (5,5 GB) e `game_o2/` (1,2 GB, build do experimento OPT) | P | `/` 100% com 4,0 GB livres; `du -sh build/zeebo/*`; nada apagado sem pedido |
| K4 | Identificar o indefinido 560 (era 559) | P | `grep -c 'undefined reference' build/zeebo/game/integrate.err`; nenhum é de `zstrip`/`zs_*` |
| K5 | A1: agrupar strips opacos por textura (radix por `texId`) | M | 337 `BindTexture`/quadro com 520 texturas vivas; cache de 8 KB do Adreno; `docs/otimizacoes-propostas.md` A1 |
| K6 | A2: juntar strips contíguos num `DrawArrays` — inserir degenerados no pool antes de ligar `Z_TJOIN` | M | 486 draws/quadro (221 do `zstrip`); `zstrip.c:26-31` (10.659 px espúrios sem degenerados), `:983` |
| K7 | A3: medir o cache da modelview (já no diff: `m_model_changed`) e passar o `hash_gate` | P | 397 `LoadMatrixx`/quadro; `zstrip.c:128,536-545`; `tools/hash_gate.sh A B` |
| K8 | B1: cachear a inversa por matriz de origem nos welds | M | `f__njInvertMatrix` 5,4% das amostras; 282 chamadas (`path_editor_reorder` 126, `event_motion_weld` 62, `event_weld` 32); `otimizacoes-propostas.md` B1 |
| K9 | B3: colisão com coerência (cache do último polígono por consulta) | M | `polygon_collect_near` ~10% + `world_list_near` ~6% do perfil; 7 chamadas de `world_list_near` por jogador por quadro |
| K10 | Filmes em 448 linhas inteiras (`step=1`) | M | `ports/zeebo/zplm.c:147-151` pula uma linha quando `height/step > maxh` (448 → 224 linhas) |
| K11 | Zero (row 39): geometria errada (campo teal listrado, sem o robô) | M | row 39 após `2364e53`; iluminação `simple` caiu na do `draw_basic` (`zlive.c`) |
| K12 | Twinkle Circuit (row 65): pista 95% preta e sequência de teclas da corrida | M | `52455310e0db`; `DEBUG_TITLE=65`; a sequência de 23:2x parou no menu |
| K13 | Egg Carrier (row 46): HUD sem cor e canto superior preto | M | `71b404f1d722` (466 cores, sem HUD claro); rows 47–57 não medidas |
| K14 | Cortar ~6 MB do `.mod` (35,65 MiB contra 30–32 MB) | M | `sa1g.mod` 37.379.612 B; 242 `.ovl` = 32,1 MB no disco; `.mod` já carrega a região dos overlays |
| K15 | Vozes de evento longo: `ADXT_StartAfs` além do menu | P | só o menu (flid 0) foi correlacionado (0,88); `zaudio.c` (zADXT_StartAfs via objcopy) |
| K16 | Clipes sustentados: 17/179 repetem o clipe inteiro; pitch bend ignorado | M | `zaudio.c` (intro+laço, `MM_STATUS_DONE`); `src/sound/sound.c:757` (`sound_slot_set_pitch`) |
| K17 | Exercitar o save no campo (cartão emulado) e conferir a data do RTC | P | `sa1vmu.bin`; `zsys.c` (`GETJULIANDATE`); nunca exercitado em campo |
| K18 | Névoa nas outras áreas e nos modos PVR que faltam | M | só opacos da via GL; Sky Deck/Ruin/Casino não olhados; PVR modo 1 (`offset.a`) e modo 2 (alfa da tabela) |
| K19 | `session --gpu` determinístico (`--frames=N`) para medir `DYNRES` na placa | M | D5: mesmo trecho de 60 s terminou em ~3 s e ~21 s; ~31 quadros |
| K20 | Boomerang no zeebx (teste de PC já passa) | P | `zboom.c`, `zhid.c`, `tests/boom_test.c`; medida no emulador pendente desde `fe22ce0` |

## Atualização 2026-11-24 17:25 — build íntegro, cache da modelview corrigido no flush e tique de 16,667 ms

Rodada do cron de 2 h, 17:25–17:40. `HEAD` = `04cc830` (17:09), branch
`zeebo-port`; árvore limpa, só `todo.md` não rastreado (arquivo da outra sessão,
fora de `docs/` — não entrou no commit). **Nenhum `make` concorrente nesta
janela**: o último commit é de 8 min antes do build.

| Item | Medido agora | Observação |
|---|---|---|
| `make` do applet | nada a fazer | objetos de `app.c`/`zstrip.c` já recompilados |
| `make -f game.mk mod` | **744 de 744 objetos**, `duplicatas: 0`, **indefinidos: 560**, 45,7 s | nenhum `FALHOU`; mesmo 560 de 15:18 |
| `sa1g.mod` | **37.379.932 B** (35,65 MiB), 17:27 | 242 `.ovl` = **32.114.398 B**; região 2.242.340 B |
| `host-selftest` | **PASSED (0 failures)** | linka `fast.c`/`zfpu.c` |
| `PROGRAMS` | **244** = 40 módulos + 204 eventos | `modules/` tem 41 diretórios (fora: `movie`); `MISSING_PROGRAMS` **vazio** |
| Palco de debug | **76** linhas (`DEBUG_STAGES`), 73 `STAGE_*` + 3 zeros | `src/progress/title.c:202`, tabela `:384-461` |
| Matriz medida | **8 de 76** com build recente | 6 em `build/zeebo/matrix_r/summary.txt` (23/11 20:18: rows 3, 37, 39, 46, 65, 71), row 40 (24/11 13:2x) e row 35/Chaos 0 (`74d283a`) |
| `.err` do build | 4.073 arquivos, **todos** com o mesmo aviso | apóstrofo no `#error` de `engine/native/prelude.h:18` (`play's`), cosmético; nenhum erro |
| Disco | `/` a **98%**, 13 GB livres | melhorou de 100%/4,0 GB (15:18); `keep/` 6,7 GB em 80 builds, `game/` 5,6 GB, `game_o2/` 1,2 GB |

#### O que foi feito desde a atualização anterior (6 commits, de `3080abb` a `04cc830`)

- **`5bcf387` 15:43 — A3, cache da modelview (K7 fechado).** `model_flush`
  guarda os 16 valores e `zstrip_model_begin` compara na cópia que já fazia:
  **`LoadMatrixx` 968.657 → 635.484 em 80 s (-34%, -136/quadro)**, instruções
  23.100 → 23.139 M (+0,17%). Junto: `vprof` de binds/texturas distintas/runs
  translúcidos/strips unidos (hash aberto de 512; a varredura linear custava
  1,7% do guest).
- **`5bcf387` — A2 (K6) medido e desligado.** Juntar strips translúcidos
  vizinhos poupa 8,1% dos draws (824.465 → 757.656 em 60 s) com custo zero de
  instruções, **mas** sem vértices degenerados desenha o triângulo de ligação
  (10.659 px espúrios no mesmo quadro). Fica atrás de `Z_TJOIN=0`.
- **Descoberta que muda a prioridade de A1:** o batching opaco **já funciona**
  (6 draws/quadro); 155 dos 161 draws do `zstrip` são translúcidos, um por
  strip, e a ordem por z impede agrupar. O alvo não é o opaco.
- **`b08f0d1` 16:30 — correção da matriz do modelo.** A flag `m_model_changed`
  era decidida no `zstrip_model_begin`; entre o begin e o flush outro modelo (ou
  o passe 2D) trocava a MODELVIEW e o modelo deferido saía com a matriz alheia
  (**halo do sol esticado, botões do menu com a textura do cenário**). Agora o
  `model_flush` compara `m_model` com `l_model` na hora (16 comparações em RAM):
  635.484 → 635.123 `LoadMatrixx`, 23.135 → 23.209 M instruções, 2.440 quadros,
  `hash do estado` **IGUAL**.
- **`33e3c7c` 16:15 — tique de 16,667 ms.** `ISHELL_SetTimer` só aceita ms
  inteiros e `1000/60` truncado em 16 somava 960 ms em 60 tiques; a fração vai
  num acumulador (16,17,17,16…). Medido headless (60 s): **3.589 tiques em
  60.001 ms = 16,718 ms/tique (+0,3%)**, 1.888 quadros = 31,78 ms/quadro; com
  `--window` o tempo do guest fecha **100,0%** do real. Novo log `tfun`
  (`uptime ms`, `janela ms`, `periodo medio pedido us`, `tiques na janela`).
- **`04cc830` 17:09 — `estado_hash.ref` refeita.** O tique de 16,667 mudou
  quantos quadros cabem em 100 s (3.089 → 3.088) e o gate passou a divergir
  **sem** regressão de desenho: os 10 primeiros hashes (640 quadros) são
  idênticos e a divergência é de fase. Regra nova registrada: quando o pacing
  muda, `hash_ref.sh update` no mesmo commit.
- **`2d2a71f`/`b2f42e7`** — o contador "strips unidos" só aparece com
  `Z_TJOIN=1` (com 0, fazia ruído no relatório).

Nada disso foi medido nesta rodada (não rodei o zeebx): os números acima são os
dos próprios commits. **O que falta de medição:** os 8 de 76 da matriz, e repetir
o Chaos 0 3× — o "20 quadros de 3 vsyncs" é de uma rodada só.

#### O que falta para o OBJETIVO FINAL (port completo, todo o conteúdo)

| Conteúdo | Estado em 17:25 | Falta |
|---|---|---|
| Fases de ação (stg00–stg10, stg12) | 12 programas linkados; 1 ato medido em 9 deles em builds anteriores; com o `.mod` atual só a row 3 (Emerald Coast) | atos 2–4 de todas; Hot Shelter (stg12) **sem linha no menu** (`title.c:384-461`) |
| Station Square | `adv00` linkado; row 40 rodou (câmera num pilar, sem HUD) | rows 41–45 nunca medidas |
| Mystic Ruins | `adv02` + `tikal_prog`; **nenhuma** das rows 58–61 medida | as 4 áreas |
| Egg Carrier / por dentro | `adv01obj`+`adv0100`/`adv0130`; row 46 escura, HUD sem cor (`71b404f1d722`, 466 cores) | rows 47–57 nunca medidas |
| Past | `adv03` + `tikal_prog`; rows 62–64 nunca medidas | as 3 áreas |
| Chefes | Chaos 0 (row 35) medido e otimizado; Chaos 2/4 (row 37, escura/esverdeada); Zero (row 39, geometria errada, `c2eb03f3a477`) | **Chaos 6, Perfect Chaos, Egg Viper 2/3, E101, E101-R: sem linha no menu**; crash ao vencer o Chaos 0 sem log (`todo.md:20`) |
| Chao Garden / corrida | `al_main`+`al_garden00-02`+`al_race` linkados; rows 69–72 não medidas com este `.mod` | as 3 áreas e a corrida |
| Minijogos | Sky Chase 1/2 rodam; Sand Hill roda (row 68); Twinkle Circuit (row 65, `52455310e0db`) 95% preto | pista do Circuit e a sequência de teclas da corrida |
| 2D / sprites | matriz do modelo corrigida (`b08f0d1`) | textura errada com texlist curto (painel de pause, `todo.md:12`); `zstrip_tex_release_all` não limpa `tex_names[]` (`zstrip.c:1357-1366`) |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1` | 448 linhas inteiras (hoje pula metade, `zplm.c:147-151`) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes sustentados, pitch bend (`sound.c:757`), vozes de evento longo, SFX 259 ausente (`todo.md:38`) |
| Save | cartão emulado, data do RTC | save no campo não exercitado |
| Desempenho | Station Square ~34-36 ms; Chaos 0 quase sem quadro de 3 vsyncs | ms no Adreno 130 não medidos (sem aparelho) |
| Pacote | `.mod` 35,65 MiB + 242 `.ovl` | cortar ~6 MB; instalador |

#### 20 quick wins (W1–W20, abertos em 24/11 17:25; P ≤ 1 h, M ≤ meio dia)

Substituem K1–K20. **Fechados desde a lista anterior:** K7/A3 (cache da
modelview, `5bcf387`), K6/A2 (medido, desligado atrás de `Z_TJOIN`), K3 parcial
(o `/` saiu de 100% para 98%, 13 GB livres; nada apagado por mim). Fora da
lista, ainda abertos: névoa nas outras áreas e modos PVR 1/2 (K18), o 560º
indefinido (`integrate.err`), o aviso de apóstrofo em `engine/native/prelude.h:18`
(cosmético, suja 4.073 `.err`).

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| W1 | Rodar a matriz inteira (76 rows) com o `.mod` de hoje, quadro + log por linha | M | só **8 de 76** medidas; `tools/stage_matrix.sh` faz 29 por vez; `build/zeebo/matrix_r/summary.txt` (6 linhas de 23/11) |
| W2 | 7 linhas no menu de debug (Hot Shelter, CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R) | P | `src/progress/title.c:202` (76 linhas) e tabela `:384-461` (73 `STAGE_*` + 3 zeros); os 7 programas já estão em `PROGRAMS` (`ports/zeebo/game.mk:152-153`) |
| W3 | Repetir o Chaos 0 3× e fixar o número de quadros de 3 vsyncs | P | `zinst.c` (`tperf`); a mesma medida deu 94, 154 e 162 antes da correção do ritmo — o "20 de ~2380" é de 1 rodada |
| W4 | `zstrip_tex_release_all` limpar `tex_names[]` (hoje `tex_count=0` e o array fica com nomes GL mortos) | P | `zstrip.c:1357-1366`; superfície que ainda aponte para o índice passa a usar outra textura |
| W5 | Bug de textura em sprite 2D com texlist curto (título do pause vira faixa preta) | M | `todo.md:12`; reproduzido com `session --state` (slot-3, 16:44); precisa de state novo com build instrumentado |
| W6 | Crash ao vencer o Chaos 0 sem log (pico de 760 MB para 64 MB emulados) | M | `todo.md:20`; `.log`/`.serial.log` mais recentes são de 15:58; pedir "Exportar .log" ou state com o chefe enfraquecido |
| W7 | A2 com vértices degenerados para ligar `Z_TJOIN` | M | 8,1% dos draws (824.465 → 757.656 em 60 s), 10.659 px espúrios; `zstrip.c:26-31,983`; exige cópia no pool |
| W8 | B1: cachear a inversa por matriz de origem nos welds | M | `f__njInvertMatrix` 5,4% das amostras; 282 chamadas (`path_editor_reorder` 126, `event_motion_weld` 62, `event_weld` 32) |
| W9 | B3: colisão com coerência (cache do último polígono por consulta) | M | `polygon_collect_near` ~10% + `world_list_near` ~6%; 7 chamadas de `world_list_near` por jogador por quadro |
| W10 | `draw_meshset_gl`: tirar a chamada `zstrip_model_vertex` por vértice do caminho quente | M | ~4-5% do perfil depois do `-O2` (24/11 noite, 2) |
| W11 | Zero (row 39): geometria errada (campo teal listrado, sem o robô) | M | `matrix_r/summary.txt`: hash `c2eb03f3a477`, `traps=0 ausente=0`; iluminação `simple` caiu no `draw_basic` |
| W12 | Twinkle Circuit (row 65): pista 95% preta e sequência de teclas da corrida | M | hash `52455310e0db`; `DEBUG_TITLE=65` |
| W13 | Egg Carrier (row 46): HUD sem cor e canto superior preto | M | hash `71b404f1d722` (466 cores); rows 47–57 não medidas |
| W14 | Cortar ~6 MB do `.mod` (35,65 MiB contra 30–32 MB) | M | `sa1g.mod` 37.379.932 B; 242 `.ovl` = 32.114.398 B; o `.mod` já reserva a região dos overlays |
| W15 | Filmes em 448 linhas inteiras (`step=1`) | M | `zplm.c:147-151` (`while height/step > maxh: step++` → 224 de 448) |
| W16 | Vozes de evento longo (`ADXT_StartAfs` além do menu) | P | só o flid 0 do menu foi correlacionado (0,88 / ganho 0,98); `zaudio.c` |
| W17 | Clipes sustentados (17/179 repetem o clipe inteiro) e pitch bend ignorado | M | `zaudio.c` (intro+laço por `MM_STATUS_DONE`); `src/sound/sound.c:757` |
| W18 | Exercitar o save no campo e conferir a data do RTC | P | `sa1vmu.bin`; `zsys.c` (`GETJULIANDATE`); nunca exercitado em campo |
| W19 | Espaço: `keep/` com 80 builds (6,7 GB) e `game_o2/` (1,2 GB) | P | `/` a 98% com 13 GB livres; `/` esteve a 100% às 15:18; só apago com autorização |
| W20 | Boomerang medido no zeebx (teste de PC já passa) | P | `zboom.c`, `zhid.c`, `tests/boom_test.c`; pendente desde `fe22ce0` |

## Atualização 2026-11-24 19:30 — passe 2D novo na árvore (depth desligado, `EMIT_RUN` reordenado), sem commit e sem medida

Rodada do cron de 2 h, 19:30–19:45. `HEAD` = `05b53ea` (17:30, o commit de
docs da rodada anterior), branch `zeebo-port`. **Nenhum commit novo desde a
atualização das 17:25**: o que apareceu foi trabalho de outra sessão **só na
árvore de trabalho** — `ports/zeebo/zstrip.c` (18:03, +29/−10),
`src/hud/overlay.c` (18:01) e `sa1n.bmp` (18:18, 921.654 B, não rastreado,
captura do zeebx). Nada disso foi commitado nem medido; o `mod` desta rodada
compilou **com** essas mudanças (é o que o estado abaixo mede).

| Item | Medido agora (19:30) | Observação |
|---|---|---|
| `make` do applet | rc=0, nada a recompilar | `zstrip.c`/`overlay.c` já compilados às 18:0x |
| `make -f game.mk mod` | **744 de 744 objetos**, `duplicatas: 0`, **indefinidos: 560** | nenhum `FALHOU`; mesmo 560 de 15:18/17:25 |
| `sa1g.mod` | **37.375.564 B** (35,64 MiB), 19:32 | 4.368 B menor que 17:27 — efeito do reordenamento do `zstrip_flush` |
| `.ovl` | **242** = **32.114.443 B**; região 2.242.340 B | inalterado |
| `host-selftest` | **PASSED (0 failures)** | linka `fast.c`/`zfpu.c` |
| `PROGRAMS` | **244** = 40 módulos + 204 eventos | `modules/` tem 41 diretórios (fora: `movie`, só dados); `MISSING_PROGRAMS` **vazio** |
| Menu de debug | **76** linhas = 73 `STAGE_*` + 3 zeros | `src/progress/title.c:202`, tabela `:384-461`; **0 das 7 linhas pedidas** em W2 |
| Matriz medida | **8 de 76** | `build/zeebo/matrix_r/summary.txt` **ainda de 23/11 20:18** (6 rows: 3, 37, 39, 46, 65, 71) + row 40 (24/11 13:2x) + row 35/Chaos 0 (`74d283a`) |
| `.err` do build | 12.240 arquivos no total, **0** com `error:` em `game/obj` | só o aviso de apóstrofo em `engine/native/prelude.h:18` |
| Disco | `/` a **98%**, **12 GB** livres (432/467 G) | era 13 GB às 17:25; `keep/` 6,9 G, `game/` 5,8 G, `game_o2/` 1,2 G |
| Ledger de estado | `tools/estado_hash.ref` de 17:38 (= binário de `b08f0d1`) | **não vale** para a árvore atual |

#### O que mudou na árvore (sem commit): o passe 2D

- **`zstrip.c:38`** — `SCREEN_NEAR (-(16<<16))` / `SCREEN_FAR (1<<16)`
  substituem o ortho −1..+1 em `mode_screen` (`:470`), no ortho do caminho
  translúcido (`:920`, `:1002`) e no `zscene_gl_init` (`:1513`, `:1572`). O
  comentário da mudança: prioridades 2D do Ninja chegam como **z=9..15** no menu
  de pause e o ortho −1..1 recortava o painel inteiro.
- **`zstrip.c:733` e `:925`** — `glDisable(GL_DEPTH_TEST)` no *flush* opaco e no
  laço translúcido, religado em `:1005`/`:1029`/`:1569`: "as prioridades do
  Ninja ordenam os quads entre si; testar contra o Z da cena escondia a HUD".
  É o caminho do **HUD sem cor / canto preto do Egg Carrier (row 46)** e do
  painel de pause.
- **`zstrip.c:966-972`** — `EMIT_RUN()` passou a sair **antes** de ligar
  textura/blend do strip corrente. Antes o run pendente era emitido **depois**,
  e o estado ficava deslocado em um strip: "botão do pause com textura do
  cenário, halos e quads gigantes". Esta é a causa provável da família do bug
  de textura do `todo.md:12` (falta confirmar com state).
- **`overlay.c:888-899`** — a sombra lateral do painel de pause usa
  `PART_SIDE_SHADOW` sob `ZEEBO_PORT` (o original repete `kado_shado` em todo
  tile interno; a borda fica em blocos).

**Não medido:** nenhum quadro, nenhum hash, nenhuma linha da matriz, e o gate
`estado_hash.ref` **não foi rodado** (com este passe 2D ele deve divergir). Se o
código entrar, a regra de 17:09 vale: `hash_ref.sh update` no mesmo commit.

#### O que falta para o OBJETIVO FINAL (port completo, todo o conteúdo)

| Conteúdo | Estado em 19:30 | Falta |
|---|---|---|
| Fases de ação (stg00–stg10, stg12) | 12 programas linkados; só a row 3 (Emerald Coast) tem medida, e é de 23/11 20:18 | atos 2–4 de todas; Hot Shelter (stg12) **sem linha no menu**; nada medido com o `.mod` atual |
| Station Square | `adv00` linkado; row 40 rodou (câmera num pilar, sem HUD) | rows 41–45 nunca medidas |
| Mystic Ruins | `adv02` + `tikal_prog`; **nenhuma** das rows 58–61 medida | as 4 áreas |
| Egg Carrier / por dentro | `adv01obj`+`adv0100`/`adv0130`; row 46 escura, HUD sem cor (`71b404f1d722`) | rows 47–57 nunca medidas; o passe 2D da árvore ataca o HUD, falta medir |
| Past | `adv03` + `tikal_prog`; rows 62–64 nunca medidas | as 3 áreas |
| Chefes | Chaos 0 (row 35) medido e otimizado; Chaos 4 (row 37) medido, escuro/esverdeado (`70b135d59597`); Zero (row 39, geometria errada, `c2eb03f3a477`) | Chaos 2 (row 36), Egg Viper 1 (row 38) e E101-R não medidos com este build; **Chaos 6, Chaos 7, Egg Viper 2/3 (EGM2/EGM3), E101, E101-R: sem linha no menu**; crash ao vencer o Chaos 0 sem log (`todo.md:20`) |
| Chao Garden / corrida | `al_main`+`al_garden00-02`+`al_race` linkados; rows 69–72 não medidas com este `.mod` | as 3 áreas e a corrida |
| Minijogos | Sky Chase 1/2 rodam; Sand Hill roda (row 68); Twinkle Circuit (row 65, `52455310e0db`) 95% preto | pista do Circuit e a sequência de teclas da corrida |
| 2D / sprites | passe 2D novo **na árvore** (ortho −16..1, depth off, `EMIT_RUN` antes do estado) | commit + medida; `zstrip_tex_release_all` não limpa `tex_names[]` (`zstrip.c:1376-1386` na árvore, `:1357` no `HEAD`) |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1` | 448 linhas inteiras (hoje pula metade, `zplm.c:149-151`) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes sustentados, pitch bend (`src/sound/sound.c:757`), vozes de evento longo, SFX 259 ausente (`todo.md:38`) |
| Save | cartão emulado, data do RTC | save no campo não exercitado |
| Desempenho | Station Square ~34–36 ms; Chaos 0 quase sem quadro de 3 vsyncs | ms no Adreno 130 não medidos (sem aparelho) |
| Pacote | `.mod` 35,64 MiB + 242 `.ovl` | cortar ~6 MB; instalador |

#### 20 quick wins (W1–W20, reabertos em 24/11 19:30; P ≤ 1 h, M ≤ meio dia)

**Fechados até aqui, para não refazer:** K7/A3 (cache da modelview, `5bcf387`),
K6/A2 (medido e desligado atrás de `Z_TJOIN=0`), K3 parcial (disco saiu de
100% para 98%). **Feito na árvore, falta commit/medida:** passe 2D (ortho −16..1,
depth off, `EMIT_RUN` antes do estado) e sombra lateral do pause.

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| W1 | Rodar a matriz inteira (76 rows) com o `.mod` de hoje, quadro + log por linha | M | **8 de 76**; `tools/stage_matrix.sh` faz 29 por vez; `build/zeebo/matrix_r/summary.txt` ainda de 23/11 20:18 |
| W2 | 7 linhas no menu de debug (Hot Shelter, CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R) | P | 76 linhas, 73 `STAGE_*` + 3 zeros; `src/progress/title.c:202` e tabela `:384-461`; os 7 programas já estão em `PROGRAMS` (`ports/zeebo/game.mk:152-153`) |
| W3 | Commitar e medir o passe 2D da árvore (`zstrip.c` + `overlay.c`) com `hash_ref.sh update` no mesmo commit | P | `zstrip.c:38,470,733,920,925,966-972,1002,1005,1029,1513,1569`; `overlay.c:888-899`; gate não rodado, `.ref` é de 17:38 |
| W4 | Repetir o Chaos 0 3× e fixar os quadros de 3 vsyncs | P | `zperf`; a mesma medida deu 94, 154 e 162 antes do tique de 16,667 ms — "20 de ~2380" é de 1 rodada |
| W5 | `zstrip_tex_release_all` limpar `tex_names[]` | P | `zstrip.c:1376-1386` na árvore (`:1357` no `HEAD`): zera `tex_count` e deleta nomes GL, array fica com nomes mortos |
| W6 | Confirmar com state novo se o `EMIT_RUN` reordenado resolve o painel de pause (título em faixa preta) | M | `todo.md:12`; reproduzido com `session --state` slot-3 16:44; precisa de state salvo com build instrumentado |
| W7 | Crash ao vencer o Chaos 0 sem log (pico de 760 MB para 64 MB emulados) | M | `todo.md:20`; `.log`/`.serial.log` mais recentes são de 15:58; pedir "Exportar .log" ou state com o chefe enfraquecido |
| W8 | A2 com vértices degenerados para ligar `Z_TJOIN` | M | 8,1% dos draws (824.465 → 757.656 em 60 s), 10.659 px espúrios; `zstrip.c:26-31,983`; exige cópia no pool |
| W9 | B1: cachear a inversa por matriz de origem nos welds | M | `f__njInvertMatrix` 5,4% das amostras; 282 chamadas (`path_editor_reorder` 126, `event_motion_weld` 62, `event_weld` 32) |
| W10 | B3: colisão com coerência (cache do último polígono por consulta) | M | `polygon_collect_near` ~10% + `world_list_near` ~6%; 7 chamadas de `world_list_near` por jogador por quadro |
| W11 | `draw_meshset_gl`: tirar `zstrip_model_vertex` por vértice do caminho quente | M | ~4–5% do perfil depois do `-O2` (24/11 noite, 2) |
| W12 | Zero (row 39): geometria errada (campo teal listrado, sem o robô) | M | `matrix_r/summary.txt`: hash `c2eb03f3a477`, `traps=0 ausente=0`; iluminação `simple` caiu no `draw_basic` |
| W13 | Twinkle Circuit (row 65): pista 95% preta e sequência de teclas da corrida | M | hash `52455310e0db`; `DEBUG_TITLE=65` |
| W14 | Egg Carrier (row 46): HUD sem cor e canto superior preto | M | hash `71b404f1d722` (466 cores); rows 47–57 não medidas; PASSE 2D da árvore mexe exatamente aqui — medir com ele |
| W15 | Cortar ~6 MB do `.mod` (35,64 MiB contra 30–32 MB) | M | `sa1g.mod` 37.375.564 B; 242 `.ovl` = 32.114.443 B; o `.mod` já reserva a região dos overlays |
| W16 | Filmes em 448 linhas inteiras (`step=1`) | M | `zplm.c:149-151` (`while height/step > maxh: step++` → 224 de 448) |
| W17 | Vozes de evento longo (`ADXT_StartAfs` além do menu) | P | só o flid 0 do menu foi correlacionado (0,88 / ganho 0,98); `zaudio.c` |
| W18 | Clipes sustentados (17/179 repetem o clipe inteiro) e pitch bend ignorado | M | `zaudio.c` (intro+laço por `MM_STATUS_DONE`); `src/sound/sound.c:757` |
| W19 | Espaço: `keep/` com 80 builds (6,9 GB) e `game_o2/` (1,2 GB) | P | `/` a **98%, 12 GB livres** (era 13 GB às 17:25); só apago com autorização |
| W20 | Boomerang medido no zeebx (teste de PC já passa) | P | `zboom.c`, `zhid.c`, `tests/boom_test.c`; pendente desde `fe22ce0` |

Fora dos 20, ainda abertos: névoa nas outras áreas e modos PVR 1/2 (K18), o aviso
de apóstrofo em `engine/native/prelude.h:18` (cosmético, suja 8.166 `.err`) e
`CHECK_GAME=1` parado em "não chegou à seleção do cartão (`ava_vmssel`)".

## Atualização 2026-11-24 21:35 — 751 objetos e 259 indefinidos: os 7 arquivos de movimento entram como overlay

Rodada do cron de 2 h, 21:34–21:50. `HEAD` = `ab4e50b` (19:34, o commit de docs
da rodada anterior), branch `zeebo-port`; **nenhum commit de código** desde
`05b53ea`. A árvore recebeu trabalho de outra sessão entre 20:44 e 20:53 (o
passe 2D de 18:03 continua nela): `ports/zeebo/game.mk` (+13/−2),
`ports/zeebo/zstrip.c` (+59/−8), `zstrip.h` (+2) e
`modules/advertise/preview.c` (+9) — diffs contra o `HEAD`, somando as duas
janelas. O `make` do applet já estava feito (20:53) e o `mod` desta rodada
(21:36) linkou tudo — os números abaixo são dessa árvore, não do `HEAD`.

| Item | Medido agora (21:36) | Observação |
|---|---|---|
| `make` do applet | rc=0, nada a recompilar | `zstrip.o`, `sa1z.elf`/`.mod` de 20:53 (outra sessão) |
| `make -f game.mk mod` | **751 de 751 objetos**, `duplicatas: 0`, **indefinidos: 259** | era 744/560 às 19:30: **−301 indefinidos (−53,8%)** |
| `sa1g.mod` | **37.869.740 B** (36,12 MiB), 21:36 | +494.176 B (+0,48 MiB) sobre os 37.375.564 B de 19:32 |
| `.ovl` | **249** = **33.347.291 B** | 242 → 249: os 7 `*_mot` |
| `zovl_motion` | **0x01434a70**, tamanho **0x99680** (628.352 B) | região própria entre `tikal` (0x01400750) e `evento` (0x014ce0f0); vale o maior dos 7 |
| `host-selftest` | **PASSED (0 failures)** | linka `fast.c`/`zfpu.c` |
| `PROGRAMS` | **251** = 47 (40 módulos + 7 `*_mot`) + 204 eventos | `modules/` tem 41 dirs (fora: `movie`); `MISSING_PROGRAMS` **vazio** (`zprogs.c` só o terminador `0`) |
| Ações de personagem | **0 indefinidos** `sonic_*`/`amy_*` | `sonic_clear_action` virou `D` em `program-s_mot.o` |
| Indefinidos por família | alife **68**, `dat_0c…` **65** (33 símbolos), outros **61**, `bu*` **29**, `tikal_*` **16**, `card_*` **12**, `chaos6_*` **8** | base de W6–W12 |
| Menu de debug | **76** linhas, **0 das 7** de W2 | `src/progress/title.c:202`, tabela `:384-461` |
| Matriz medida | **8 de 76** | `build/zeebo/matrix_r/summary.txt` ainda de 23/11 20:18 (rows 3, 37, 39, 46, 65, 71) |
| `.err` do build | **8.166** arquivos, **0** com `error:` | só o aviso de apóstrofo em `engine/native/prelude.h:18` |
| Disco | `/` a **98%**, **12 GB** livres (432/467 G) | `keep/` 6,9 G, `game/` 6,0 G, `game_o2/` 1,2 G |
| Ledger de estado | `tools/estado_hash.ref` de 17:38 | **não vale** para esta árvore |

#### O que mudou na árvore (sem commit)

- **`game.mk:146-153` e `:170-180` — `MOTION_PROGRAMS`.** Os sete arquivos de
  movimento (`a_mot b_mot e_mot k_mot s_mot m_sbmot s_sbmot`) passaram a ser
  compilados e linkados como **overlays ARM relocáveis** no grupo `motion`, em
  vez de lidos como PRS cru em `zdc_ram`. O comentário da mudança registra o
  sintoma: "ponteiros absolutos do DC… ler o PRS cru não define os símbolos
  importados pelo 1ST_READ (`sonic_clear_action` ficava zerado após o Chaos 0)".
  Efeito medido: **744 → 751 objetos**, **242 → 249 `.ovl`**, **560 → 259
  indefinidos**, `sonic_*`/`amy_*` a zero. Os 7 se substituem entre si no
  `MOTION_AREA`, então a região é dimensionada pelo maior `.ovl_*`
  (`b_mot`: 628.300 B no `.o` → 628.352 B alinhados), não pela soma; o
  `.ovl` em disco do `b_mot` tem só 273.899 B de dados (o resto é BSS/zero).
  **Não medido no zeebx**: falta ver a pose de vitória e o `MOTION_AREA`
  convivendo com stage/chefe depois da carga.
- **`zstrip.c:108-127` + `preview.c:423` — `zstrip_screen_depth()`.** A seleção
  de personagem mistura preview 3D e roda 2D **no mesmo depth buffer**: só esse
  draw religa `GL_DEPTH_TEST` (`preview.c:423`), e o começo de cada quadro volta
  a desligar (`zstrip.c:1566`, `:1576`). HUD e pause continuam por ordem de
  pintura, sem competir com o Z da cena — o caminho do row 46 e do painel de
  pause. `zstrip_screen_depth` já está no `.mod` (`T` em 0x156c).
- O **passe 2D** de 18:03 continua na árvore exatamente como descrito na
  atualização das 19:30 (ortho −16..1, depth off, `EMIT_RUN` antes do estado);
  nada dele foi commitado nem medido. `overlay.c` (18:01) não mudou nesta
  janela.

**Não medido:** nenhum quadro, nenhum hash, nenhuma linha da matriz, e o gate
`estado_hash.ref` não foi rodado (com o passe 2D ele deve divergir). Se o código
entrar, `hash_ref.sh update` no mesmo commit.

#### O que falta para o OBJETIVO FINAL (port completo, todo o conteúdo)

| Conteúdo | Estado em 21:36 | Falta |
|---|---|---|
| Fases de ação (stg00–stg10, stg12) | 12 programas linkados; só a row 3 (Emerald Coast) tem medida, de 23/11 20:18 | atos 2–4 de todas; Hot Shelter (stg12) **sem linha no menu**; nada medido com este `.mod` |
| Station Square | `adv00` linkado; row 40 rodou (câmera num pilar, sem HUD) | rows 41–45 nunca medidas |
| Mystic Ruins | `adv02` + `tikal_prog`; nenhuma das rows 58–61 medida | as 4 áreas; `tikal_player_step` (14 refs) ainda indefinido |
| Egg Carrier / por dentro | `adv01obj`+`adv0100`/`adv0130`; row 46 escura, HUD sem cor (`71b404f1d722`) | rows 47–57 não medidas; o passe 2D + `screen_depth` atacam o HUD, falta medir |
| Past | `adv03` + `tikal_prog`; rows 62–64 nunca medidas | as 3 áreas; mesmo `tikal_player_step` |
| Chefes | Chaos 0 (row 35) medido e otimizado; motion de vitória agora linkado; Chaos 4 (row 37) escuro (`70b135d59597`); Zero (row 39) geometria errada (`c2eb03f3a477`) | **Chaos 6, Chaos 7, EGM2/EGM3, E101, E101-R: sem linha no menu**; `chaos0_event_form/melt/action` (2 refs cada) e `chaos6_*`/`chaos7_*` (20) indefinidos — candidatos do crash ao vencer o Chaos 0 (`todo.md:20`) |
| Chao Garden / corrida | `al_main`+`al_garden00-02`+`al_race` linkados | as 3 áreas e a corrida; **`alife_*` 68 refs** (`alife_face_set_eyes` 13, `alife_face_set_mouth` 10) indefinidos |
| Minijogos | Sky Chase 1/2 rodam; Sand Hill roda (row 68); Twinkle Circuit (row 65, `52455310e0db`) 95% preto | pista do Circuit e a sequência de teclas da corrida |
| 2D / sprites | passe 2D + `zstrip_screen_depth` **na árvore** | commit + medida; `zstrip_tex_release_all` não limpa `tex_names[]` (`zstrip.c:1394-1405`) |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1` | 448 linhas inteiras (hoje pula metade, `zplm.c:149-151`) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes sustentados, pitch bend (`src/sound/sound.c:757`), vozes de evento longo, SFX 259 ausente (`todo.md:38`) |
| Save | cartão emulado, data do RTC | save no campo não exercitado; `card_*` 12 refs indefinidos (`card_menu_drives` 4) |
| Desempenho | Station Square ~34–36 ms; Chaos 0 quase sem quadro de 3 vsyncs | ms no Adreno 130 não medidos (sem aparelho) |
| Pacote | `.mod` 36,12 MiB + 249 `.ovl` | cortar ~6 MB; instalador |

#### 20 quick wins (W1–W20, abertos em 24/11 21:36; P ≤ 1 h, M ≤ meio dia)

**Fechados até aqui, para não refazer:** K7/A3 (cache da modelview, `5bcf387`),
K6/A2 (medido e desligado atrás de `Z_TJOIN=0`), K3 parcial (disco saiu de 100%
para 98%). **Feito no build desta rodada, falta commit/medida:** os 7 overlays
de movimento (W1) e o depth do preview na seleção (W2).

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| W1 | Commitar e medir os 7 `*_mot` como overlay: pose de vitória após chefe e `MOTION_AREA` convivendo com stage | M | `game.mk:146-153,170-180`; **751/751**, **259 indefinidos** (eram 560), `sonic_*`/`amy_*` = 0; `zovl_motion` 0x01434a70/0x99680; `.mod` 37.869.740 B |
| W2 | Commitar e medir o passe 2D + `zstrip_screen_depth` (preview/roda da seleção) com `hash_ref.sh update` no mesmo commit | M | `zstrip.c:38,108-127,488,751,938-943,987-988,1020-1023,1531,1566,1576,1592`; `preview.c:423`; `overlay.c:888-899`; gate não rodado, `.ref` de 17:38 |
| W3 | 7 linhas no menu de debug (Hot Shelter, CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R) | P | 76 linhas, 73 `STAGE_*` + 3 zeros; `src/progress/title.c:202` e `:384-461`; os 7 já estão em `PROGRAMS` (`game.mk:157-158`) |
| W4 | Rodar a matriz inteira (76 rows) com o `.mod` de hoje, quadro + log por linha | M | **8 de 76**; `tools/stage_matrix.sh` faz 29 por vez; `summary.txt` de 23/11 20:18 |
| W5 | Repetir o Chaos 0 3× e fixar os quadros de 3 vsyncs | P | `zperf`; a mesma medida deu 94, 154 e 162 antes do tique de 16,667 ms |
| W6 | Fechar o crash ao vencer o Chaos 0: ligar `chaos0_event_form/melt/action` (2 refs cada) | M | `todo.md:20`; `grep undefined` em `integrate.err`; `.log` mais recente de 15:58 |
| W7 | `alife_face_set_eyes` (13) e `alife_face_set_mouth` (10): rosto dos Chao | M | **alife 68 refs** no `integrate.err`; `al_main`/`al_garden00-02` já linkados |
| W8 | API de arquivo `bu*` (29 refs: `buFindExecFile` 8, `buLoadFileEx` 6, `buGetDiskFree` 6, `buRewriteExecFile` 5, `buSaveExecFile` 4) | M | `integrate.err`; afeta save/cartão e leitura de exec em runtime |
| W9 | `tikal_player_step` (14 refs): Tikal jogável em Mystic Ruins/Past | M | 16 refs `tikal_*`; `tikal_prog` já é overlay próprio (`game.mk:172`) |
| W10 | `chaos6_event_*` (8) e `chaos7_event_*` (12): Chaos 6 e 7 | M | `integrate.err`; ambos já em `PROGRAMS`, sem linha no menu (W3) |
| W11 | Resolver os 65 refs a 33 `dat_0c…` (endereços absolutos de DC dos `symbols.txt`) | M | ex.: `dat_0cac8a7c` em `config/adv00/symbols.txt:1340`; vêm de eventos/stages, não do port |
| W12 | `card_*` (12 refs, `card_menu_drives` 4): menu de cartão de memória | M | `integrate.err`; `CHECK_GAME=1` para em `ava_vmssel` |
| W13 | `zstrip_tex_release_all` limpar `tex_names[]` | P | `zstrip.c:1394-1405`: zera `tex_count` e deleta nomes GL, o array fica com nomes mortos |
| W14 | Zero (row 39): geometria errada (campo teal listrado, sem o robô) | M | hash `c2eb03f3a477`, `traps=0 ausente=0`; iluminação `simple` caiu no `draw_basic` |
| W15 | Twinkle Circuit (row 65): pista 95% preta e sequência de teclas | M | hash `52455310e0db`; `DEBUG_TITLE=65` |
| W16 | Egg Carrier (row 46): HUD sem cor e canto preto; medir com o passe 2D | M | hash `71b404f1d722` (466 cores); rows 47–57 não medidas |
| W17 | Atos 2–4 das 12 fases de ação e a linha do Hot Shelter (stg12) | M | `PROGRAMS` (`game.mk:157`); única medida de fase é a row 3 (23/11) |
| W18 | Filmes em 448 linhas inteiras (`step=1`) | M | `zplm.c:149-151` (`while height/step > maxh: step++` → 224 de 448) |
| W19 | Clipes sustentados (17/179 repetem o clipe inteiro) e pitch bend ignorado | M | `zaudio.c` (intro+laço por `MM_STATUS_DONE`); `src/sound/sound.c:757` |
| W20 | Save no campo e conferir a data do RTC | P | `sa1vmu.bin`; `zsys.c` (`GETJULIANDATE`); nunca exercitado em campo |

## Atualização 2026-11-24 22:19 — Chaos 0 e preview 3D fechados e verificados

A árvore de 21:35 foi exercitada de ponta a ponta antes do commit:

- **Freeze após Chaos 0 fechado:** `s_mot.ovl` carregou antes do chefe e ficou na
  região `motion`, separada de `b_chaos0.ovl`. Um probe temporário reduziu o chefe
  ao último hit, sem saltar a máquina de estados: o jogo executou hit, melt,
  `stage_clear`, pose, placar e saiu de ACTION após 6.535 quadros para MENU e
  MENU_RUN. A sessão continuou até 150 s, sem `Budget`, callback preso ou trap;
  `pose_node` caiu de 97% no freeze para 1,6% das amostras. O probe foi removido.
  Evidência preservada em
  `~/.hermes/cache/scratch/chaos0-motion-autoclear/`.
- **Preview 3D fechado por A/B isolado:** os draws do modelo chegavam ao
  rasterizador, mas a roda 2D posterior sem depth o cobria. Ligar depth só para
  o preview mudou o MD5 de `903d1dee4859fbb2784a4d14b12d2bbb` (sem Sonic) para
  `da3a8551e3aea374e4f9e4fe5c70e610` (Sonic visível). A implementação localizada
  (`preview_draw` → `zstrip_screen_depth(1)`) produz exatamente o mesmo hash da
  referência A/B e restaura o modo 2D no quadro seguinte. Fora da seleção, o
  fluxo de 70 s terminou byte a byte igual (`eac52efd998f51f5f9c45b6a65e357c3`).
  Evidências em `char-preview-run37`, `char-preview-depth-on`,
  `char-preview-targeted` e `char-preview-targeted-flow` no scratch do Hermes.
- **Pause/HUD preservados:** Emerald Coast com pause aos 45 s mostrou painel,
  TIME, anéis, vidas e cena 3D. `src/hud/overlay.c` faz parte desta entrega: é a
  correção anterior da sombra do painel, trocando nos tiles internos a textura
  de canto repetida (`PART_CORNER_SHADOW`) pela lateral contínua já preparada
  pelos dados (`PART_SIDE_SHADOW`) somente no Zeebo.
- **Gates finais:** `751/751 objetos`, `duplicatas: 0`, `259 indefinidos`,
  `host-selftest` PASSED, `novfp` com 0 instruções VFP e `hash_ref: IGUAL`.
  Build release normal: `sa1g.mod` 37.870.332 B; 249 overlays, 33.347.139 B.
  Captura do gate de pause em
  `~/.hermes/cache/scratch/final-pause-hud-gate/`.

W1, W2 e W6 estão fechados. O próximo gate funcional para movimentos deve
exercitar também a pose de vitória dos outros personagens; os sete arquivos
`*_mot` já seguem o mesmo caminho relocável.

## Atualização 2026-11-24 23:20 — crash depois do resultado do Chaos 0

Log do Rafael (`~/.config/zeebx/relatorios/Sonic Adventure 1 (Zeebo Port).serial.log`): a
animação e a contagem de pontos funcionaram e, 20 s depois (RNDCLEAR.ADX em
141,7 s, `ev0002.ovl` em 160,6 s), o applet caiu com
`fault at 0xe0000000 pc 0x2cd6a0 lr 0x14b084`. Traduzindo (ELF = guest − 0x10200):
`_builtin_prefetch` chamada de `text_print_front+0x178`. A origem é `src/draw/text.c`,
que escreve o texto nas store queues do SH-4 (`SQ_BASE` 0xe0000000, QACR0/1), e
`card_frame_update` o chama no aviso do cartão de memória que vem depois da fase.

Correção: `gen_patch.py` remenda `text.c` (como já faz com `njDebug.c`): `TEXT_OPEN`
passa a escrever num rascunho `ztext_sq[16384]` e `TEXT_CLOSE` só entrega as células
a `present_record`. Prova: um probe temporário chamando `text_print_front` por mais
de 200 quadros no menu terminou sem falha (`text_print_front voltou`, 201 e 400),
onde antes a primeira chamada derrubava o jogo. O probe foi removido. Gates:
751/751, duplicatas 0, `host-selftest`, `novfp` 0 e `hash_ref: IGUAL`. **Não medido:** o
texto do aviso ainda não aparece na tela (a captura do probe não o mostrou); o aviso
do cartão depende de `present_record` e de um desenho de texto no port.

## Atualização 2026-11-24 23:40 — build limpo em `469f602`; o gate de desenho não se reproduz

Rodada do cron de 2 h (23:40–23:57). `HEAD` = `469f602` (23:18), branch
`zeebo-port`; árvore **limpa** — `git status` só acusa `sa1n.bmp` e `todo.md`,
não rastreados. **Nenhum commit nesta janela.** Os dois commits desde a nota das
22:19 são `a5f8fa3` (22:40 — `_mot` como overlay, depth do preview, passe 2D,
`overlay.c`) e `469f602` (23:18 — `text.c` sem store queue), ambos já descritos
em 22:19/23:20; o que falta desde então é justamente **medir os gates com esse
`HEAD`**, e é aí que esta rodada achou um problema.

| Item | Medido agora (23:46) | Observação |
|---|---|---|
| `make` do applet | rc=0, nada a recompilar | `sa1z.elf` de 23:33 (300.100 B) |
| `make -f game.mk mod` | **751 de 751 objetos**, `duplicatas: 0`, **indefinidos: 259** (112 símbolos) | nenhum `FALHOU` |
| `sa1g.mod` | **37.834.444 B** (36,08 MiB), 23:42 | 35.888 B menor que os 37.870.332 B de 22:19: efeito do patch de `text.c` |
| `.ovl` | **249** = **33.346.090 B** (31,80 MiB) | inalterado em número |
| Regiões `zovl_*` | campo 0x011bb150, extra 0x013ba6b0, tikal 0x013f3d90, **motion 0x014280b0** (`0x99680`), evento 0x014c1730 | motion saiu de 0x01434a70 (21:36) para 0x014280b0 — 51.648 B (`0xc9c0`) abaixo; o `ztext_sq[16384]` mudou o BSS, mas a causa exata do deslocamento não foi medida |
| `host-selftest` | **PASSED (0 failures)** | linka `fast.c`/`zfpu.c` |
| `novfp.sh` | **OK, 0 instruções VFP** | `build/zeebo/sa1g.elf` |
| `hash_ref.sh check` | **DIFERENTE** (rc=1) | ref é de 17:38 e não foi refeita em nenhum dos dois commits |
| Repetição do gate | 2 rodadas do **mesmo** `.mod` + o `.ref` de 17:38: 3088 quadros nas três, hashes 1–3 iguais, **4º diverge** (0x16ce3a75 no `.ref` × 0xae7f30f5 × 0x7f49ba75) | o estado de desenho não é determinístico hoje |
| `PROGRAMS` | **251** = 47 (40 módulos + 7 `*_mot`) + 204 eventos | `modules/` 41 dirs (fora: `movie`); `MISSING_PROGRAMS` **vazio** (`zprogs.c` só `0`) |
| Indefinidos por família | alife **68**, `dat_0c…` **65** (33 símbolos), `bu*` **29**, `tikal_*` **16**, `chaos7_*` **15**, `card_*` **12**, `chaos6_*` **8**, `chaos0_*` **6**, outros **40** | `strncmp` 5, `parked_car_object` 3, `garden_camera_step` 3, `fun_0c90d634`/`fun_0c9002ac` 2 cada |
| Menu de debug | **76** linhas: 73 estágios + 3 zeros | `src/progress/title.c:202` (`DEBUG_STAGES 76`), tabela `:384-460`; **0 das 7** linhas de W2 |
| Matriz | **7 de 76 com artefato** em `build/zeebo/matrix_r/` (rows 3, 37, 39, 46, 65, 68, 71) | `summary.txt` (23/11 20:18) tem só 6 — a row 68 (`row68.log`, hash `8f52bbe305a4`, 1206 quadros) está fora dele; a contagem antiga de 8 incluía a row 40, que não deixou artefato |
| `.err` do build | **12.247** arquivos, **0** com `error:` | só o aviso de apóstrofo em `engine/native/prelude.h:18` |
| Disco | `/` a **98%**, **9,2 GB** livres (434/467 G) | eram 12 GB às 21:36: **−2,8 GB em 2 h**; `keep/` 6,9 G, `game/` 6,3 G, `game_o2/` 1,2 G |

#### O achado desta rodada: `hash_ref` vermelho e não reprodutível

`ports/zeebo/tools/hash_ref.sh check` devolve **`hash_ref: DIFERENTE`**. O
`.ref` (`tools/estado_hash.ref`, 48 hashes) é de **17:38** e não foi refeito em
`a5f8fa3` nem em `469f602` — a regra escrita no próprio arquivo ("quando o
desenho muda, `hash_ref.sh update` no mesmo commit") foi violada duas vezes.
Mais grave: rodando o comando interno do script **duas vezes com o binário
atual** (além da execução do `check`), todas as rodadas apresentam **3088
quadros** e os hashes 1–3 idênticos (`0xf03ecc58`, `0x5ed4d058`, `0x710a83b5`),
mas **divergem no 4º** (0xae7f30f5 na primeira e 0x7f49ba75 na segunda; o `.ref`
de 17:38 traz 0x16ce3a75) e seguem diferentes até o fim. Duas execuções do
mesmo `.mod` não produzem o mesmo desenho, então o gate **não serve de prova**
neste estado — e a nota "hash_ref: IGUAL" de 23:20 não se reproduz aqui.

O que já está descartado: `DYNRES` não é a causa — `DYNRES ?= 0`
(`ports/zeebo/Makefile:74`) e o default do `brew_egl.c:104-105`, sem nenhum
`-DZ_DYNRES` no `game.mk` (o `CFLAGS_DYNRES` de `Makefile:85` sai vazio com 0). Próximo passo
barato: rodar as duas execuções com `ZEEBX_DRAWLOG`/`--dump-gl` e comparar os
quadros 193–256 (o 4º hash cobre 64 quadros a partir do 193) para achar a
primeira diferença de desenho;
suspeitas medidas a testar: (a) índice/nome de textura dependente da ordem de
abertura dos arquivos no `zstrip`, (b) 16 KB de `ztext_sq` no BSS mudando o que
sobra de heap, (c) ordem do `EMIT_RUN` no passe 2D. **Só depois** de estabilizar,
`hash_ref.sh update` no mesmo commit da correção.

#### O que mudou desde as 23:20 (só medida, nenhum código)

- Nada foi commitado nem editado na árvore depois de `469f602`; a árvore é a
  mesma que o commit descreve.
- Comprovado que o patch de texto está aplicado de fato no build:
  `build/zeebo/game/patched/src/draw/text.c:72` (`ztext_sq[16384]`) e `:286`
  (`text_sq = ztext_sq`), gerados por `gen_patch.py:28-34,73-77` — e ainda assim
  o aviso do cartão **não foi visto na tela** (falta o desenho por
  `present_record`).
- O `.mod` encolheu 35.888 B e as regiões de overlay andaram **51.648 B**
  (`0xc9c0`) para baixo: `zovl_motion` de 0x01434a70 para 0x014280b0 e o
  `zovl_evento` de 0x014ce0f0 para 0x014c1730. Qualquer endereço de overlay
  anotado antes de 23:18 está desatualizado.

#### O que falta para o OBJETIVO FINAL (port completo, todo o conteúdo)

Mapa das 76 linhas do menu (`title.c:384-460`), para não errar row: **0–2**
Practice, **3–5** Emerald Coast, **6–8** Windy Valley, **9–11** Twinkle Park,
**12–14** Speed Highway, **15–17** Red Mountain, **18–20** Sky Deck, **21–23**
Lost World (`STAGE_RUIN`), **24–27** Ice Cap, **28–31** Casinopolis, **32–34**
Final Egg, **35** Chaos 0, **36** Chaos 2, **37** Chaos 4, **38** Egg Viper 1
(`EGGMOBILE1`), **39** Eggman robo ("Zero" nos docs), **40–45** Station Square,
**46–51** Egg Carrier, **52–57** Egg Carrier por dentro, **58–61** Mystic Ruins,
**62–64** Past, **65** Twinkle Circuit (`CART`), **66** Sky Chase 1, **67** Sky
Chase 2, **68** Sand Hill (`SUNABOARD`), **69–72** Chao Garden (SS/EG/MR/corrida),
**73–75** zeros.

| Conteúdo | Estado em 23:46 | Falta |
|---|---|---|
| Emerald Coast (3–5) | row 3 medida em 23/11 (`575323d451b6`), atos 2–3 nunca | 2 linhas |
| Windy Valley (6–8), Twinkle Park (9–11), Speed Highway (12–14), Red Mountain (15–17), Sky Deck (18–20), Lost World (21–23), Ice Cap (24–27), Casinopolis (28–31), Final Egg (32–34) | 9 fases, 29 linhas, **nenhuma medida** com `.mod` algum | 29 linhas + os atos |
| Hot Shelter (`stg12`) | programa linkado (`game.mk:160`), **sem linha no menu** | linha + medida; único stage sem entrada |
| Station Square (40–45) | row 40 rodou uma vez (24/11 13:2x, sem artefato salvo) | rows 41–45 |
| Egg Carrier (46–51) e por dentro (52–57) | row 46 escura, HUD sem cor (`71b404f1d722`, 466 cores) | 11 linhas |
| Mystic Ruins (58–61) e Past (62–64) | nunca medidas; `tikal_player_step` 14 refs | 7 linhas + Tikal jogável |
| Chefes | Chaos 0 (35) medido e otimizado; Chaos 2 (36) nunca; Chaos 4 (37) escuro (`70b135d59597`); Egg Viper 1 (38) nunca; "Zero" (39) geometria errada (`c2eb03f3a477`); Chaos 6/7, EGM2/3, E101, E101-R **sem linha** (e `chaos6_*` 8 + `chaos7_*` 15 indefinidos; E101/E101-R linkam, só não têm entrada) | 3 chefes com linha faltando medir + 6 programas sem linha |
| Chao Garden (69–72) | `al_main`+`al_garden00-02`+`al_race` linkados, 0 medidas | 4 linhas; **alife 68 refs** (`alife_face_set_eyes` 13, `_mouth` 10, `garden_camera_step` 3) |
| Minijogos | Sky Chase 1/2 rodam; Sand Hill roda (row 68, `8f52bbe305a4`); Twinkle Circuit (65) 95% preto | pista do Circuit e a sequência de teclas da corrida |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1` | 448 linhas inteiras (`zplm.c:149-151` pula metade) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes sustentados, pitch bend (`src/sound/sound.c:757`), vozes longas, SFX 259 (`todo.md:38`), `syCfgSetSoundMode` |
| Save | cartão emulado, data do RTC | save no campo nunca exercitado; `card_*` 12 refs (`card_menu_drives` 4); `CHECK_GAME=1` para em `ava_vmssel` |
| Texto/2D | passe 2D e depth do preview commitados e verificados; texto do aviso não aparece | `present_record` desenhar o texto; `zstrip_tex_release_all` não limpa `tex_names[]` (`zstrip.c:1394-1405`) |
| Desempenho | Station Square ~34–36 ms no zeebx; Chaos 0 quase sem quadro de 3 vsyncs | ms no Adreno 130 (sem aparelho); **gate de desenho instável** (acima) |
| Pacote | `.mod` 36,08 MiB + 249 `.ovl` (31,80 MiB); disco com 9,2 GB livres | cortar ~6 MB; instalador; espaço |

#### 20 quick wins (W1–W20, abertos em 24/11 23:46; P ≤ 1 h, M ≤ meio dia)

**Fechados nas rodadas anteriores, para não refazer:** os 7 `*_mot` como overlay
(`a5f8fa3`), o passe 2D + `zstrip_screen_depth` do preview (`a5f8fa3`), o freeze
pós-Chaos 0 (medido até MENU_RUN) e o texto sem store queue (`469f602`, crash
`pc 0x2cd6a0`). Também: cache da modelview (`5bcf387`), `Z_TJOIN` medido e
desligado, disco de 100% para 98% (agora de volta a 98% com 9,2 GB).

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| W1 | Achar a fonte de não-determinismo do desenho e só então refazer `estado_hash.ref` | M | 2 rodadas do mesmo `.mod`: 3088 quadros, hashes 1–3 iguais, 4º 0xae7f30f5 × 0x7f49ba75 (`.ref` de 17:38: 0x16ce3a75); `DYNRES ?= 0` em `Makefile:74` |
| W2 | 7 linhas no menu de debug (Hot Shelter, CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R) | P | 76 linhas, 73 estágios + 3 zeros; `src/progress/title.c:202` e `:384-460`; os 7 já estão em `PROGRAMS` (`game.mk:160-161`) |
| W3 | Rodar a matriz inteira (76 rows) com este `.mod`, quadro + log por linha | M | **7 de 76** com artefato; `summary.txt` de 23/11 20:18; `tools/stage_matrix.sh` faz 29 por vez |
| W4 | Rodar os atos 2+ das 9 fases de ação (29 linhas de 6–34) | M | rows 4–5, 7–8, 10–11, 13–14, 16–17, 19–20, 22–23, 25–27, 29–31, 33–34 nunca rodaram; só a row 3 tem hash |
| W5 | "Zero" (row 39, `STAGE_EGGMANROBO`): geometria errada, campo teal sem o robô | M | `c2eb03f3a477`, 777 quadros, `traps=0 ausente=0`; iluminação `simple` caindo no `draw_basic` |
| W6 | Twinkle Circuit (row 65, `STAGE_CART`): pista 95% preta e sequência de teclas | M | `52455310e0db`, 879 quadros; `DEBUG_TITLE=65` |
| W7 | Egg Carrier: row 46 (HUD sem cor, canto preto) e as 11 linhas 47–57 | M | `71b404f1d722`, 466 cores; 12 linhas, 1 medida |
| W8 | Chaos 4 (row 37) escuro/esverdeado; medir Chaos 2 (36) e Egg Viper 1 (38) | M | `70b135d59597`, 800 quadros; rows 36/38 sem artefato |
| W9 | Fechar Chaos 6/7 (`chaos6_event_form/melt` 4+4, `chaos7_*` 15 refs) e pôr linha nos 6 chefes | M | `build/zeebo/game/integrate.err`; rows de chefe existentes: 35–39 |
| W10 | Rosto dos Chao: `alife_face_set_eyes` (13) e `alife_face_set_mouth` (10) | M | alife 68 refs em `integrate.err`; `al_main`/`al_garden00-02` linkados |
| W11 | Resto do alife (45 refs: `garden_camera_step` 3, `_still` 1, `draw_palettes` 2, `chao_name_draw_at` 2) | M | mesmas rows 69–72; nenhuma medida |
| W12 | API de arquivo `bu*` (29 refs: `buFindExecFile` 8, `buLoadFileEx` 6, `buGetDiskFree` 6, `buRewriteExecFile` 5, `buSaveExecFile` 4) | M | `integrate.err`; afeta save/cartão e exec em runtime |
| W13 | `card_*` (12 refs, `card_menu_drives` 4) e save no campo com data do RTC | M | `integrate.err`; `CHECK_GAME=1` para em `ava_vmssel`; `sa1vmu.bin` |
| W14 | `tikal_player_step` (14 refs): Tikal jogável em Mystic Ruins/Past | M | 16 refs `tikal_*`; `tikal_prog` é overlay próprio (`game.mk:175`) |
| W15 | 65 refs a 33 `dat_0c…` (endereços absolutos do DC) | M | ex. `dat_0cac8a7c` em `config/adv00/symbols.txt:1340`; vêm de eventos/stages |
| W16 | Fazer o texto do aviso do cartão aparecer (`present_record` → GL) | P | patch ativo (`patched/src/draw/text.c:72,286`), probe não cai, tela sem texto |
| W17 | Stubs triviais: `strncmp` 5, `parked_car_object` 3, `fun_0c90d634` 2, `fun_0c9002ac` 2, `syCfgSetSoundMode` 1, `memmove` 1, `loader_buffer` 1 | P | `integrate.err`; 15 refs, nenhum exige engenharia reversa |
| W18 | Filmes em 448 linhas inteiras (`step=1`) | M | `ports/zeebo/zplm.c:146-153` (`while height/step > maxh: step++` → 224 de 448) |
| W19 | Áudio: 17/179 clipes sustentados, pitch bend e SFX 259 | M | `zaudio.c` (intro+laço por `MM_STATUS_DONE`); `src/sound/sound.c:757`; `todo.md:38` |
| W20 | Cortar ~6 MB do `.mod` e recuperar disco (9,2 GB livres, `keep/` 6,9 G) | M | `.mod` 37.834.444 B; 249 `.ovl` = 33.346.090 B; `df -h /` 98% |

Fora dos 20, ainda abertos: ms no aparelho (sem Zeebo em mão), o aviso de
apóstrofo em `engine/native/prelude.h:18` (cosmético, suja os 12.247 `.err`),
a row 40 sem artefato salvo, e a limpeza de `keep/`/`game_o2/` — que é
destrutiva e só acontece com autorização explícita.

## Atualização 2026-11-25 00:28 — as threads do jogo nunca recebiam a vez (diálogos de NPC)

Sintoma (Rafael): NPC de biquíni na frente do hotel não mostrava a caixa de diálogo e o
jogo ficava preso até um botão. Reprodução controlada: `message_show_timed` chamado de um
probe em Emerald Coast não mostrava nada. Instrumentação (removida): a thread da mensagem
era criada e preparada (`prepare`), mas `thread_yield` nunca a escolhia (`next == self` em
todo yield). Causa: `zthreads.c` definia `thread_next` e `thread_exit` com corpo vazio e
fracos; no `.mod` o jogo inteiro vira fraco (`--weaken`) e o primeiro fraco vence, e o
applet entra primeiro no link. O ELF tinha `thread_next` = `bx lr` em 0x417c.
Correção: `thread_next` e `thread_exit` viram só referências fracas, chamadas se existirem.
Resultado: `run_timed` entra, `bitmap_flush_queue` sobe a textura, e a faixa e o texto
aparecem no quadro (faixa escura de y 380 a 440, texto legível). Gates: 751/751, duplicatas 0,
`host-selftest` PASSED, `novfp` 0, `hash_ref` IGUAL, Emerald Coast 90 s sem fault.
Varredura da classe: nenhum outro símbolo fraco do applet colide com o jogo
(só `thread_current`, que é o mesmo símbolo dos dois lados).
Aberto: toda thread do jogo agora roda de verdade (mensagens, páginas, eventos); eventos e
NPCs que nunca funcionaram podem mostrar defeitos novos. Não validei NPCs nem cenas reais.

## Atualização 2026-11-25 01:55 — threads de ponta a ponta, Z-Pad remapeado; o gate do desenho mede tempo de parede

Rodada do cron (01:49–02:05). `HEAD` = `074ca8d` (01:43), branch `zeebo-port`.
Árvore **suja**: `ports/zeebo/zshinobi.c` modificado (`buIsReady` agora devolve 1 para o A1
e 0 para os outros — antes era o contrário, `zshinobi.c:360-366`), `ports/zeebo/tools/vmu_import.py`
novo (importa save de VMU real para `sa1vmu.bin`, CRC-16 conferido) e os de sempre
`sa1n.bmp`/`todo.md` não rastreados. **Seis commits** desde a nota das 00:28; nesta rodada
nada foi commitado além deste documento.

| Item | Medido agora (01:51–01:55) | Observação |
|---|---|---|
| `make` do applet | rc=0, **nada a recompilar** | `build/zeebo/sa1z.elf` 301.356 B, 01:41 |
| `make -f game.mk mod` | **751 de 751 objetos**, `duplicatas: 0`, **indefinidos: 259** (112 símbolos) | nenhum `FALHOU`; 0 `.err` com `error:` |
| `sa1g.mod` | **37.820.508 B** (36,05 MiB), 01:51 | 13.936 B menor que os 37.834.444 B de 23:42 |
| `.ovl` | **249** = **33.347.012 B** (31,80 MiB), região 2.242.340 B | +922 B em relação a 23:46 |
| Regiões `zovl_*` | modulo `0x00f94350`, field `0x011b7ab0`, extra `0x013b7010`, tikal `0x013f06f0`, **motion `0x01424a10`** (`0x99680`), evento `0x014be090`, fim `0x01532750` | todas **13.984 B (`0x36a0`) abaixo** das de 23:46: o `.mod` menor deslocou tudo |
| `host-selftest` | **PASSED (0 failures)** | |
| `novfp.sh` | **OK, 0 instruções VFP** | `build/zeebo/sa1g.elf` |
| `hash_ref.sh check` | **DIFERENTE** (rc=1) | 2 rodadas do mesmo `.mod` em 100 s: **3241 e 3088 quadros**; hashes 1–3 iguais, **4º diverge** (`0xe4064257`…) — causa abaixo |
| `PROGRAMS` | **251** = 47 programas + 204 eventos | `modules/` 41 dirs (fora `movie`); `MISSING_PROGRAMS` vazio |
| Menu de debug | **76** linhas: 73 estágios + 3 zeros | `src/progress/title.c:202` (`DEBUG_STAGES 76`), tabela `:384-460`; **0 das 7** linhas de W2 |
| Matriz | **7 de 76** com artefato (rows 3, 37, 39, 46, 65, 68, 71) | arquivos de 23/11 20:17–20:18; nada novo desta vez |
| Indefinidos por família | alife **68**, `dat_0c…` **65**, `bu*` **29**, `tikal_*` **16**, `chaos7_*` **15**, `card_*` **12**, `chaos6_*` **8**, `chaos0_*` **6**, outros **40** (`egg_viper_*` 8 dentro) | inalterado em relação a 23:46 |
| Disco | `/` a **99%**, **6,7 GB** livres | eram 9,2 GB às 23:46 (**−2,5 GB em 2 h**); `keep/` 7,1 G, `game/` 7,2 G |

#### O achado desta rodada: `hash_ref` compara quadros de tempo de parede

O `check` continua vermelho e agora se sabe por quê. Duas execuções do **mesmo** `.mod`
terminam com **3241 e 3088 quadros apresentados** em 100 s de parede (5% de diferença:
o runner é paceado por relógio, então quantos quadros cabem depende da carga do host).
Os hashes 1–3 (até 192 quadros) são idênticos e o 4º já diverge, exatamente onde a
fronteira dos 64 quadros cai em outro ponto da sequência. Não é (só) não-determinismo
do desenho: é a **contagem de quadros** que muda entre rodadas.

Prova do contrário quando o limite é por quadros: com `--frames=1024` e `--frames=2048`,
duas rodadas do mesmo `.mod` deram **388 quadros e 6 hashes idênticos** (as duas
execuções byte a byte iguais). Ou seja, o desenho é reprodutível; o gate é que mede
errado. Antes de refazer o `.ref` (de 17:38, 48 hashes, 3088 quadros) é preciso entender
por que o runner para em 388 quadros nesse modo — provavelmente o guest sai (fim do
laço do applet) — e então gerar o gate por `--frames`, não por `--seconds`.

#### O que foi feito desde a atualização das 00:28 (6 commits)

- `436a5f4` (00:58) — `njDrawPolygon3D`/`njDrawTriangle3D`: `njStartVertex*` devolvia 0 e o
  jogo escrevia em `0x18`. Agora devolve um rascunho de 64 KB; `njEndVertex` lê os registros
  `KMVERTEX` de 8 palavras (param, x, y, 1/z, u, v, cor, offset) e entrega ao strip, e
  `KM_VERTEXPARAM_ENDOFSTRIP` fecha o strip (`ports/zeebo/ninja/njkm_zeebo.c:87-150`).
- `ab790fc` (00:58) — `fatal()` das threads diz o motivo no `zlog` (antes girava mudo, o jogo
  parava sem fault nem log) e o `malloc` da pilha é checado (`zthreads.c:105-135`).
- `1dbeb93` (01:31) — botões do Z-Pad no losango do Dreamcast: **1=A, 4=B, 3=Y, 2=X**
  (antes 1=A, 2=B, 3=X, 4=Y); `zsys.c:129-143`.
- `a861b97` (01:40) — contextos de threads mortas voltam a ficar livres: o `heap_free` do jogo
  nunca chama `native_thread_freed`, então uma conversa de NPC deixava um contexto preso até
  esgotar os 32 e o jogo parar mudo; `reclaim()` varre os anéis do jogo (`zthreads.c:145-215`).
- `074ca8d` (01:43) — o botão 1 (`AVK_CONFIRM`, 0xe064) vale **só A**; o HOME do Z-Pad
  (Back+Start) vale o Start do Dreamcast (`zsys.c:75-83,129-140`).
- Gate com o `HEAD`: 751/751, 0 duplicatas, `host-selftest` PASSED, `novfp` 0.

#### O que falta para o OBJETIVO FINAL (port completo, todas as fases e conteúdo)

Mapa das 76 linhas do menu (`title.c:384-460`): **0–2** Practice, **3–5** Emerald Coast,
**6–8** Windy Valley, **9–11** Twinkle Park, **12–14** Speed Highway, **15–17** Red Mountain,
**18–20** Sky Deck, **21–23** Lost World (`STAGE_RUIN`), **24–27** Ice Cap, **28–31**
Casinopolis, **32–34** Final Egg, **35** Chaos 0, **36** Chaos 2, **37** Chaos 4, **38** Egg
Viper 1 (`EGGMOBILE1`), **39** Eggman robô ("Zero" nos docs), **40–45** Station Square,
**46–51** Egg Carrier, **52–57** Egg Carrier por dentro, **58–61** Mystic Ruins, **62–64**
Past, **65** Twinkle Circuit (`CART`), **66** Sky Chase 1, **67** Sky Chase 2, **68** Sand
Hill (`SUNABOARD`), **69–72** Chao Garden (SS/EG/MR/corrida), **73–75** zeros.

| Conteúdo | Estado em 01:55 | Falta |
|---|---|---|
| Emerald Coast (3–5) | row 3 medida em 23/11 (`575323d451b6`); atos 2–3 nunca | 2 linhas + re-medir a row 3 depois dos fixes de vértice/threads |
| Windy Valley (6–8), Twinkle Park (9–11), Speed Highway (12–14), Red Mountain (15–17), Sky Deck (18–20), Lost World (21–23), Ice Cap (24–27, 4 atos), Casinopolis (28–31, 4 atos), Final Egg (32–34) | 9 fases, 29 linhas, **nenhuma medida** | 29 linhas |
| Hot Shelter (`stg12`) | programa linkado (`game.mk:160`), **sem linha no menu** | linha + medida |
| Station Square (40–45) | row 40 rodou uma vez (24/11 13:2x) **sem artefato salvo** | 6 linhas com quadro + log |
| Egg Carrier (46–51) e por dentro (52–57) | row 46 escura, HUD sem cor (`71b404f1d722`) | 11 linhas |
| Mystic Ruins (58–61) e Past (62–64) | nunca medidas; `tikal_player_step` 14 refs | 7 linhas + Tikal jogável |
| Chefes | Chaos 0 (35) medido e otimizado; Chaos 2 (36) nunca; Chaos 4 (37) escuro (`70b135d59597`); Egg Viper 1 (38) nunca; "Zero" (39) geometria errada (`c2eb03f3a477`); Chaos 6/7, EGM2/3, E101, E101-R **sem linha** | medir 36 e 38, corrigir 37 e 39, e 6 programas sem linha |
| Chao Garden (69–72) | `al_main`+`al_garden00-02`+`al_race` linkados, **0 medidas** | 4 linhas; alife **68 refs** |
| Minijogos | Sky Chase 1/2 rodam; Sand Hill roda (row 68, `8f52bbe305a4`); Twinkle Circuit (65) 95% preto | pista do Circuit e a sequência de teclas da corrida |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1` | 448 linhas inteiras (`zplm.c:149` dobra o step e mostra 224) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes, pitch bend (`src/sound/sound.c:757`), SFX 259 |
| Save | cartão emulado + RTC; `buIsReady` corrigido **na árvore, sem commit** | save no campo nunca exercitado; `card_*` 12 refs; `CHECK_GAME=1` para em `ava_vmssel` |
| Texto/2D | passe 2D e depth do preview verificados; texto do aviso do cartão ainda não aparece | `present_record` desenhar o texto; `zstrip_tex_release_all` não limpa `tex_names[]` (`zstrip.c:1394`) |
| Desempenho | Station Square ~34–36 ms no zeebx; Chaos 0 quase sem quadro de 3 vsyncs | ms no Adreno 130 (sem aparelho). Gate por `--frames` antes de qualquer número |
| Pacote | `.mod` 36,05 MiB + 249 `.ovl` (31,80 MiB); disco **99%**, 6,7 GB | cortar ~6 MB; instalador; espaço |

#### 20 quick wins (W1–W20 novos, abertos em 25/11 01:55; P ≤ 1 h, M ≤ meio dia)

**Fechados desde a lista das 23:46, para não refazer:** os três defeitos de thread —
`thread_next/thread_exit` tapados pelo applet (`15f4b2a`), `fatal()` mudo e `malloc` da
pilha sem checagem (`ab790fc`), contextos de threads mortas presos (`a861b97`); o
`njDrawPolygon3D`/`njDrawTriangle3D` escrevendo em `0x18` (`436a5f4`); o mapeamento do
Z-Pad (1=A, 4=B, 3=Y, 2=X e HOME=Start, `1dbeb93`/`074ca8d`); e, das rodadas anteriores,
os 7 `*_mot` como overlay, o passe 2D + `zstrip_screen_depth` do preview, o freeze
pós-Chaos 0 e o texto sem store queue (`469f602`).

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| W1 | Gate por `--frames` no lugar de `--seconds=100`, entender a parada em 388 quadros e só então refazer o `.ref` | M | `hash_ref: DIFERENTE`; 3241 × 3088 quadros no mesmo `.mod`; 2×388 quadros idênticas; `.ref` de 17:38 com 48 hashes; `tools/hash_ref.sh:10` |
| W2 | 7 linhas no menu de debug (Hot Shelter, CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R) | P | 76 linhas, 0/7; `src/progress/title.c:202` e `:384-460`; os 7 já em `PROGRAMS` (`game.mk:160-161`) |
| W3 | Rodar a matriz inteira (76 rows) com este `.mod`, quadro + log por linha | M | **7 de 76** com artefato; `build/zeebo/matrix_r/` (23/11 20:17); `tools/stage_matrix.sh` faz 29 por vez |
| W4 | Atos 2+ das 9 fases de ação (29 linhas de 6–34) | M | rows 4–5, 7–8, 10–11, 13–14, 16–17, 19–20, 22–23, 25–27, 29–31, 33–34 nunca rodaram |
| W5 | "Zero" (row 39, `STAGE_EGGMANROBO`): geometria errada, campo teal sem o robô | M | `c2eb03f3a477`, 777 quadros, `traps=0 ausente=0`; iluminação `simple` caindo no `draw_basic` |
| W6 | Twinkle Circuit (row 65, `STAGE_CART`): pista 95% preta e sequência de teclas | M | `52455310e0db`, 879 quadros; `DEBUG_TITLE=65` |
| W7 | Egg Carrier: row 46 (HUD sem cor, canto preto) e as 11 linhas 47–57 | M | `71b404f1d722`, 466 cores; 12 linhas, 1 medida |
| W8 | Chefes: medir Chaos 2 (36) e Egg Viper (38), corrigir Chaos 4 (37) escuro/esverdeado | M | `70b135d59597`, 800 quadros; rows 36/38 sem artefato |
| W9 | Fechar Chaos 6/7 (`chaos6_event_form/melt` 4+4, `chaos7_*` 15 refs) e pôr linha nos 6 chefes | M | `build/zeebo/game/integrate.err`; rows de chefe existentes: 35–39 |
| W10 | Chao: rostos (`alife_face_set_eyes` 13, `alife_face_set_mouth` 10) e o resto do alife (`garden_camera_step` 3, `_still` 1, `draw_palettes` 2) | M | alife 68 refs em `integrate.err`; `al_main`/`al_garden00-02` linkados |
| W11 | API de arquivo `bu*` (29 refs: `buFindExecFile` 8, `buLoadFileEx` 6, `buGetDiskFree` 6, `buRewriteExecFile` 5, `buSaveExecFile` 4) | M | `integrate.err`; afeta save/cartão e exec em runtime |
| W12 | Cartão: commitar o `buIsReady` da árvore (`zshinobi.c:360-366`) e o `tools/vmu_import.py`, e fechar `card_*` (12 refs, `card_menu_drives` 4) até o save no campo com RTC | M | diff na árvore; `CHECK_GAME=1` para em `ava_vmssel`; `sa1vmu.bin` |
| W13 | `tikal_player_step` (14 refs): Tikal jogável em Mystic Ruins/Past | M | 16 refs `tikal_*`; `tikal_prog` é overlay próprio (`game.mk:175`) |
| W14 | 65 refs a 33 `dat_0c…` (endereços absolutos do DC) | M | ex. `dat_0cac8a7c` em `config/adv00/symbols.txt:1340`; vêm de eventos/stages |
| W15 | Texto do aviso do cartão via `present_record` e limpeza de `tex_names[]` no `zstrip_tex_release_all` | P | patch ativo (`patched/src/draw/text.c:72,286`), tela sem texto; `zstrip.c:1394`; bug do pause em `todo.md` |
| W16 | Stubs triviais: `strncmp` 5, `memmove` 1, `parked_car_object` 3, `fun_0c90d634`/`fun_0c9002ac` 2 cada, `syCfgSetSoundMode` 1, `loader_buffer` 1, `cache_ram_init`/`cache_ram_restore` 2, `advertise_module_end` 1 | P | `integrate.err`; 17 refs, nenhum exige engenharia reversa |
| W17 | Filmes em 448 linhas inteiras (`step=1`) | M | `ports/zeebo/zplm.c:146-153` (`while height/step > maxh: step++` → 224 de 448) |
| W18 | Áudio: 17/179 clipes sustentados, pitch bend e SFX 259 | M | `zaudio.c` (intro+laço por `MM_STATUS_DONE`); `src/sound/sound.c:757`; `todo.md:38` |
| W19 | Cortar ~6 MB do `.mod` e recuperar disco (99%, 6,7 GB; `keep/` 7,1 G + `game/` 7,2 G) | M | `.mod` 37.820.508 B; 249 `.ovl` = 33.347.012 B; limpeza de `keep/` é destrutiva e exige autorização |
| W20 | Regressão de NPCs e cenas com as threads rodando (o fix mudou o fluxo de quem nunca rodava) | M | `15f4b2a`+`a861b97`; NPC do hotel em Station Square era o sintoma original; nada validado depois |

Fora dos 20, ainda abertos: ms no aparelho (sem Zeebo em mão), o aviso de apóstrofo em
`engine/native/prelude.h:18` (cosmético, suja os `.err`), a row 40 sem artefato salvo, e a
limpeza de `keep/`/`game_o2/` — destrutiva, só com autorização explícita.

## Atualização 2026-11-25 (madrugada) — `.mod` 37,8 → 30,8 MB: código dos programas em overlay

Pedido do Rafael: levar o `.mod` de 36 MB para 16–24 MB. Medida de onde estavam os bytes (ELF de
`sa1g`): `.text` 10,26 MB (8,1 MB deles são o código dos 251 programas de fase/chefe/evento
linkados de uma vez), `.rodata` 0,97, `.data` 5,10, `.bss` 15,59 (o elf2mod grava o bss como zeros;
`zdc_ram` sozinho são 7,34), mais ~5,8 MB das regiões de overlay de dados. O zeebx mapeia o arquivo
inteiro (mais 1 MB), então tudo que está no ELF tem de estar no arquivo.

| passo | `.mod` | commit |
|---|---:|---|
| ponto de partida | 37.821.596 | `15f4b2a`… |
| `--gc-sections` só no código (`GC=0` desliga) | 37.380.252 (-0,44 MB) | `016a38e` |
| código dos programas em overlay (`.ovlc_<P>`), 249 `.ovl` agora 36,8 MB no disco | 30.772.188 (-6,6 MB) | este bloco |

- **GC de dados não serve.** O primeiro teste (`--gc-sections` inteiro, -1,8 MB) parou o jogo em `TRAP READ`
  no boot: o decomp conta com a vizinhança de variáveis (o read de `light_read_disc_sources` passa do fim
  de `light_sources` e entra nos membros seguintes de `sources_file`). `gen_ovl.py` põe `KEEP` em
  `.rodata`/`.data`/`.bss`/`COMMON`; só as seções de função saem.
- **Overlay de código.** `tools/program.ld` junta o `.text` de cada programa; `game.mk` renomeia para
  `.ovlc_<P>`; `gen_ovl.py` põe uma região de código por grupo (`zovlc_<grupo>`, NOLOAD, mesmo
  endereço para todos do grupo) e grava no `.ovl` (magia `OVL2`) o código em PRS com a lista de
  relocação; `zovl_load_code` (`zgdfs.c`) enche a região ao abrir o programa. O zeebx invalida o JIT
  nas escritas do guest (`mark_code_dirty`), então trocar o código da região funciona.
- **Três armadilhas, todas do elf2mod/da separação RO×RW** (o `.mod` tem `ro-base` e `rw-base` e o RW
  sobe 8 bytes a mais que o RO no zeebx): (1) um `bl` do `.mod` (RO) para a região (RW) o elf2mod recusa
  (`Unsupported relocation type 28`): `tools/gen_ovlstubs.py` renomeia cada função global do programa
  para `F__ovlf` e gera no `.mod` um stub `F: ldr pc,[pc,#-4]; .word F__ovlf` (literal ABS32; 32 chamadas
  assim); (2) um `bl` da região (RW) para o `.mod` (RO), com o deslocamento relativo calculado no link,
  errava por esses 8 bytes (o jogo caía com `pc=0`): o extrator grava essas chamadas com o bit 30 e o
  loader soma `(delta_ro - delta_rw)/4` ao campo de 24 bits; (3) literais `ABS32` do código entram na
  mesma lista de relocação dos dados, por classe RO/RW.
- **Verificado:** linhas 3, 6, 9, 12, 15, 18 e 21 do menu de debug (via L+R+Cima) entram em modo 4 com 0
  faults, e a 15 carrega também o código do evento. As demais linhas (24–66) e os gates estão no
  commit.
- **Não verificado, e é o risco real:** no aparelho o ARM11 tem cache de instruções; escrever código numa
  região e executá-lo precisa de uma invalidação que o BREW não oferece que eu tenha achado (a nota de
  `overlay-and-mod-size.md` já alertava). No zeebx funciona porque o JIT acompanha as escritas. Quem
  testar no Zeebo de verdade deve contar com isso. Quando houver como invalidar, é uma chamada em
  `zovl_load_code`.
- **Falta para chegar a 16–24 MB** (o `.mod` agora tem ro 3,5 + rw 5,1 + zi 21,8): `zdc_ram` (7,34 MB,
  ainda por medir quanto é de fato usado), os ~8 MB de bss em buffers de 100–600 KB (`clip_wav`,
  `dialog_pixels` ×2, `polygon_pool`, `card_work`…) e as regiões de dados (5,8 MB). Mover bss para
  `malloc` encolhe o arquivo, não a RAM.

### Medida do `zdc_ram` (2026-11-25, madrugada): 4,5–4,7 MB tocados dos 7,34 MB

Sonda temporária (revertida) em `zthreads.c`: a cada 120 quadros marca as páginas de 4 KB de `zdc_ram`
que alguma vez ficaram diferentes de zero. Em Emerald Coast (row 3) aos 35 s: 4.528 KB; em Station
Square (row 40): 4.676 KB. O maior bloco contínuo começa no endereço-base e vai até ~1,9–2,2 MB
(a área do programa, `MODULE_AREA` 0x0c900000, onde o decomp lê o binário SH-4 de cada programa; o
jogo roda do overlay ARM, então essa leitura provavelmente é dispensável — **não testado**), o resto
são pedaços de 4 KB a 5 MB espalhados até uma página perto do fim (0x0cfff000). Zero escrito não conta
como tocado, então é um piso. Compactar o mapa (`ZDC` por tabela de áreas, constante em
inicializadores) economizaria no máximo ~2,7 MB; pular a leitura do binário SH-4 pode render ~2 MB
sozinho e é a primeira coisa a testar. Com os dois, o `.mod` iria a ~26–27 MB; chegar a 24 MB exige
também mexer nos ~8 MB de bss de buffers.

## Atualização 2026-11-25 04:20 — A não confirma o menu de debug; a fase entra por b2+b3, rows 3 e 40 medidas

Rodada do cron (03:55–04:20). `HEAD` = `332d732` (03:13), branch `zeebo-port`, árvore limpa (só
`sa1n.bmp` e `todo.md` não rastreados, os de sempre). Sete commits desde a atualização das 01:55;
nesta rodada não se commitou nenhum código, só este documento.

| Item | Medido agora (03:58–04:20) | Observação |
|---|---|---|
| `make -C ports/zeebo` | rc=0, **nada a recompilar**; `sa1z.elf` 302.948 B, 03:13 | applet feito antes do `mod` (zstrip/ztexture/app.c) |
| `make -f game.mk mod` | **752 de 752 objetos**, duplicatas 0, **indefinidos 19** (14 símbolos) | nenhum `FALHOU`; 0 `.err` com `error:`; +1 objeto = `zovlstubs.o` |
| `sa1g.mod` | **30.770.572 B** (29,35 MiB), 04:20 | era 37.820.508 B (36,05 MiB) às 01:51 |
| `.ovl` | **249** = **36.853.845 B** (35,15 MiB) no disco | o `mod` imprime “gen_ovl: 249 overlays … regiao 2242340 B” |
| Regiões `zovl_*` | `modulo 0x00838bd0` (`0x223760`), `field 0x00aa36f0`, `extra 0x00cb7a10`, `tikal 0x00d1dba0`, `motion 0x00d546e0` (`0x99680`), `evento 0x00dedd60`, `fim 0x00e794c0` | dados 5.891.072 B em 6 grupos; código `zovlc_*` 664.816 B |
| `host-selftest` | **PASSED (0 failures)** | |
| `novfp.sh` | **OK, 0 instruções VFP** | `build/zeebo/sa1g.elf` |
| `PROGRAMS` | **251** = 47 programas + 204 eventos; `MISSING_PROGRAMS` vazio | `modules/` tem 41 dirs (fora `movie`); **não existe `stg11`** no decomp (stg00–stg10, stg12) |
| Indefinidos reais dos overlays | **114 símbolos, 220 referências** (`nm -u build/zeebo/game/ovl/program-*.o` menos o que `sa1g.elf` define) | o `integrate.err` mostra só 19: `gen_ovl.py:195` descarta indefinido e `game.mk:424` liga com `--unresolved-symbols=ignore-all` |
| Matriz | **7 de 76** de 23/11 (rows 3, 37, 39, 46, 65, 68, 71) + **rows 3 e 40 medidas hoje** | rows 4–34, 36–39, 41–72 nunca; 0 das 7 linhas de V3 |
| `ms jogo/quadro` | row 3: **20–21 ms**; row 40: **14–15 ms** | era 80–83 ms na row 3 (23/11) e 34–36 ms na row 40 |
| Disco | `/` a **99%**, **5,0 GB** livres | `keep/` 7,1 G + `game/` 7,2 G |

#### O achado desta rodada: A não confirma o menu de debug — e o atalho que entra na fase

O caminho que o `tools/stage_matrix.sh` usa (`DEBUG_TITLE=N` + `14000:b1,17000:b1`) **não entra mais na
fase**: a row 3 rodou 45 s em `game_mode 0x2` (title) com 1438 quadros e hash `5d38968c25a3`; a row 21
também ficou no menu, com os mesmos 1438 quadros e `c8d000b7037c` (o destaque muda o desenho) — o menu
desenha (as duas listas, “STAGE SELECT” e os nomes) e não sai dele.
Medido com o mesmo `.mod`, 20 s, três vezes:

| teclas | quadros | hash do último quadro |
|---|---|---|
| nenhuma | 688 | `5d38968c25a3` |
| `8000:b1,12000:b1,16000:b1` | 688 | `5d38968c25a3` (**idêntico**: A não fez nada) |
| `8000:down,12000:down,16000:up` | 688 | `74c7593577b6` (o d-pad mexe na seleção) |

No boot normal o mesmo: `10000:b1,14000:b1` e nenhuma tecla dão 701 quadros e `b8600610ed3b`, com
`movie sah448_6.sfd` no log nas duas — o A também não pula o filme (`ports/zeebo/zmovie.c:197` testa
`per_info[0]->press`). **O caminho que funciona é segurar b2+b3** (`PDD_DGT_TX|PDD_DGT_TY`, que no menu
de debug é `title_mode = TITLE_QUIT`, `title.c:1260-1263`): com `6000:b2,8000:b3` a row 3 entra em
`game_mode 0x4` (ACTION) e a row 40 em `0x5` (FIELD).

- Row 3 (Emerald Coast, `DEBUG_TITLE=3`), 60 s: modo 4, **1829 quadros**, último `8031d98b7314`,
  ms jogo/quadro 97 no primeiro quadro e 20–21 depois; `traps=1`, só o stub `cache_ram_init` no boot,
  `ausente=0`; quadro com céu azul no topo e areia clara, 99,7% dos pixels não-pretos, 255 níveis
  (`build/zeebo/matrix_r/row3_2026-11-25.png`).
- Row 40 (Station Square, `DEBUG_TITLE=40`), 60 s: modo 5, **1840 quadros**, último `9f1f67d18d0e`,
  14–15 ms; cenário completo (99,9% não-preto, 236 níveis; `build/zeebo/matrix_r/row40_2026-11-25.png`).

Por que o A se perde, hipótese não confirmada: `pad.press` é calculado dentro de
`pdExecPeripheralServer` (`ports/zeebo/zsys.c:203`, `pad.press = now & ~pad.old`) e essa função é
chamada 2–3 vezes por quadro (`src/system/frame.c:144` e `:155`, `ports/zeebo/zninjasys.c:79`); a borda
que a primeira chamada produz é zerada pela seguinte, e o `pad_scan` do jogo lê o registro depois. Não
provei que o `press` morre no jogo todo: numa fase, três A (20/25/30 s) mudaram o dump 800
(`1c1165fd24b2` sem × `c856714e6ee7` com), mas o b4 deu o mesmo hash do b1 e o b2 outro, e as rodadas
apresentam 1077–1079 quadros, então a comparação por número de dump não é controlada. Instrumentar
`pad.press` é a V1; não mexi em código nesta rodada (só docs).

#### O que foi feito desde a atualização das 01:55 (7 commits, `8b742a4`..`332d732`)

- `947a5d0` (02:13) — menu de debug escondido: **L+R+Cima por 3 s** reinicia no menu de fases
  (`zsys.c:123-155`, `src/main.c:184-190`); o `game.mk` passou a refazer as cópias remendadas quando o
  fonte muda. **Não testado hoje** (L/R só existem no IHID; com o A morto o menu não confirma).
- `45a45a8` (02:20) — o pareamento do Z-Pad não vale para o Boomerang (`zboom` entrega A/B/X/Y no
  sentido do Dreamcast).
- `8189f9b` (02:21) — `buIsReady` estava invertido e escondia o cartão A1 na seleção; `tools/vmu_import.py`
  importa o save do VMU real (CRC-16 conferido).
- `37c1974` (02:26) — `zstrip_tex_release_all` zera `tex_names[]`; o `zhid` limita eventos por leitura e
  solta o controle que some (suspeito natural da perda do `press`, ver acima).
- `016a38e` (02:41) — `--gc-sections` só no código (`GC=0` desliga): `.mod` 37.821.596 → 37.380.252 B.
  GC de dados para o jogo no boot (`TRAP READ`): o decomp conta com a vizinhança das variáveis.
- `c3ed5f6` (03:09) — código dos programas em overlay (`.ovlc_<P>`, região `zovlc_<grupo>`, magia
  `OVL2`), stubs no `.mod` e correção do `bl` RW→RO no loader: `.mod` 37,4 → **30,8 MB**.
- `332d732` (03:13) — medida do `zdc_ram` (4,5–4,7 MB tocados de 7,34) e o caminho para 24 MB.

#### O que falta para o OBJETIVO FINAL (port completo, todas as fases e conteúdo)

Mapa do menu (`src/progress/title.c:384-460`, 76 linhas): 0–2 Practice, 3–5 Emerald Coast, 6–8 Windy
Valley, 9–11 Twinkle Park, 12–14 Speed Highway, 15–17 Red Mountain, 18–20 Sky Deck, 21–23 Lost World,
24–27 Ice Cap, 28–31 Casinopolis, 32–34 Final Egg, 35 Chaos 0, 36 Chaos 2, 37 Chaos 4, 38 Egg Viper 1,
39 “Zero”, 40–45 Station Square, 46–51 Egg Carrier, 52–57 Egg Carrier por dentro, 58–61 Mystic Ruins,
62–64 Past, 65 Twinkle Circuit, 66–67 Sky Chase, 68 Sand Hill, 69–72 Chao Garden, 73–75 zeros.

| Conteúdo | Estado em 04:20 | Falta |
|---|---|---|
| Emerald Coast (3–5) | row 3 medida hoje (`8031d98b7314`, modo 4, 20–21 ms); rows 4–5 nunca | 2 linhas |
| Windy Valley (6–8), Twinkle Park (9–11), Speed Highway (12–14), Red Mountain (15–17), Sky Deck (18–20), Lost World (21–23), Ice Cap (24–27), Casinopolis (28–31), Final Egg (32–34) | 29 linhas, **nenhuma medida com artefato** (o commit do overlay diz ter entrado em modo 4 nas rows 3, 6, 9, 12, 15, 18, 21, sem quadro guardado) | 29 linhas |
| Hot Shelter (`stg12`) | linkado (`game.mk:165`, `src/stage/stage.c:97`), **sem linha no menu** | linha + medida |
| Station Square (40–45) | row 40 medida hoje (`9f1f67d18d0e`, modo 5, 14–15 ms, cenário completo) | 5 linhas (41–45) |
| Egg Carrier (46–51) e por dentro (52–57) | row 46 escura, HUD sem cor (`71b404f1d722`, 23/11) | 11 linhas |
| Mystic Ruins (58–61) e Past (62–64) | nunca medidas; `tikal_*` 31 refs | 7 linhas + Tikal jogável |
| Chefes | Chaos 0 (35) medido e otimizado; Chaos 2 (36) nunca; Chaos 4 (37) escuro (`70b135d59597`); Egg Viper 1 (38) nunca; “Zero” (39) geometria errada (`c2eb03f3a477`); Chaos 6/7, EGM2/3, E101, E101-R **sem linha** e com 8+15+7 refs | medir 36/38, corrigir 37/39, 6 programas sem linha |
| Chao Garden (69–72) | `al_main`+`al_garden00-02`+`al_race` linkados, **0 medidas** | 4 linhas; alife 32 refs + garden 25 |
| Minijogos | Sky Chase 1/2 e Sand Hill rodam (row 68, `8f52bbe305a4`); Twinkle Circuit (65) 95% preto | pista do Circuit |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1` | 448 linhas (`zplm.c:146-153` dobra o step) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes, pitch bend (`src/sound/sound.c:757`), SFX 259 |
| Save | cartão emulado + RTC; `buIsReady` corrigido (`8189f9b`); `vmu_import.py` | save no campo nunca exercitado; `card_*`/`chao_*` 11 refs; `CHECK_GAME=1` para em `ava_vmssel` |
| Texto/2D | `zstrip_tex_release_all` agora zera `tex_names[]` (`37c1974`); 2D e depth do preview verificados | texto do aviso do cartão ainda não aparece (`present_record`) |
| Desempenho | 14–21 ms/quadro de jogo no zeebx (rows 3 e 40) | ms no Adreno 130 (sem aparelho); gate por `--frames` antes de qualquer número |
| Pacote | `.mod` 29,35 MiB + 249 `.ovl` (35,15 MiB); disco 99%, 5,0 GB | cortar para 26–27 MiB (V7/V8); instalador |

#### 20 quick wins (V1–V20, abertos em 25/11 04:20; P ≤ 1 h, M ≤ meio dia)

**Fechados desde a lista W1–W20 das 01:55, para não refazer:** **W19** (cortar ~6 MB) —
`016a38e`+`c3ed5f6` levaram o `.mod` de 37,8 para 30,77 MB; **W15** na metade — o `tex_names[]` foi
corrigido (`37c1974`), falta o texto do aviso do cartão; **W12** na metade — `buIsReady` e o
`vmu_import.py` entraram (`8189f9b`), falta o save no campo; **W16** continua aberto, agora com a lista
exata dos 19 indefinidos (V15). Os demais W1–W14, W17, W18 e W20 seguem abertos e viraram V2, V5, V3,
V4, V6, V10, V11, V12, V13, V14, V16, V18, V19 e V20.

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| V1 | Medir e corrigir o caminho do `press`: pôr um contador em `pad_scan` (sonda revertida) e ver se a borda chega; suspeitos `37c1974` e as 3 chamadas de `pdExecPeripheralServer` | M | hashes iguais com/sem A (688 quadros `5d38968c25a3`; 701 `b8600610ed3b`); d-pad muda (`74c7593577b6`); `zsys.c:203`; `frame.c:144,155`; `zninjasys.c:79` |
| V2 | Pôr **b2+b3** como entrada da matriz (`tools/stage_matrix.sh`, default `14000:b1,17000:b1`) e documentar no `menu-debug.md` | P | rows 3 e 40 mediram hoje com `6000:b2,8000:b3`; com b1 ficam em `game_mode 0x2` |
| V3 | 7 linhas no menu de debug: Hot Shelter (`stg12`), CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R | P | `title.c:202` (`DEBUG_STAGES 76`) e `:384-460`; 0/7; os 7 já em `PROGRAMS` (`game.mk:165-166`) |
| V4 | Rodar a matriz inteira (76 linhas) com o caminho da V2, quadro + log por linha | M | **7 de 76** com artefato, todos de 23/11; `build/zeebo/matrix_r/`; `stage_matrix.sh` faz 29 por vez |
| V5 | Gate de desenho por `--frames` no lugar de `--seconds=100`, e só então refazer o `.ref` | M | `tools/hash_ref.sh:10`; 3241 × 3088 quadros no mesmo `.mod`; 2×388 idênticas com `--frames` |
| V6 | Atos 2+ das 9 fases de ação: 29 linhas de 6–34 | M | rows 4–5, 7–8, 10–11, 13–14, 16–17, 19–20, 22–23, 25–27, 29–31, 33–34 nunca rodaram |
| V7 | `zdc_ram`: parar de ler o binário SH-4 na `MODULE_AREA` (~2 MB; o jogo roda do overlay ARM) | M | 4,5–4,7 MB tocados de 7,34; maior bloco 1,9–2,2 MB desde a base; nota de 03:13 |
| V8 | `.mod` de 29,35 para 26–27 MiB: mapa ZDC por áreas (≤2,7 MB) + regiões de dados de 5,89 MB | M | `.mod` 30.770.572 B; `zovl_modulo` 0x223760; `elf2mod` ro/rw/zi |
| V9 | Tirar os ~8 MB de bss de buffers (`clip_wav`, `dialog_pixels` ×2, `polygon_pool`, `card_work`) para `malloc` | M | nota de 03:13; `.mod` zi 21,8 MB; mover bss não muda a RAM, só o arquivo |
| V10 | Chefes: medir Chaos 2 (36) e Egg Viper (38), corrigir Chaos 4 (37) escuro/esverdeado | M | `70b135d59597`, 800 quadros; rows 36/38 sem artefato |
| V11 | Chaos 6/7 nos overlays e linha no menu para os 6 chefes que não têm | M | `chaos6_event_form`/`_melt` 4+4; `chaos7_*` 15; rows de chefe: 35–39 |
| V12 | Chao: alife e o resto do garden | M | 32 refs alife (`alife_create` 3, `alife_chao_create_garden` 3) + 25 `garden_*`; rows 69–72 sem medida |
| V13 | Tikal jogável em Mystic Ruins/Past | M | `tikal_object` 15, `tikal_player_step` 14, `tikal_walk_action` 2; `tikal_prog` é overlay próprio (`game.mk:180`) |
| V14 | 66 refs a `dat_0c…` (endereços absolutos do DC) | M | ex. `dat_0cb2d478` 4, `dat_0cac8a7c` 4, `dat_0cb18618` 4; `config/adv00/symbols.txt:1340` |
| V15 | 19 indefinidos do `.mod`: `strncmp` 5, `draw_palettes` 2, `memmove`, `loader_buffer`, `cache_ram_init`, `cache_ram_restore`, `syCfgSetSoundMode`, `advertise_module_end`, `eggmanrobo_land`, `card_drive_object`, 3 `dat_0c…` | P | `build/zeebo/game/integrate.err`; nenhum exige engenharia reversa |
| V16 | Cartão e save no campo: `card_*`/`chao_*` 11 refs (`card_menu_drives`, `card_write_chao`, `card_read_chao`…) | M | `integrate.err`/nm; `CHECK_GAME=1` para em `ava_vmssel`; RTC já real |
| V17 | 7 refs `egg_viper_event_*` (chefe 38, `b_egm1`) e 7 `warp_*` (estações de teleporte) | P | refs medidas nos overlays; `b_egm1` linkado (`game.mk:166`) |
| V18 | Filmes em 448 linhas inteiras (`step=1`) | M | `ports/zeebo/zplm.c:146-153`: `while height/step > maxh: step++` → 224 de 448 |
| V19 | Áudio: 17/179 clipes sustentados, pitch bend e SFX 259 | M | `zaudio.c` (intro+laço por `MM_STATUS_DONE`); `src/sound/sound.c:757`; `todo.md:38` |
| V20 | Twinkle Circuit (row 65) e regressão de Sand Hill/Sky Chase com este `.mod` | M | `52455310e0db`, 879 quadros, pista 95% preta; row 68 `8f52bbe305a4`; rows 66/67 nunca re-medidas |

Fora dos 20, ainda abertos: ms no aparelho (sem Zeebo em mão), o aviso de apóstrofo em
`engine/native/prelude.h:18` (cosmético, suja os `.err`) e a limpeza de `keep/`/`game_o2/` —
destrutiva, só com autorização explícita.

## Atualização 2026-11-25 06:35 — Windy Valley, Twinkle Park e Speed Highway medidos; nenhum commit em 2 h e o caminho padrão do zeebx no `frame_hash.sh` não existe

Rodada do cron (06:21–06:40). `HEAD` = `caa3ecc` (04:20), branch `zeebo-port`, árvore **limpa** (só
`sa1n.bmp` e `todo.md` não rastreados, os de sempre). **Zero commits desde a atualização das 04:20** —
nada de código entrou em 2 h10; esta rodada commitou só este documento e mediu três linhas novas.

| Item | Medido agora (06:22–06:35) | Observação |
|---|---|---|
| `make -C ports/zeebo` | rc=0, **nada a recompilar**; `sa1z.elf` 302.948 B, 03:13 | applet feito antes do `mod`, como manda a armadilha do zstrip/ztexture/app.c |
| `make -f game.mk mod` | **752 de 752 objetos**, `duplicatas: 0`, **indefinidos: 19** | nenhum `FALHOU`; 4 `.err` reescritos (title/main e dependentes), **0** com `error:` |
| `sa1g.mod` | **30.770.572 B** (29,35 MiB), 06:27 | **byte-idêntico** ao de 04:20: link determinístico, regiões `zovl_*` as mesmas |
| `.ovl` | **249** = **36.853.845 B** (35,15 MiB) | `gen_ovl: 249 overlays, regiao 2242340 B` |
| `host-selftest` | **PASSED (0 failures)** | |
| `novfp.sh` | **OK, 0 instruções VFP** | `build/zeebo/sa1g.elf` (27.597.404 B, 06:28) |
| `PROGRAMS` | **251** = 47 programas + 204 eventos; `MISSING_PROGRAMS` vazio | `config/ev*/` tem 204 dirs, todos em `PROGRAMS` |
| Indefinidos dos overlays | **114 símbolos** (`nm -u ovl/program-*.o` menos o que `sa1g.elf`/os próprios overlays definem) | famílias: `dat_0c…` 34, `alife_*` 27, `garden_*` 9, `card_*` 9, `egg*` 7, `chaos7_*` 5, `warp_*` 4, `tikal_*` 3, `chaos0/2_*` 3 cada, `chaos6_*` 2 |
| Matriz | **11 de 76** linhas com artefato: 3, **6, 9, 12**, 37, 39, 40, 46, 65, 68, 71 | as três novas são desta rodada; `build/zeebo/matrix_r/` (5 PNGs de 25/11) |
| Disco | `/` a **99%**, **4,7 GB** livres | era 5,0 GB às 04:20; `keep/` 7,2 G + `game/` 7,8 G |

#### O achado desta rodada: o caminho padrão do zeebx no `frame_hash.sh` morreu — a matriz “falha” em toda linha

`tools/frame_hash.sh:15` aponta por default para `~/projects/zeebx-v060/target/release/zeebx`, e esse
diretório **não existe mais** (o único binário na máquina é
`~/projects/zeebx-emu-copytex/target/release/zeebx`, de 24/11 16:56; `zeebx-emu/target/` nem existe). O
sintoma engana: a primeira tentativa de rodar a row 6 devolveu `FALHOU: sem quadro (rc=127)` — parece
falha do jogo, é path de binário. `tools/hash_ref.sh:9` já usa o copytex; só o `frame_hash.sh` ficou
para trás, então `stage_matrix.sh` (que chama `frame_hash.sh`) não roda como documentado. Com
`ZEEBX=~/projects/zeebx-emu-copytex/target/release/zeebx` tudo funciona — as três linhas abaixo foram
medidas assim.

#### Três linhas novas, todas pelo caminho `6000:b2,8000:b3` (45 s)

| row | fase | modo | quadros | último hash | ms jogo/quadro | quadro |
|---:|---|---|---:|---|---|---|
| 6 | Windy Valley act 1 | `0x4` ACTION | **1382** | `3be948f582a6` | 23–24 (1º de 80) | 98,6% não-preto, 640×480 |
| 9 | Twinkle Park act 1 | `0x4` ACTION | **1020** | `09e03f1bc6e2` | 34–36 | 97,5% não-preto |
| 12 | Speed Highway act 1 | `0x4` ACTION | **1388** | `3a0ac207e776` | ~20 | 95,8% não-preto |

`traps=0`, `ausente=0` nas três. As teclas são seguradas de 6 s a 8 s e o modo ACTION entrou em
6,55 s na row 6 — o `TITLE_QUIT` dispara na borda do b2+b3, não precisa manter. A row 6 rodou duas
vezes com hash **idêntico** (`3be948f582a6`), reforçando que o desenho é reprodutível e o problema do
gate é só a contagem por tempo de parede (X1).

PNGs: `build/zeebo/matrix_r/row{6,9,12}_2026-11-25.png`.

#### O que falta para o OBJETIVO FINAL (port completo, todas as fases e conteúdo)

Mapa do menu (`src/progress/title.c:384-460`, 76 linhas): 0–2 Practice, 3–5 Emerald Coast, 6–8 Windy
Valley, 9–11 Twinkle Park, 12–14 Speed Highway, 15–17 Red Mountain, 18–20 Sky Deck, 21–23 Lost World,
24–27 Ice Cap, 28–31 Casinopolis, 32–34 Final Egg, 35 Chaos 0, 36 Chaos 2, 37 Chaos 4, 38 Egg Viper 1,
39 “Zero”, 40–45 Station Square, 46–51 Egg Carrier, 52–57 Egg Carrier por dentro, 58–61 Mystic Ruins,
62–64 Past, 65 Twinkle Circuit, 66–67 Sky Chase, 68 Sand Hill, 69–72 Chao Garden, 73–75 zeros.

| Conteúdo | Estado em 06:35 | Falta |
|---|---|---|
| Emerald Coast (3–5) | row 3 medida (`8031d98b7314`, modo 4, 20–21 ms) | rows 4–5 |
| Windy Valley (6–8) | **row 6 medida agora** (`3be948f582a6`, modo 4, 23–24 ms, 98,6% não-preto) | rows 7–8 |
| Twinkle Park (9–11) | **row 9 medida agora** (`09e03f1bc6e2`, modo 4, 34–36 ms) | rows 10–11 |
| Speed Highway (12–14) | **row 12 medida agora** (`3a0ac207e776`, modo 4, ~20 ms) | rows 13–14 |
| Red Mountain (15–17), Sky Deck (18–20), Lost World (21–23), Ice Cap (24–27), Casinopolis (28–31), Final Egg (32–34) | 20 linhas, **nenhuma com artefato** | 20 linhas |
| Hot Shelter (`stg12`) | linkado (`game.mk:165`, `src/stage/stage.c:97`), **sem linha no menu** | linha + medida |
| Station Square (40–45) | row 40 medida (04:20: `9f1f67d18d0e`, modo 5, 14–15 ms) | 5 linhas (41–45) |
| Egg Carrier (46–51) e por dentro (52–57) | row 46 escura, HUD sem cor (`71b404f1d722`, 23/11) | 11 linhas |
| Mystic Ruins (58–61) e Past (62–64) | nunca medidas; `tikal_*` 3 símbolos / 14 refs de `tikal_player_step` | 7 linhas + Tikal jogável |
| Chefes | Chaos 0 (35) medido e otimizado; Chaos 2 (36) nunca; Chaos 4 (37) escuro (`70b135d59597`); Egg Viper 1 (38) nunca; “Zero” (39) geometria errada (`c2eb03f3a477`); Chaos 6/7, EGM2/3, E101, E101-R **sem linha** | medir 36/38, corrigir 37/39, 6 programas sem linha |
| Chao Garden (69–72) | `al_main`+`al_garden00-02`+`al_race` linkados, **0 medidas**; alife 27 símbolos, garden 9 | 4 linhas |
| Minijogos | Sky Chase 1/2 e Sand Hill rodam (row 68, `8f52bbe305a4`); Twinkle Circuit (65) 95% preto | pista do Circuit + regressão de 66/67/68 com este `.mod` |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1` | 448 linhas (`zplm.c:146-153` dobra o step) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes, pitch bend (`src/sound/sound.c:757`), SFX 259 |
| Save | cartão emulado + RTC; `buIsReady` corrigido (`8189f9b`); `vmu_import.py` | save no campo nunca exercitado; `card_*` 9 símbolos; `CHECK_GAME=1` para em `ava_vmssel` |
| Texto/2D | `tex_names[]` corrigido (`37c1974`); 2D e depth do preview verificados | texto do aviso do cartão não aparece (`present_record`) |
| Desempenho | 14–36 ms/quadro de jogo no zeebx (rows 3, 6, 9, 12, 40) | ms no Adreno 130 (sem aparelho); gate por `--frames` antes de qualquer número |
| Pacote | `.mod` 29,35 MiB + 249 `.ovl` (35,15 MiB); disco 99%, **4,7 GB** | cortar para 26–27 MiB; instalador; espaço |

#### 20 quick wins (X1–X20, abertos em 25/11 06:35; P ≤ 1 h, M ≤ meio dia)

**Fechados desde a lista V1–V20 das 04:20:** nenhum item inteiro. Parciais: **V2** está provado em três
linhas novas (b2+b3 entra em modo 4 nas rows 6, 9 e 12), mas o default do `stage_matrix.sh` continua
`14000:b1,17000:b1` — vira X2/X3. **V4** subiu de 7 para 11 de 76 (rows 6, 9, 12). O resto segue aberto e
foi renumerado de X1 a X20 com a evidência atualizada.

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| X1 | Gate de desenho por `--frames` no lugar de `--seconds=100`, e só então refazer o `.ref` | M | `tools/hash_ref.sh:10`; 3241 × 3088 quadros no mesmo `.mod`; 2×388 idênticas com `--frames`; row 6 repetida hoje deu `3be948f582a6` nas duas |
| X2 | `frame_hash.sh:15`: default do `ZEEBX` aponta para `~/projects/zeebx-v060`, que **não existe**; usar o copytex (como `hash_ref.sh:9`) ou detectar | P | `FALHOU: sem quadro (rc=127)` → `nice: .../zeebx-v060/...: Arquivo ou diretório inexistente`; único binário: `~/projects/zeebx-emu-copytex/target/release/zeebx` (24/11 16:56) |
| X3 | `stage_matrix.sh`: default `KEYS=6000:b2,8000:b3`, soltar as teclas, e um `.log` por linha em `matrix_r/` | P | script hoje usa `14000:b1,17000:b1` (`b1` morre, V1/X4); rows 6/9/12 entraram em modo 4 só com b2+b3 |
| X4 | Medir o caminho do `press` (`pad_scan` com contador, sonda revertida) | M | 688 quadros `5d38968c25a3` com e sem A; d-pad muda (`74c7593577b6`); `zsys.c:203`; `frame.c:144,155`; `zninjasys.c:79` |
| X5 | 7 linhas no menu de debug: Hot Shelter (`stg12`), CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R | P | `title.c:202` (`DEBUG_STAGES 76`) e `:384-460`; 0/7; os 7 já em `PROGRAMS` (`game.mk:165-166`) |
| X6 | Rodar as 65 linhas restantes da matriz, quadro + log por linha | M | **11 de 76** com artefato; restam 15–34, 36–39, 41–72, 73–75; `stage_matrix.sh` faz 29 por vez |
| X7 | Atos 2+ das 9 fases de ação: 26 linhas de 6–34 ainda sem medida | M | rows 4–5, 7–8, 10–11, 13–14, 16–17, 19–20, 22–23, 25–27, 29–31, 33–34 |
| X8 | `zdc_ram`: parar de ler o binário SH-4 na `MODULE_AREA` (~2 MB; o jogo roda do overlay ARM) | M | 4,5–4,7 MB tocados de 7,34; maior bloco 1,9–2,2 MB desde a base; nota de 03:13 |
| X9 | `.mod` de 29,35 para 26–27 MiB: mapa ZDC por áreas (≤2,7 MB) + regiões de dados de 5,89 MB | M | `.mod` 30.770.572 B; `zovl_modulo` `0x223760`; `elf2mod` ro/rw/zi |
| X10 | Tirar os ~8 MB de bss de buffers (`clip_wav`, `dialog_pixels` ×2, `polygon_pool`, `card_work`) para `malloc` | M | nota de 03:13; `.mod` zi 21,8 MB; mover bss não muda a RAM, só o arquivo |
| X11 | Chefes: medir Chaos 2 (36) e Egg Viper (38), corrigir Chaos 4 (37) escuro/esverdeado | M | `70b135d59597`, 800 quadros; rows 36/38 sem artefato |
| X12 | Chaos 6/7 nos overlays e linha no menu para os 6 chefes que não têm | M | `chaos6_*` 2 símbolos, `chaos7_*` 5; rows de chefe hoje: 35–39 |
| X13 | Chao: alife e o resto do garden | M | 27 símbolos `alife_*` + 9 `garden_*`; rows 69–72 sem medida |
| X14 | Tikal jogável em Mystic Ruins/Past | M | `tikal_object`, `tikal_player_step` (14 refs), `tikal_walk_action`; `tikal_prog` é overlay próprio (`game.mk:180`) |
| X15 | 34 símbolos `dat_0c…` (endereços absolutos do DC) | M | `dat_0cb2d478`, `dat_0cac8a7c`, `dat_0cb18618`; `config/adv00/symbols.txt:1340` |
| X16 | 19 indefinidos do `.mod` + os 114 dos overlays: `strncmp` 5, `draw_palettes` 2, `memmove`, `loader_buffer`, `cache_ram_init/_restore`, `syCfgSetSoundMode`, `card_drive_object`, `eggmanrobo_land` | P | `build/zeebo/game/integrate.err` (19); `nm -u build/zeebo/game/ovl/program-*.o` (114); nenhum exige engenharia reversa |
| X17 | Cartão e save no campo: 9 símbolos `card_*` (`card_menu_drives`, `card_write_chao`, `card_read_chao`…) | M | `CHECK_GAME=1` para em `ava_vmssel`; RTC já real; `vmu_import.py` pronto |
| X18 | 7 refs `egg_viper_event_*` (chefe 38, `b_egm1`) e 4 símbolos `warp_*` (teleportes) | P | refs medidas nos overlays; `b_egm1` linkado (`game.mk:166`) |
| X19 | Filmes em 448 linhas inteiras (`step=1`) | M | `ports/zeebo/zplm.c:146-153`: `while height/step > maxh: step++` → 224 de 448 |
| X20 | Twinkle Circuit (row 65) + regressão de Sand Hill/Sky Chase com este `.mod` (o código em overlay mudou como o stage carrega) | M | `52455310e0db`, 879 quadros, pista 95% preto; row 68 `8f52bbe305a4` (23/11); rows 66/67 nunca re-medidas |

Áudio (17/179 clipes, pitch bend em `src/sound/sound.c:757`, 259 SFX) ficou fora dos 20 por já estar
bem descrito em `otimizacoes-propostas.md`; segue aberto, esforço M.

Fora dos 20, ainda abertos: ms no aparelho (sem Zeebo em mão), o aviso de apóstrofo em
`engine/native/prelude.h:18` (cosmético, suja os `.err`) e a limpeza de `keep/`/`game_o2/` —
destrutiva, só com autorização explícita (disco a 99%, 4,7 GB).

## Atualização 2026-11-25 08:40 — Mystic Ruins (row 58) e Chao Garden (row 69) medidos; 13 de 76 linhas

Rodada do cron (08:25–08:40). `HEAD` = `41384dc` (06:30), branch `zeebo-port`, árvore **limpa** (só
`sa1n.bmp` e `todo.md` não rastreados, os de sempre). **Zero commits desde a atualização das 06:35** — o
último commit de código continua `332d732` (03:13). Esta rodada commitou só este documento; o que há de
novo são **duas linhas da matriz medidas pela primeira vez** (rows 58 e 69), as duas na família FIELD e
as duas reprodutíveis.

| Item | Medido agora (08:30–08:38) | Observação |
|---|---|---|
| `make -C ports/zeebo` | rc=0, **nada a recompilar**; `sa1z.elf` 302.948 B, 03:13 | applet feito antes do `mod`, como manda a armadilha do zstrip/ztexture/app.c |
| `make -f game.mk mod` | **752 de 752 objetos**, `duplicatas: 0`, **indefinidos: 19** | nenhum `FALHOU`; 13 `.err` no disco, **0** com `error:` |
| `sa1g.mod` | **30.770.572 B** (29,35 MiB), 08:37 | mesmo tamanho de 04:20 e 06:27: link determinístico |
| `.ovl` | **249** = **36.853.845 B** (35,15 MiB) | `gen_ovl: 249 overlays, regiao 2242340 B` |
| `host-selftest` | **PASSED (0 failures)** | |
| `PROGRAMS` | **251** = 47 programas + 204 eventos; `MISSING_PROGRAMS` vazio | `modules/` tem 41 dirs (sem `movie`); **não existe `stg11`** |
| Indefinidos | **5** no `sa1g.elf` (`card_drive_object`, `dat_0ca4ec58`, `dat_0ca4f790`, `dat_0ca50678`, `dat_0cb2d478`); **114** dentro dos overlays (`nm -g -u` menos o que o ELF e os overlays definem) | famílias: `dat_0c…` 34, `alife_*` 27, `garden_*` 9, `card_*` 9, `egg*` 7, `chaos7_*` 5, `warp_*` 4, `tikal_*` 3, `chaos0/2/6_*` 3/3/2, `func_*` 2, `chao_*` 2 |
| Matriz | **13 de 76** linhas medidas: 3, 6, 9, 12, 37, 39, 40, 46, **58**, 65, 68, **69**, 71 | eram 11 às 06:35; 14 PNGs em `build/zeebo/matrix_r/` |
| Disco | `/` a **99%**, **4,7 GB** livres; `build/zeebo` sozinho = **17 GB** | `find build/zeebo -name '*.ovl'` = 20.349 cópias antigas (2,7 GB só de `.ovl`) |

#### O achado desta rodada: os dois primeiros campos desenham, e o menu de debug os entrega pelo mesmo atalho

Rows 58 (Mystic Ruins) e 69 (Chao Garden) nunca tinham sido medidas — a última vez que apareceram foi
às 13h de 23/11, quando voltavam à tela de reinício por `programa ausente`. Agora:

| row | fase | modo | quadros (45 s) | último hash | não-preto (>24) | cores | traps |
|---:|---|---|---:|---|---|---:|---|
| 58 | Mystic Ruins | `0x5` FIELD (entra em 6552 ms) | **1366** | `17e45c00ea25` | 98,7% | 2.587 | 1 (`cache_ram_init`) |
| 69 | Chao Garden | `0x5` FIELD (entra em 6552 ms) | **1384** | `89c67795ce0b` | 99,2% | 2.518 | 1 (`cache_ram_init`) |

`ausente=0` nas duas; a row 58 rodou **duas vezes e deu o mesmo hash nas duas** (`17e45c00ea25`), o que
confirma de novo que o desenho é reprodutível e que o vermelho do `hash_ref.sh check` é só a contagem por
tempo de parede (Y1). As duas entram em FIELD com as mesmas teclas das fases de ação (`6000:b2,8000:b3`),
sem precisar de d-pad.

O que os números dizem e o que não dizem: desenham — 98,7%/99,2% dos pixels não-pretos (métrica soma RGB
> 24, a mesma que reproduz 98,6/97,5/95,8 das rows 6/9/12) e média RGB 109,96,98 / 152,151,133 — mas com
**2.587 e 2.518 cores** contra **10.868** da Windy Valley e **9.451** do Twinkle Park. Cena com pouca
variação de cor pode ser noite/floresta (Mystic Ruins) ou jardim chapado, e pode ser textura faltando
(`alife_*`/`garden_*` 36 símbolos indefinidos) — não dá para separar sem olhar o PNG contra o PC
(`12b`). Não afirmo que está certo; afirmo que não é tela preta e que não há trap nem programa ausente.

#### O que foi feito desde a atualização das 06:35

- **nenhum commit**, de código ou de docs, entre 06:30 (`41384dc`) e 08:38;
- rows 58 e 69 medidas (45 s cada, `ZEEBX=~/projects/zeebx-emu-copytex/target/release/zeebx`,
  `KEYS=6000:b2,8000:b3`), logs em `build/zeebo/matrix_r/row{58,69}.log` e PNGs
  `row{58,69}_2026-11-25.png`; o `.mod` foi reconstruído com `DEBUG_TITLE` e **devolvido ao boot normal**
  (30.770.572 B às 08:37);
- métrica de “não-preto” fixada em soma RGB > 24 e reaplicada às 13 linhas, para os números ficarem
  comparáveis entre rodadas (o `> 12` que eu tinha usado dava 99,2/98,6/96,5 nas rows 6/9/12).

#### O que falta para o OBJETIVO FINAL (port completo, todas as fases e conteúdo)

Mapa do menu (`src/progress/title.c:384-460`, 76 linhas): 0–2 Practice, 3–5 Emerald Coast, 6–8 Windy
Valley, 9–11 Twinkle Park, 12–14 Speed Highway, 15–17 Red Mountain, 18–20 Sky Deck, 21–23 Lost World,
24–27 Ice Cap, 28–31 Casinopolis, 32–34 Final Egg, 35 Chaos 0, 36 Chaos 2, 37 Chaos 4, 38 Egg Viper 1,
39 “Zero”, 40–45 Station Square, 46–51 Egg Carrier, 52–57 Egg Carrier por dentro, 58–61 Mystic Ruins,
62–64 Past, 65 Twinkle Circuit, 66–67 Sky Chase, 68 Sand Hill, 69–72 Chao Garden, 73–75 zeros.

| Conteúdo | Estado em 08:40 | Falta |
|---|---|---|
| Emerald Coast (3–5) | row 3 medida (`8031d98b7314`, modo 4, 20–21 ms) | rows 4–5 |
| Windy Valley (6–8) | row 6 (`3be948f582a6`, modo 4, 23–24 ms, 98,6%) | rows 7–8 |
| Twinkle Park (9–11) | row 9 (`09e03f1bc6e2`, modo 4, 34–36 ms, 97,5%) | rows 10–11 |
| Speed Highway (12–14) | row 12 (`3a0ac207e776`, modo 4, ~20 ms, 95,8%) | rows 13–14 |
| Red Mountain (15–17), Sky Deck (18–20), Lost World (21–23), Ice Cap (24–27), Casinopolis (28–31), Final Egg (32–34) | 20 linhas, **nenhuma medida** com este `.mod` | 20 linhas |
| Hot Shelter (`stg12`) | linkado (`game.mk:165`, `src/stage/stage.c:97`), **sem linha no menu** | linha + medida |
| Station Square (40–45) | row 40 (`9f1f67d18d0e`, modo 5, 14–15 ms, 99,6%) | 5 linhas (41–45) |
| Egg Carrier (46–51) e por dentro (52–57) | row 46 escura, HUD sem cor (`71b404f1d722`, 23/11; 86,9% >24, 691 cores) | 11 linhas |
| Mystic Ruins (58–61) | **row 58 medida agora** (`17e45c00ea25`, FIELD, 98,7%, 2.587 cores) | rows 59–61 (ato 2 e as duas entradas) |
| Past (62–64) | nunca medidas; `tikal_*` 3 símbolos / 14 refs de `tikal_player_step` | 3 linhas + Tikal jogável |
| Chefes | Chaos 0 (35) medido e otimizado; Chaos 2 (36) nunca; Chaos 4 (37) escuro (`70b135d59597`, média RGB 27,50,49); Egg Viper 1 (38) nunca; “Zero” (39) 0,9% não-preto (`c2eb03f3a477`); Chaos 6/7, EGM2/3, E101, E101-R **sem linha** | medir 36/38, corrigir 37/39, 6 programas sem linha |
| Chao Garden (69–72) | **row 69 medida agora** (`89c67795ce0b`, FIELD, 99,2%, 2.518 cores); `al_main`+`al_garden00-02`+`al_race` linkados | rows 70–72; alife 27 + garden 9 símbolos |
| Minijogos | Sky Chase 1/2 e Sand Hill rodam (row 68, `8f52bbe305a4`, 97,8%); Twinkle Circuit (65) **16,6% não-preto** | pista do Circuit + regressão de 66/67/68 com este `.mod` |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1` | 448 linhas (`zplm.c:146-153` dobra o step) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes, pitch bend (`src/sound/sound.c:757`), SFX 259 |
| Save | cartão emulado + RTC; `buIsReady` corrigido (`8189f9b`); `vmu_import.py` | save no campo nunca exercitado; `card_*` 9 símbolos (1 é indefinido até no ELF); `CHECK_GAME=1` para em `ava_vmssel` |
| Texto/2D | `tex_names[]` corrigido (`37c1974`); 2D e depth do preview verificados | texto do aviso do cartão não aparece (`present_record`) |
| Desempenho | 14–36 ms/quadro de jogo no zeebx (rows 3, 6, 9, 12, 40) | ms no Adreno 130 (sem aparelho); gate por `--frames` antes de qualquer número |
| Pacote | `.mod` 29,35 MiB + 249 `.ovl` (35,15 MiB); disco 99%, **4,7 GB** | cortar para 26–27 MiB; instalador; espaço (build/zeebo = 17 GB) |

#### 20 quick wins (Y1–Y20, abertos em 25/11 08:40; P ≤ 1 h, M ≤ meio dia)

**Fechados desde a lista X1–X20 das 06:35: nenhum** (não houve commit). Dois itens melhoraram de
evidência sem fechar: **X6** subiu de 11 para 13 de 76 (rows 58 e 69), **X13** ganhou a primeira medida de
Chao Garden. O resto foi renumerado de Y1 a Y20 com a evidência atualizada; o que não muda de estado não
muda de conteúdo.

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| Y1 | Gate de desenho por `--frames` no lugar de `--seconds=100`, e só então refazer o `.ref` | M | `tools/hash_ref.sh:14` (`--seconds=100`); 3241 × 3088 quadros no mesmo `.mod`; row 58 repetiu `17e45c00ea25` nas duas rodadas de hoje |
| Y2 | `frame_hash.sh:15`: default do `ZEEBX` aponta para `~/projects/zeebx-v060`, que **não existe** | P | `FALHOU: sem quadro (rc=127)`; único binário: `~/projects/zeebx-emu-copytex/target/release/zeebx` |
| Y3 | `stage_matrix.sh:17`: default `KEYS=14000:b1,17000:b1` não entra na fase; usar `6000:b2,8000:b3` e um `.log` por linha em `matrix_r/` | P | rows 58 e 69 entraram em FIELD aos **6552 ms** com b2+b3; `b1` não sai do menu (`5d38968c25a3` com e sem A) |
| Y4 | Medir o caminho do `press` (`pad_scan` com contador, sonda revertida) | M | 688 quadros `5d38968c25a3` com e sem A; d-pad muda (`74c7593577b6`); `zsys.c:203`; `frame.c:144,155`; `zninjasys.c:79` |
| Y5 | 7 linhas no menu de debug: Hot Shelter (`stg12`), CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R | P | `title.c:202` (`DEBUG_STAGES 76`) e `:384-460`; 0/7; os 7 já em `PROGRAMS` (`game.mk:165-166`) |
| Y6 | Rodar as 63 linhas restantes da matriz, quadro + log por linha | M | **13 de 76** medidas hoje; restam 0–2, 4–5, 7–8, 10–11, 13–34, 36–38, 41–45, 47–57, 59–64, 66, 67, 70, 72–75 |
| Y7 | Atos 2+ das fases de ação: 24 linhas ainda sem medida | M | rows 4–5, 7–8, 10–11, 13–14, 16–17, 19–20, 22–23, 25–27, 29–31, 33–34 |
| Y8 | `zdc_ram`: parar de ler o binário SH-4 na `MODULE_AREA` (~2 MB; o jogo roda do overlay ARM) | M | 4,5–4,7 MB tocados de 7,34; maior bloco 1,9–2,2 MB desde a base; nota de 03:13 |
| Y9 | `.mod` de 29,35 para 26–27 MiB: mapa ZDC por áreas (≤2,7 MB) + regiões de dados de 5,89 MB | M | `.mod` 30.770.572 B; `zovl_modulo` `0x223760`; `elf2mod` ro/rw/zi |
| Y10 | Limpar as 20.349 cópias de `.ovl` dentro de `build/zeebo` antes do próximo pacote | P | `build/zeebo` = **17 GB**, disco **4,7 GB** livres (99%); 2,7 GB só de `.ovl` fora do pacote |
| Y11 | Chefes: medir Chaos 2 (36) e Egg Viper (38); corrigir Chaos 4 (37) escuro/esverdeado e “Zero” (39) | M | row 37 média RGB **27,50,49** e 72,0% >24 (`70b135d59597`); row 39 **0,9%** não-preto; rows 36/38 sem artefato |
| Y12 | Chaos 6/7 nos overlays e linha no menu para os 6 chefes que não têm | M | `chaos6_*` 2 símbolos, `chaos7_*` 5; rows de chefe hoje: 35–39 |
| Y13 | Chao: alife e o resto do garden, depois da row 69 | M | 27 `alife_*` + 9 `garden_*` indefinidos; row 69 mediu 2.518 cores (contra 10.868 da Windy Valley): checar textura |
| Y14 | Tikal jogável em Mystic Ruins/Past | M | `tikal_object`, `tikal_player_step` (14 refs), `tikal_walk_action`; `tikal_prog` é overlay próprio (`game.mk:180`); row 58 mediu, Past (62–64) nunca |
| Y15 | 34 símbolos `dat_0c…` (endereços absolutos do DC) | M | `dat_0cb2d478` (o único que também falta no ELF), `dat_0cac8a7c`, `dat_0cb18618`; `config/adv00/symbols.txt:1340` |
| Y16 | 114 símbolos dos overlays + os 5 do ELF: `strncmp`, `draw_palettes`, `memmove`, `loader_buffer`, `cache_ram_init/_restore`, `syCfgSetSoundMode`, `card_drive_object`, `eggmanrobo_land` | P | `nm -g -u build/zeebo/game/ovl/program-*.o` → 114; `nm -u build/zeebo/sa1g.elf` → 5; nenhum exige engenharia reversa |
| Y17 | Cartão e save no campo: 9 símbolos `card_*` (`card_menu_drives`, `card_write_chao`, `card_read_chao`…) | M | `CHECK_GAME=1` para em `ava_vmssel`; RTC já real; `vmu_import.py` pronto; `card_drive_object` indefinido no ELF final |
| Y18 | 7 refs `egg_viper_event_*` (chefe 38, `b_egm1`) e 4 símbolos `warp_*` (teleportes) | P | refs medidas nos overlays; `b_egm1` linkado (`game.mk:166`) |
| Y19 | Filmes em 448 linhas inteiras (`step=1`) | M | `ports/zeebo/zplm.c:146-153`: `while height/step > maxh: step++` → 224 de 448 |
| Y20 | Twinkle Circuit (row 65) + regressão de Sand Hill/Sky Chase com este `.mod` | M | row 65 **16,6%** não-preto e 285 cores (`52455310e0db`, 879 quadros); row 68 `8f52bbe305a4` (23/11); rows 66/67 nunca re-medidas |

Áudio (17/179 clipes, pitch bend em `src/sound/sound.c:757`, 259 SFX) ficou fora dos 20 por já estar
bem descrito em `otimizacoes-propostas.md`; segue aberto, esforço M.

Fora dos 20, ainda abertos: ms no aparelho (sem Zeebo em mão), o aviso de apóstrofo em
`engine/native/prelude.h:18` (cosmético, suja os `.err`), `MISSING_PROGRAMS`/`stg11` (não existe no
decomp) e a limpeza de `keep/`/`game_o2/` — destrutiva, só com autorização explícita (disco a 99%,
4,7 GB).

### O limite real é a RAM de pico, não o tamanho do arquivo (2026-11-25)

`ZeeboDeveloperGuide0.97.md` §5.1: heap de **32 MB**, o `.mod` é carregado inteiro nele; o device GEC do
simulador (`ZeeboSDKInstaller.msi`) declara `HEAPSIZE 16000000`. Pico = `.mod` + uso do `malloc` (medido
no zeebx: 7,85 MB em uso, 15 blocos). Com `.mod` 30,8 MB o pico passa de 38 MB: **não cabe em 32 MB**; para
caber o `.mod` precisa ficar em ~24 MB ou menos (e em ~8 MB para os 16 MB do simulador). Passar bss para
`malloc` não ajuda se o buffer vive o tempo todo; só ajuda o que for alocado e liberado por fase.

Candidatos medidos (bss de 8,2 MB fora do `zdc_ram`): `clip_wav` 0,66, `heap_arena` 0,59, `dialog_pixels`
2×0,52 (um em `advertise`, outro em `summary`, que nunca rodam juntos), `polygon_pool` 0,52, `card_work` 0,50,
`zbank_mem` 0,31, `tex_rgba`/`tex_scratch`/`texture_buffer` 3×0,26, `zasset_adx` 0,20. O bss dos programas
soma só 1,94 MB (`al_main` 0,63, `advertise` 0,54, `summary` 0,52) e o ganho de pô-lo na região do overlay
seria de 0,2–0,4 MB: não vale. Maiores alavancas: `zdc_ram` compactado (−2,7 MB, piso medido 4,5 MB tocados),
`dialog_pixels` compartilhado (−0,5), buffers de textura/áudio alocados por fase.

### Estudo da RAM de pico (2026-11-25)

Pico de `malloc` (zeebx, `mem minimo livre KB` do log, 10 linhas do menu de debug, 75 s): **9,5 MB em todas**,
quase constante. Causa principal: `mc_arena` de 6 MB (`zlive.c`), o cache dos pontos de modelo já convertidos
para ponto fixo. Medido o uso real: 2,5–3,8 MB em 75 s e crescendo devagar (modelos distintos acumulados), nunca
chegou a 6. Com 2 MB o cache reinicia a cada ~40 s (reconverte só o que aparece), 0 faults, e o pico caiu para
**5,4 MB** (`5ae288d`). O resto, por `--trace=malloc` (só aloca, sem free): `njInitVertexBuffer` 2×640 KB,
um bloco de 630 KB e ~20× 256 KB em `surface_upload` (buffers de textura; vivos ao mesmo tempo não medido).

Pico total hoje = `.mod` 30,8 MB + 5,4 MB ≈ 36 MB contra o heap de 32 MB do Zeebo: **ainda não cabe**. O `.mod` é
quem manda: ro 3,5 + rw 5,1 + zi 21,8 (`zdc_ram` 7,34, outros bss 8,2, regiões de overlay 6,3). `zdc_ram` só se
encolhe com o mapa completo das áreas do DC que o jogo usa (a medição por páginas tocadas deixa buracos que outros
stages podem usar: 0x0cf00000–0x0cfff000 nunca tocada em 2 stages, mas é o `IDLE_MEMORY`/pool do jogo). Não tentei
compactar sem esse mapa — **outra sessão começou a fazer exatamente isso às 10:50** (ver a atualização abaixo).

---

## Atualização 2026-11-25 10:54 — pico de malloc 9,5 → 5,4 MB e `zdc_ram` sem o binário SH-4; Chaos 2, Egg Viper 1 e Past medidos (16 de 76)

Rodada do cron (10:40–10:54). `HEAD` = `7b4133d` (10:04), branch `zeebo-port`. Entrei com a árvore **limpa**
(só `sa1n.bmp` e `todo.md` não rastreados, os de sempre) e medi tudo entre 10:41 e 10:47. **Às 10:50:49 outra
sessão passou a editar `zgdfs.c`, `zrebase.c` e `zrebase.h`** (o mapa por trechos do `zdc_ram`) e o `sa1g.mod`
de 10:52:58 já é **28.096.508 B** — em fluxo, não medido por mim; os números da tabela abaixo são do `.mod`
determinístico de 10:42–10:50 (30.770.796 B), construído a partir de `7b4133d` sem alteração local.

| Item | Medido nesta rodada | Observação |
|---|---|---|
| `make -C ports/zeebo` | rc=0, nada a recompilar; `sa1z.elf` 303.208 B | applet feito antes do `mod`, como manda a armadilha do zstrip/ztexture/app.c |
| `make -f game.mk mod` | **752 de 752 objetos**, `duplicatas: 0`, **indefinidos: 19** | nenhum `FALHOU`; 13 `.err` no disco, **0** com `error:` |
| `sa1g.mod` | **30.770.796 B** (29,35 MiB), 10:50 | +224 B sobre os 30.770.572 B de 08:37: é o `zprs_skip_next` de `7b4133d`; link determinístico (mesmo tamanho em dois builds) |
| `.ovl` | **249** = **36.853.937 B** (35,15 MiB) | `gen_ovl: 249 overlays, regiao 2242340 B` (era 36.853.845 B às 08:37) |
| `host-selftest` | **PASSED (0 failures)** | |
| `PROGRAMS` | **251** = 47 programas + 204 eventos; `MISSING_PROGRAMS` vazio | confirmado por `make -p`; `modules/` tem 41 dirs (sem `movie`); não existe `stg11` |
| Indefinidos | **5** no `sa1g.elf` (`card_drive_object`, `dat_0ca4ec58`, `dat_0ca4f790`, `dat_0ca50678`, `dat_0cb2d478`); **114** dentro dos overlays (`nm -g -u` menos o que ELF e overlays definem) | famílias: `dat_0c…` 34, `alife_*` 27, `garden_*` 9, `card_*` 9, `egg*` 7, `chaos7_*` 5, `warp_*` 4, `tikal_*` 3, `chaos2/0/6_*` 3/3/2, `func_*` 2 |
| Matriz | **16 de 76** linhas: 3, 6, 9, 12, **36**, 37, **38**, 39, 40, 46, 58, **62**, 65, 68, 69, 71 | eram 13 às 08:40; as três novas são desta rodada (45 s cada, `KEYS=6000:b2,8000:b3`) |
| Disco | `/` a **100%**, **3,8 GB** livres; `build/zeebo` = **17 GB** | 20.349 cópias de `.ovl` continuam lá (2,7 GB fora do pacote) |

### O que foi feito desde a atualização das 08:40 (4 commits, `db80852`..`7b4133d`)

| Commit | O que mudou | Efeito medido por quem commitou |
|---|---|---|
| `db80852` (09:11) | docs: heap de 32 MB (GEC 16 MB), o `.mod` inteiro vive nele, candidatos de bss | fecha o diagnóstico "o limite é RAM de pico, não tamanho de arquivo" |
| `5ae288d` (09:31) | `zlive.c:536`: cache de pontos convertidos **6 → 2 MB** (`MC_ARENA`) | pico de malloc **9,5 → 5,4 MB**; cache reinicia a cada ~40 s e reconverte só o que aparece |
| `3fade5e` (09:32) | docs: estudo do pico (`--trace=malloc`: 2×640 KB do `njInitVertexBuffer`, ~20×256 KB em `surface_upload`) | registro do que compõe os 5,4 MB |
| `7b4133d` (10:04) | `zgdfs.c`+`prs.c`: com overlay ARM carregado, `prs_decompress` **pula o binário SH-4** do programa (contador `zprs_skip_next`) | `zdc_ram` tocado **4,6 → 2,0 MB** e fase carrega mais rápido |

### O que os números novos dizem

- **Pico total agora:** `.mod` 30,77 MB + 5,4 MB de malloc ≈ **36,2 MB** contra o heap de 32 MB — **ainda não
  cabe**. Para caber com o pico atual, o `.mod` tem de ficar em **≤ 26,6 MB** (32 − 5,4). Com os 28,1 MB que a
  outra sessão já produz às 10:52, o pico teórico cai para ~33,5 MB: falta **~1,5 MB**, e é por isso que o mapa
  do `zdc_ram` (7,34 → 4,45 MB na árvore de 10:50) é o caminho, não um exercício.
- **Não consegui reproduzir a medida de 5,4 MB** nesta rodada: o log de `mem minimo livre KB` (`app.c:278`)
  aparece num run curto (12 s: 64.831 KB livres de 64 MB, ou seja 0,7 MB usados no boot), mas some nos runs de
  75 s porque o log do zeebx guarda só o fim (milhares de linhas de GL tomam o buffer). O número de 5,4 MB é da
  sessão que mediu com 10 linhas do menu; fica como **medida de terceiro**, reproducível só com um gate de log.
- **`zprs_skipped` é escrito e nunca lido** (`zgdfs.c:129` define, zero leitores): não há prova no log de que o
  skip dispara. É quick win P (Z6).
- **Rows 36, 38 e 62 desenham.** Chaos 2 (36) e Egg Viper 1 (38) entram em ACTION (`0x4`); Past (62) entra em
  FIELD (`0x5`). Nenhuma tela preta, nenhum `programa ausente`, `traps=1` (só o stub `cache_ram_init`). A row 62
  rodou **duas vezes com o mesmo hash** (`6f20d39ebe77`), de novo com o `.mod` recém-construído.

| row | fase | modo | quadros (45 s) | hash | não-preto (>24) | cores | média RGB | traps |
|---:|---|---|---:|---|---|---:|---|---:|
| 36 | Chaos 2 | `0x4` ACTION (6552 ms) | 1387 | `0d81563c8df6` | 95,9% | 2.994 | 137,0/157,1/169,1 | 1 |
| 38 | Egg Viper 1 | `0x4` ACTION (6552 ms) | 1394 | `573547c0cc7d` | 98,7% | 1.988 | 91,9/96,6/149,7 | 1 |
| 62 | Past | `0x5` FIELD (6552 ms) | 1392 | `6f20d39ebe77` (2×) | 99,5% | 3.605 | 128,5/119,4/100,7 | 1 |

PNGs e logs em `build/zeebo/matrix_r/row{36,38,62}_2026-11-25b.*`. O que os números **não** dizem: 1.988–3.605
cores é pouco contra as 10.868 da Windy Valley e 9.451 do Twinkle Park — pode ser cena noturna/escura (Chaos 2 é
de noite, Egg Viper é hangar) ou textura faltando (`egg*` 7 e `dat_0c…` 34 indefinidos). Não afirmo que está
certo; afirmo que não é tela preta. Separar isso exige comparar o PNG com o PC (Z11).

### Achado lateral: o atalho do boot normal cai num quadro que já tem nome

Run de 75 s **sem `DEBUG_TITLE`** (boot normal: LOGO → MOVIE em 3,4 s), segurando b2+b3 em 6/8 s, rendeu 4.299
quadros e hash **`8f52bbe305a4`** — o mesmo hash registrado para a **row 68 (Sand Hill)** em 23/11. Ou o atalho
do título cai direto no Sand Hill (último save/pista padrão), ou a cena é estática o bastante para repetir o
quadro. Não é conclusão, é pista: barato de confirmar (Z18) e mudaria o caminho de teste de todas as fases.

#### O que falta para o OBJETIVO FINAL (port completo, todas as fases e conteúdo)

Mapa do menu (`src/progress/title.c:384-460`, 76 linhas): 0–2 Practice, 3–5 Emerald Coast, 6–8 Windy Valley,
9–11 Twinkle Park, 12–14 Speed Highway, 15–17 Red Mountain, 18–20 Sky Deck, 21–23 Lost World, 24–27 Ice Cap,
28–31 Casinopolis, 32–34 Final Egg, 35 Chaos 0, 36 Chaos 2, 37 Chaos 4, 38 Egg Viper 1, 39 "Zero", 40–45
Station Square, 46–51 Egg Carrier, 52–57 Egg Carrier por dentro, 58–61 Mystic Ruins, 62–64 Past, 65 Twinkle
Circuit, 66–67 Sky Chase, 68 Sand Hill, 69–72 Chao Garden, 73–75 zeros.

| Conteúdo | Estado em 10:54 | Falta |
|---|---|---|
| Emerald Coast (3–5) | row 3 (`8031d98b7314`, modo 4, 20–21 ms) | rows 4–5 |
| Windy Valley (6–8) | row 6 (`3be948f582a6`, modo 4, 98,6%) | rows 7–8 |
| Twinkle Park (9–11) | row 9 (`09e03f1bc6e2`, 97,5%) | rows 10–11 |
| Speed Highway (12–14) | row 12 (`3a0ac207e776`, 95,8%) | rows 13–14 |
| Red Mountain (15–17), Sky Deck (18–20), Lost World (21–23), Ice Cap (24–27), Casinopolis (28–31), Final Egg (32–34) | 20 linhas, **nenhuma medida** com este `.mod` | 20 linhas |
| Hot Shelter (`stg12`) | linkado (`game.mk:165`, `src/stage/stage.c:97`), **sem linha no menu** | linha + medida |
| Station Square (40–45) | row 40 (`9f1f67d18d0e`, 99,6%) | 5 linhas (41–45) |
| Egg Carrier (46–51) e por dentro (52–57) | row 46 escura, HUD sem cor (`71b404f1d722`, 86,9% >24, 691 cores) | 11 linhas |
| Mystic Ruins (58–61) | row 58 (`17e45c00ea25`, FIELD, 98,7%, 2.587 cores) | rows 59–61 |
| Past (62–64) | **row 62 medida agora** (`6f20d39ebe77` 2×, FIELD, 99,5%, 3.605 cores) | rows 63–64 + Tikal jogável |
| Chefes | Chaos 0 (35) ok; **Chaos 2 (36) e Egg Viper 1 (38) medidos agora** (95,9%/98,7%); Chaos 4 (37) escuro (`70b135d59597`, média 27,50,49); "Zero" (39) 0,9% | corrigir 37/39; Chaos 6/7, EGM2/3, E101, E101-R **sem linha** |
| Chao Garden (69–72) | row 69 (`89c67795ce0b`, 99,2%, 2.518 cores); `al_main`+`al_garden00-02`+`al_race` linkados | rows 70–72; alife 27 + garden 9 |
| Minijogos | Sky Chase 1/2 e Sand Hill rodam (row 68, `8f52bbe305a4`, 97,8%); Twinkle Circuit (65) **16,6%** | pista do Circuit; regressão 66/67/68 com o `.mod` novo |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1` | 448 linhas (`zplm.c:146-153`) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes, pitch bend (`sound.c:757`), 259 SFX |
| Save | cartão emulado + RTC; `buIsReady` (`8189f9b`); `vmu_import.py` | save no campo nunca exercitado; `card_*` 9 símbolos; `CHECK_GAME=1` para em `ava_vmssel` |
| Texto/2D | `tex_names[]` (`37c1974`); 2D e depth do preview ok | texto do aviso do cartão (`present_record`) |
| Desempenho | 14–36 ms/quadro de jogo no zeebx (rows 3, 6, 9, 12, 40); pico de malloc 5,4 MB | ms no Adreno 130 (sem aparelho) |
| Pacote | `.mod` 29,35 MiB + 249 `.ovl` (35,15 MiB); 28,1 MB em fluxo na árvore de 10:52 | **≤ 26,6 MB** para caber nos 32 MB com o pico atual; instalador; disco 100% |

#### 20 quick wins (Z1–Z20, abertos em 25/11 10:54; P ≤ 1 h, M ≤ meio dia)

**Fechado desde a lista Y1–Y20: Y8** (`7b4133d`: o binário SH-4 do programa não é mais descomprimido;
`zdc_ram` tocado 4,6 → 2,0 MB) — o mapa por trechos que a outra sessão está escrevendo agora é o passo seguinte,
não o mesmo item. **Y11 andou pela metade**: Chaos 2 (36) e Egg Viper 1 (38) medidos; Chaos 4 (37) e "Zero"
(39) seguem ruins. O resto foi renumerado de Z1 a Z20 com a evidência de hoje.

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| Z1 | `frame_hash.sh:15`: default do `ZEEBX` aponta para `~/projects/zeebx-v060`, que **não existe** | P | `FALHOU: sem quadro (rc=127)`; único binário: `~/projects/zeebx-emu-copytex/target/release/zeebx` (usado nesta rodada) |
| Z2 | Gate de desenho por `--frames` no lugar de `--seconds=100`, e só então refazer o `.ref` | M | `tools/hash_ref.sh:14` (`--seconds=100`); 3241 × 3088 quadros no mesmo `.mod`; row 62 repetiu `6f20d39ebe77` 2× |
| Z3 | 7 linhas no menu de debug: Hot Shelter (`stg12`), CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R | P | `title.c:202` (`DEBUG_STAGES 76`) e `:384-460`; 0/7; os 7 já em `PROGRAMS` (`game.mk:165-166`) |
| Z4 | Rodar as 60 linhas restantes da matriz, quadro + log por linha | M | **16 de 76** hoje; restam 0–2, 4–5, 7–8, 10–11, 13–34, 37, 39, 41–45, 47–57, 59–61, 63–64, 66, 67, 70, 72–75 |
| Z5 | Medir o pico de malloc com a árvore de 10:52 (`zdc_ram` 4,45 MB) antes de commitar o mapa | P | `app.c:278` (`minimo livre KB`); hoje 5,4 MB com `.mod` 30,77; `.mod` 28.096.508 B às 10:52 (cabe em 32? 28,1+5,4 = 33,5) |
| Z6 | Logar `zprs_skipped`: definido em `zgdfs.c:129` e **nunca lido** — sem prova de que o skip dispara | P | `grep -rn zprs_skipped ports/zeebo` = 1 ocorrência; `zinst.c:26` só reporta `zprs_calls` |
| Z7 | Fechar e commitar o mapa do `zdc_ram` (7,34 → 4,45 MB) com o selftest novo de `zrebase.c` | M | árvore suja desde 10:50 (`zgdfs.c`, `zrebase.c/h`); selftest confere `0x8ceffffc`, `0x0cb48000`, `0x0c950000`; `.mod` 28,1 MB |
| Z8 | `.mod` de 29,35 para **≤ 26,6 MiB** (32 MB de heap − 5,4 de malloc) | M | `.mod` 30.770.796 B; regiões de dados 5,89 MB (`zovl_modulo` `0x223760`); 26,6 é o teto aritmético, não estimativa |
| Z9 | Limpar as 20.349 cópias de `.ovl` dentro de `build/zeebo` antes do próximo pacote | P | `build/zeebo` = 17 GB; disco **100%**, **3,8 GB** livres; 2,7 GB só de `.ovl` fora do pacote |
| Z10 | Chaos 4 (37) escuro/esverdeado e "Zero" (39) 0,9% não-preto | M | row 37 média RGB **27,50,49** (`70b135d59597`); row 39 **0,9%**; rows 36/38 agora medidos e claros |
| Z11 | Comparar os 3 PNGs novos (36/38/62) com o PC antes de declarar as fases boas | P | 1.988–3.605 cores contra 10.868 (Windy Valley); 114 símbolos indefinidos nos overlays (`egg*` 7, `chaos2_*` 3) |
| Z12 | Chao Garden rows 70–72 e o resto do `alife` | M | 27 `alife_*` + 9 `garden_*` indefinidos; row 69 mediu 2.518 cores (contra 10.868) |
| Z13 | Tikal jogável em Mystic Ruins/Past | M | `tikal_object`, `tikal_player_step` (14 refs), `tikal_walk_action`; row 62 mediu, rows 59–61/63–64 nunca |
| Z14 | Cartão e save no campo: 9 símbolos `card_*` | M | `card_drive_object` indefinido no ELF; `CHECK_GAME=1` para em `ava_vmssel`; RTC real; `vmu_import.py` pronto |
| Z15 | 34 símbolos `dat_0c…` (endereços absolutos do DC) | M | `dat_0cb2d478` (também falta no ELF), `dat_0cac8a7c`, `dat_0cb18618`; `config/adv00/symbols.txt:1340` |
| Z16 | 114 símbolos dos overlays + os 5 do ELF | P | medido hoje: 114 (34 `dat_0c`, 27 `alife_*`, 9 `garden_*`, 9 `card_*`, 7 `egg*`, 5 `chaos7_*`, 4 `warp_*`, 3 `tikal_*`); nenhum exige engenharia reversa |
| Z17 | Filmes em 448 linhas inteiras (`step=1`) | M | `ports/zeebo/zplm.c:146-153`: `while height/step > maxh: step++` → 224 de 448 |
| Z18 | Confirmar o atalho do boot normal: b2+b3 em 6/8 s deu `8f52bbe305a4`, hash da row 68 (Sand Hill) | P | 4.299 quadros em 75 s, sem `DEBUG_TITLE`; se for Sand Hill, o roteiro de medida de todas as fases encurta |
| Z19 | Twinkle Circuit (row 65) + regressão de Sky Chase/Sand Hill com o `.mod` novo | M | row 65 **16,6%** não-preto e 285 cores (`52455310e0db`); rows 66/67 nunca re-medidas; 68 de 23/11 |
| Z20 | Som: SFX e pitch bend (fora do menu há várias rodadas) | M | 17/179 clipes; `src/sound/sound.c:757`; 259 SFX; áudio ADX e de filme já tocam |

Fora dos 20, ainda abertos: ms no aparelho (sem Zeebo em mão), o aviso de apóstrofo em
`engine/native/prelude.h:18` (cosmético, suja os `.err`), `MISSING_PROGRAMS`/`stg11` (não existe no decomp), a
textura sem cor do Egg Carrier (row 46) e a limpeza de `keep/`/`game_o2/` — destrutiva, só com autorização
explícita (disco a 100%, 3,8 GB).

## Atualização 2026-11-25 13:10 — `.mod` de 26,58 MB: cabe nos 32 MB, e a row 3 desenha com o mapa novo (99,6% não-preto, 7.871 cores)

Rodada do cron (12:55–13:10). `HEAD` = `8d99cc4` (12:19), branch `zeebo-port`, árvore **limpa** (só
`sa1n.bmp` e `todo.md` não rastreados, os de sempre); nenhum `make`/`cc1` de outra sessão rodando durante a
medição. O `.mod` de 26,58 MB é o de `35e7ee9`, e foi medido **desta rodada** — não é número de terceiro.

| Item | Medido nesta rodada | Observação |
|---|---|---|
| `make -C ports/zeebo` | rc=0, `sa1z.elf` 303.528 B | applet feito antes do `mod` (armadilha do zstrip/ztexture/app.c) |
| `make -f game.mk mod` | **752 de 752 objetos**, `duplicatas: 0`, **indefinidos: 19** | nenhum `FALHOU`; 13 `.err` no disco, **0** com `error:` |
| `sa1g.mod` | **26.581.468 B** (25,35 MiB) | **cabe no teto de 26,6 MB** que faltava às 10:54; sha256 `76da00ad4fa109d9` **igual em dois builds** (determinístico); a variante `DEBUG_TITLE=3` sai com 26.581.404 B |
| `.ovl` | **249 = 36.438.952 B** (34,75 MiB) | `gen_ovl: regiao 2.241.160 B` (era 2.242.340 B às 10:50) |
| `host-selftest` | **PASSED (0 failures)** | inclui os testes de `zrebase` |
| `PROGRAMS` | **251** = 47 programas + 204 eventos; `MISSING_PROGRAMS` vazio | `modules/` tem 41 dirs; `movie` fica **fora de propósito** (`game.mk:40` compila `modules/movie/movie.c` direto no ELF, `zmovie.o`); não existe `stg11` |
| Indefinidos | **5** no `sa1g.elf` (`card_drive_object`, `dat_0ca4ec58`, `dat_0ca4f790`, `dat_0ca50678`, `dat_0cb2d478`); **114** dentro dos overlays | recontei hoje: `nm -g -u build/zeebo/game/ovl/program-*.o` menos o definido no ELF = 114 (igual às 10:54: não piorou) |
| Matriz | **16 de 76** linhas (3, 6, 9, 12, 36, 37, 38, 39, 40, 46, 58, 62, 65, 68, 69, 71) | **nenhuma linha nova** desde 10:46; a row 3 foi re-medida com o `.mod` novo (abaixo) |
| Disco | `/` a **100%**, **4,2 GB** livres; `build/zeebo` = **18 GB** | o rebuild desta rodada regravou as 249 cópias de `.ovl` |
| Perf (row 3, zeebx) | 59–61 quadros em 2 vsyncs por janela, **0** acima de 55 ms, 1 acima de 250 ms | ~30 fps estável — compatível com os 20–21 ms/quadro de 04:20 |

### O que foi feito desde a atualização das 10:54 (3 commits, todos de 12:19)

| Commit | O que mudou | Efeito |
|---|---|---|
| `35e7ee9` | `zgdfs.c`, `zrebase.c/h`: `zdc_ram` mapeado **por trechos** (7,34 → 4,45 MB); `advertise`/`summary` também pulam o SH-4; fim de `WORK_AREA` no fim do trecho | `.mod` **30,77 → 26,58 MB** (medido por mim: 26.581.468 B) |
| `ef1df42` | `zhid.c`: o botão 1 do Z-Pad chega com o UID de `Button_2` do cabeçalho — sem a troca o pulo saía junto com o Spin Dash | mapeamento do Z-Pad (1=A) deixa de colidir com o Spin Dash |
| `8d99cc4` | `tools/estado_hash.ref` refeito: a recompilação limpa tirou ~1 MB de `.text` stale | o `.ref` acompanha o código; nenhum código mudou |

**Os três entraram juntos às 12:19** (mesmo `%ai`/`%ci`, 12:19:32) — foram commitados em bloco pela sessão
que editou a árvore a partir das 10:50; o que era "em fluxo" na rodada passada está fechado (era o Z7).

### Medido nesta rodada (o que os números novos dizem)

- **O orçamento de RAM fecha.** Com o `.mod` de 26,58 MB, a row 3 (Emerald Coast) fechou com livre mínimo
  **0xF1D6 = 61.910 KB** de 65.536 KB, ou seja **pico de alocação de 3,54 MB** (o melhor número já medido; era
  5,4 MB de terceiro às 10:54). Pela aritmética que o roadmap vinha usando: 26,58 + 3,54 = **30,1 MB** contra os
  32 MB de heap → **cabe, com ~1,9 MB de folga**. Ressalva honesta: a arena que o `GETRAMFREE` reporta no zeebx é
  de 64 MB e o console é de 30–32 MB; o 3,54 MB é proxy de alocação, não medida de hardware.
- **A row 3 desenha com o mapa novo.** `DEBUG_TITLE=3`, b1 em 14 s/17 s, 60 s: modo `0x4` ACTION, **1.836
  quadros**, hash `60d270dfd772`, **99,6%** não-preto (>24), **7.871 cores**, média RGB 171,4/171,3/182,5 —
  a cena mais rica medida até hoje (contra 10.868 da Windy Valley em 23/11 e 2.994 do Chaos 2). `traps=1` (só o
  stub `cache_ram_init`), `ausente=0`. PNG em `build/zeebo/matrix_r/row3_2026-11-25c.png`.
  **Não é byte-idêntico** ao `8031d98b7314` de 04:20 (1.829 quadros lá, 1.836 aqui): o código mudou desde então
  (`ef1df42`, `.ref` de `8d99cc4`) e a diferença de 7 quadros é de tempo de parede. Não afirmo regressão nem
  igualdade — para isso falta a comparação com o PC (AA7).
- **Correção de rota na receita da matriz: é b1, não b2+b3.** Com `DEBUG_TITLE=3` e **b2+b3** em 6/8 s o run
  **não entra na fase**: 1.439 quadros, hash **`5d38968c25a3`** — exatamente o hash do **título** documentado às
  04:20 —, 10,7% não-preto e 112 cores. Quem entra é **b1 em 14 s/17 s** (`tools/stage_matrix.sh:19` já usa
  `14000:b1,17000:b1`). A tabela de 10:54 diz `KEYS=6000:b2,8000:b3` e chegou a ACTION/FIELD; com o `.mod` de hoje
  não reproduzo isso na row 3. Quem for medir as 60 linhas restantes tem de usar **b1**.
- **Z18 cai: o "atalho do Sand Hill" não se reproduz.** Boot normal (sem `DEBUG_TITLE`), b2+b3 em 6/8 s, 75 s:
  LOGO sai em 3.183 ms, **MOVIE entra em 3.373 ms e o run termina lá** (36 amostras de `mem`, livre nunca abaixo
  de 0xF8C8 = 63.688 KB, nenhum `game_mode` novo). Não houve os 4.299 quadros nem o hash `8f52bbe305a4` de 10:54.
  Ou aquela medida pegou o filme (que é longo) e o hash coincidiu com o da row 68, ou o caminho mudou com o `.mod`
  novo. O que falta para o fluxo normal chegar ao título é **cortar o filme** — é o item AA1.

#### O que falta para o OBJETIVO FINAL (port completo, todas as fases e conteúdo)

Mapa do menu (`src/progress/title.c:384-460`, 76 linhas): 0–2 Practice, 3–5 Emerald Coast, 6–8 Windy Valley,
9–11 Twinkle Park, 12–14 Speed Highway, 15–17 Red Mountain, 18–20 Sky Deck, 21–23 Lost World, 24–27 Ice Cap,
28–31 Casinopolis, 32–34 Final Egg, 35 Chaos 0, 36 Chaos 2, 37 Chaos 4, 38 Egg Viper 1, 39 "Zero", 40–45
Station Square, 46–51 Egg Carrier, 52–57 Egg Carrier por dentro, 58–61 Mystic Ruins, 62–64 Past, 65 Twinkle
Circuit, 66–67 Sky Chase, 68 Sand Hill, 69–72 Chao Garden, 73–75 zeros.

| Conteúdo | Estado em 13:10 | Falta |
|---|---|---|
| **RAM / pacote** | **`.mod` 26,58 MB, pico 3,54 MB → 30,1 MB de 32: cabe** | levar o número ao console; instalador (AA19) |
| Emerald Coast (3–5) | row 3 **re-medida com o `.mod` novo** (`60d270dfd772`, 99,6%, 7.871 cores) | rows 4–5 (atos 2–3) |
| Windy Valley (6–8) | row 6 (`3be948f582a6`, 98,6%, de 10:54, `.mod` velho) | rows 7–8 + re-medir a 6 |
| Twinkle Park (9–11) | row 9 (`09e03f1bc6e2`, 97,5%, idem) | rows 10–11 |
| Speed Highway (12–14) | row 12 (`3a0ac207e776`, 95,8%, idem) | rows 13–14 |
| Red Mountain (15–17), Sky Deck (18–20), Lost World (21–23), Ice Cap (24–27), Casinopolis (28–31), Final Egg (32–34) | 20 linhas, **nenhuma medida** | 20 linhas |
| Hot Shelter (`stg12`) | linkado (`game.mk:165`, `src/stage/stage.c:97`), **sem linha no menu** | linha + medida (AA3) |
| Station Square (40–45) | row 40 (`9f1f67d18d0e`, 99,6%) | 5 linhas (41–45) |
| Egg Carrier (46–51) e por dentro (52–57) | row 46 escura, HUD sem cor (`71b404f1d722`, 86,9% >24, 691 cores) | 11 linhas + a cor (AA17) |
| Mystic Ruins (58–61) | row 58 (`17e45c00ea25`, FIELD, 98,7%, 2.587 cores) | rows 59–61 |
| Past (62–64) | row 62 (`6f20d39ebe77` 2×, FIELD, 99,5%, 3.605 cores) | rows 63–64 + Tikal jogável (AA10) |
| Chefes | Chaos 0 (35) ok; Chaos 2 (36) e Egg Viper 1 (38) claros; Chaos 4 (37) escuro (`70b135d59597`, média 27,50,49); "Zero" (39) 0,9% | corrigir 37/39; Chaos 6/7, EGM2/3, E101, E101-R **sem linha** (AA3) |
| Chao Garden (69–72) | row 69 (`89c67795ce0b`, 99,2%, 2.518 cores) | rows 70–72; 27 `alife_*` + 9 `garden_*` |
| Minijogos | Sky Chase 1/2 e Sand Hill rodam (row 68, `8f52bbe305a4`, 97,8%); Twinkle Circuit (65) **16,6%** | pista do Circuit; regressão 66/67/68 com o `.mod` novo |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1`; **o filme do boot não é cortado por b2+b3** | 448 linhas (`zplm.c:146-153`) + corte do filme (AA1) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes, pitch bend (`src/sound/sound.c:757`), 259 SFX (AA13) |
| Save | cartão emulado + RTC; `buIsReady` (`8189f9b`); `vmu_import.py` | save no campo nunca exercitado; `card_*` 9 símbolos; `CHECK_GAME=1` para em `ava_vmssel` |
| Texto/2D | `tex_names[]` (`37c1974`); 2D e depth do preview ok | texto do aviso do cartão (`present_record`) |

#### 20 quick wins (AA1–AA20, abertos em 25/11 13:10; P ≤ 1 h, M ≤ meio dia)

**Fechados desde a lista Z1–Z20:** **Z7** (mapa do `zdc_ram` commitado em `35e7ee9`, `.mod` 26,58 MB, selftest
verde), **Z8** (o `.mod` chegou a 26,58 MB, abaixo do teto de 26,6), **Z5** (pico medido nesta rodada: 3,54 MB) e
**Z18** (não se reproduz — virou AA1, que ataca a causa). Os demais seguem abertos, renumerados de AA1 a AA20 com
a evidência de hoje.

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| AA1 | Cortar o filme do boot (ou achar a tecla certa) para o fluxo normal chegar ao título e a um New Game | M | boot normal + b2+b3 em 6/8 s fica em **MOVIE** os 75 s (36 amostras `mem`, `game_mode` 0x8 até o fim); `zmovie.c`, `zplm.c` |
| AA2 | Trocar a receita da matriz para **b1** e re-medir as 60 linhas restantes (quadro + log por linha) | M | b2+b3 → hash do título `5d38968c25a3` (10,7%, 112 cores); b1 → ACTION `60d270dfd772` (99,6%, 7.871 cores); `tools/stage_matrix.sh:19` já usa b1 |
| AA3 | 7 linhas no menu de debug: Hot Shelter (`stg12`), CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R | P | `src/progress/title.c:202` (`DEBUG_STAGES 76`), lista em `:384-460`; 0/7; os 7 já em `PROGRAMS` (`game.mk:165-166`) |
| AA4 | `tools/frame_hash.sh:15`: default do `ZEEBX` aponta para `~/projects/zeebx-v060`, que **não existe** | P | `FALHOU: sem quadro (rc=127)`; hoje só `~/projects/zeebx-emu-copytex/target/release/zeebx` (usado nesta rodada) e o AppImage em `~/zeebx-release-latest/` |
| AA5 | Gate por `--frames` em vez de tempo de parede e só então refazer o `.ref` | M | `tools/hash_ref.sh:14` (`--seconds=100`); 3241 × 3088 quadros no mesmo `.mod`; `.ref` refeito em `8d99cc4`, mas o gate continua por tempo |
| AA6 | Logar `zprs_skipped` (o skip do SH-4 agora existe de verdade) | P | `zgdfs.c:129` define, `assets/prs.c:169` incrementa, **zero leitores**; `zinst.c:26` só reporta `zprs_calls` |
| AA7 | Comparar a row 3 nova com o PC antes de declarar Emerald Coast boa | P | `60d270dfd772` ≠ `8031d98b7314` (04:20), 1.836 × 1.829 quadros; 7.871 cores; PNG `matrix_r/row3_2026-11-25c.png` |
| AA8 | Chaos 4 (37) escuro/esverdeado e "Zero" (39) 0,9% não-preto | M | row 37 média RGB **27,50,49** (`70b135d59597`); row 39 **0,9%**; rows 36/38 claras |
| AA9 | 114 símbolos dos overlays + os 5 do ELF | M | recontei hoje: 114 (34 `dat_0c…`, 27 `alife_*`, 9 `garden_*`, 9 `card_*`, 7 `egg*`, 5 `chaos7_*`, 4 `warp_*`, 3 `tikal_*`); nenhum exige engenharia reversa |
| AA10 | Tikal jogável em Mystic Ruins/Past (rows 59–61, 63–64) | M | `tikal_object`, `tikal_player_step` (14 refs), `tikal_walk_action`; row 62 mediu, 59–61/63–64 nunca |
| AA11 | Save no campo: 9 símbolos `card_*` e o texto do aviso | M | `card_drive_object` indefinido no ELF; `CHECK_GAME=1` para em `ava_vmssel`; `vmu_import.py` pronto |
| AA12 | Filmes em 448 linhas inteiras (`step=1`) | M | `ports/zeebo/zplm.c:146-153`: `while height/step > maxh: step++` → 224 de 448 |
| AA13 | Som: SFX e pitch bend | M | 17/179 clipes; `src/sound/sound.c:757`; 259 SFX; áudio ADX e de filme já tocam |
| AA14 | Chao Garden rows 70–72 | M | 27 `alife_*` + 9 `garden_*` indefinidos; row 69 mediu 2.518 cores (contra 10.868) |
| AA15 | Atos 2–3 dos stages já bons: rows 4–5, 7–8, 10–11, 13–14 | M | 8 linhas; só a primeira de cada stage foi medida (3, 6, 9, 12) |
| AA16 | Twinkle Circuit (row 65) + regressão 66/67/68 com o `.mod` de 26,58 MB | M | row 65 **16,6%** não-preto e 285 cores (`52455310e0db`); 66/67 nunca re-medidas; 68 de 23/11 |
| AA17 | Egg Carrier por dentro (rows 47–57) e a cor do HUD da row 46 | M | row 46 `71b404f1d722`, 86,9% >24 mas **691 cores**; 11 linhas sem medida |
| AA18 | Perf: derrubar as rows 3/6/9/12 de 2 vsyncs para 1 | M | row 3: 59–61 quadros em 2 vsyncs (>16,7 ms) por janela, 0 acima de 55 ms; alvo 60 Hz |
| AA19 | Instalador: `.mod` + 249 `.ovl` num pacote que caiba no aparelho | M | 26,58 MB + 34,75 MiB de `.ovl`; `/` a 100% e `build/zeebo` em 18 GB |
| AA20 | Limpar as 20.349 cópias de `.ovl` de `build/zeebo` (2,7 GB) — **destrutiva, só com autorização** | P | `/` a **100%**, **4,2 GB** livres; `build/zeebo` = **18 GB** |

Fora dos 20, ainda abertos: ms no aparelho (sem Zeebo em mão), o aviso de apóstrofo em
`engine/native/prelude.h:18` (cosmético, suja os `.err`), `MISSING_PROGRAMS`/`stg11` (não existe no decomp) e a
limpeza de `keep/`/`game_o2/` — destrutiva (AA20).

## Atualização 2026-11-25 15:10–15:25 — rows 4 e 15 medidas; `.mod` determinístico; a árvore carrega o VMU de 8 cartões sem commit

Rodada do cron (15:10–15:25). `HEAD` = `8d99cc4` (12:19), branch `zeebo-port`; **nenhum commit desde as 13:10**.
A árvore **não está limpa**: a sessão que editou das 13:15 às 13:32 deixou o VMU **de 8 cartões** em
`zshinobi.c` (+330/−110, 687 linhas), tirou `kbCfgGetId`/`kbCfgIni` de `stubnames.txt`, criou
`tools/vmu_build.py` (245 linhas) e `tests/vmu_build_test.py` (5 testes, `OK` em 0,03 s) e reescreveu trechos dos
dois `debito-tecnico-2026-11-25*.md` — **nada commitado**. O `.mod` medido abaixo **inclui** esse código:
`sa1g.elf` define `vmu_cards` e carrega as strings `sa1vmu.bin`/`sa1vmu_a1.bin`.

| Item | Medido nesta rodada | Observação |
|---|---|---|
| `make -C ports/zeebo` | nada a refazer (rc=0) | applet feito às 13:21 pela outra sessão: `sa1z.elf` 303.552 B, `sa1z.mod` 7.745.228 B |
| `make -f game.mk mod` | **752 de 752 objetos**, `duplicatas: 0`, **indefinidos: 19** | nenhum `FALHOU`; 13 `.err` no disco, **0** com `error:`; **88 s** |
| `sa1g.mod` | **26.479.644 B** (25,25 MiB), sha256 `35ced8775f165dd9` | **igual em dois builds** (15:12 e 15:24) → determinístico; 101.824 B menor que o de 13:23 |
| `.ovl` | **249 = 36.439.238 B** (34,75 MiB) | `gen_ovl` regravou as 249 cópias |
| `host-selftest` | **PASSED (0 failures)**, 1,9 s | não cobre o VMU novo (o teste dele é `tests/vmu_build_test.py`, em Python) |
| `PROGRAMS` | **251** = 47 programas + 204 eventos; `MISSING_PROGRAMS` vazio | os 47 não-`ev`: `advertise summary a_mot b_mot e_mot k_mot s_mot m_sbmot s_sbmot adv00 b_chaos0 stg00…stg10 stg12 adv0100 adv0130 adv01obj adv02 adv03 al_garden00..02 al_main al_race b_chaos2 b_chaos4 b_chaos6 b_chaos7 b_e101 b_e101_r b_egm1..3 b_robo minicart sboard shooting tikal_prog` |
| Indefinidos | **5** no ELF (`card_drive_object`, `dat_0ca4ec58`, `dat_0ca4f790`, `dat_0ca50678`, `dat_0cb2d478`); **114** nos overlays | recontei com `nm -A -g -u` + `comm`: 1.427 indefinidos → 114 fora do ELF (igual às 13:10) |
| Matriz | **18 de 76** (3, 4, 6, 9, 12, 15, 36, 37, 38, 39, 40, 46, 58, 62, 65, 68, 69, 71) | **2 linhas novas: 4 e 15** |
| Disco | `/` a **98%**, **9,1 GB** livres; `build/zeebo` = **18 GB** | `game` 8,8 G, `keep` 6,5 G, `game_o2` 1,2 G; **20.598** `.ovl` no disco |

### O que foi feito desde a atualização das 13:10

Nenhum commit. Da outra sessão (13:15–13:32): o VMU passou de um cartão fixo no A1 para **8 cartões** (A1, A2, B1,
B2, C1, C2, D1, D2 = porta×2+encaixe), cada um com ~100 KB do heap do BREW, arquivo `sa1vmu.bin` no A1 e
`sa1vmu_<a-d><1|2>.bin` nos outros; `tools/vmu_build.py` monta cartões a partir de `.VMS` do Bucanero ou de imagem
de 128 KB, e `tests/vmu_build_test.py` cobre o formato (5 testes verdes). Desta rodada: **rows 4 e 15 medidas** (abaixo)
e o `.mod` normal reconstruído duas vezes (determinístico).

### Medido nesta rodada

- **Row 4 (Emerald Coast, ato 2)** — `DEBUG_TITLE=4`, b1 em 14 s/17 s, 75 s: modo **ACTION**, **2.279 quadros**,
  hash **`3ab391199fa6`**, **98,7%** não-preto (>24), **3.288 cores**, média RGB **71,0/87,4/32,8**; `traps=1`
  (`cache_ram_init`), `ausente=0`. PNG/log em `build/zeebo/matrix/row4.{png,log}`.
- **Row 15 (Red Mountain, ato 1)** — `DEBUG_TITLE=15`, b1 em 17 s/20 s, 60 s: **ACTION aos 17.539 ms**, **1.829
  quadros**, hash **`11a5fc3a08dc`**, **97,7%** >24, **3.190 cores**, média **75,7/70,1/36,2**; `traps=1`,
  `ausente=0`. PNG/log em `matrix/row15.{png,log}`.
- **A receita b1 em 14 s/17 s não serve para todas as linhas.** A primeira tentativa da row 15, com b1 em 14 s/17 s,
  **não entrou na fase**: LOGO saiu em 3.183 ms, **MOVIE tocou de 3.373 a 10.650 ms** e o run terminou em
  **MENU_RUN** (aberto em 14.023 ms), 4.177 quadros, hash `8f52bbe305a4`, quadro 97,8% >24 / **6.225 cores** /
  média 183,1/192,0/205,0 (o menu de debug, claro). Com b1 em **17 s/20 s** a fase entrou. Quem for varrer as 58
  linhas restantes: **cronometrar as teclas depois do menu/fase aparecer**, não em tempo fixo.
- **`8f52bbe305a4` não é o Sand Hill.** Esse hash, que o roadmap vinha atribuindo à row 68 (e ao "atalho do Sand
  Hill" de 10:54), é o do **quadro do MENU de debug** nas duas medições de hoje. O log de 23/11 da row 68
  (`matrix/row68.log`, build de 518 objetos) registra **`be11bba75cea`**, outra imagem. A atribuição antiga fica
  **invalidada** até alguém re-medir a 68 com o `.mod` atual (BB16).
- **O mesmo `.mod` não seguiu o mesmo caminho duas vezes.** Duas execuções do build `DEBUG_TITLE=15` divergiram
  **aos 3,4 s, antes de qualquer tecla**: uma foi para **MOVIE** (3.373 ms) e outra para **TITLE** (3.490 ms). Não
  tenho causa medida; registro porque qualquer gate por hash/estado depende de o boot ser reprodutível (BB9/BB10).
  Ressalva honesta: `zprs_skipped` continua sem leitor, então não sei se o skip do SH-4 dispara diferente entre runs.

#### O que falta para o OBJETIVO FINAL (port completo, todas as fases e conteúdo)

| Conteúdo | Estado em 15:25 | Falta |
|---|---|---|
| **RAM / pacote** | `.mod` **26,48 MB**, pico 3,54 MB → **30,0 MB** de 32: cabe com ~2 MB | levar o número ao console; instalador (BB19) |
| Emerald Coast (3–5) | rows **3 e 4** medidas (99,6% / 98,7%) | row 5 (ato 3) |
| Windy Valley (6–8) | row 6 (`3be948f582a6`, 98,6%, `.mod` velho) | rows 7–8 + re-medir a 6 |
| Twinkle Park (9–11) | row 9 (`09e03f1bc6e2`, 97,5%) | rows 10–11 |
| Speed Highway (12–14) | row 12 (`3a0ac207e776`, 95,8%) | rows 13–14 |
| **Red Mountain (15–17)** | row **15** medida hoje (`11a5fc3a08dc`, 97,7%, 3.190 cores) | rows 16–17 |
| Sky Deck (18–20), Lost World (21–23), Ice Cap (24–27), Casinopolis (28–31), Final Egg (32–34) | 20 linhas, **nenhuma medida** | 20 linhas |
| Hot Shelter (`stg12`) | linkado (`game.mk:165`), **sem linha no menu** | linha + medida (BB4) |
| Station Square (40–45) | row 40 (`9f1f67d18d0e`, 99,6%) | rows 41–45 |
| Egg Carrier (46–51) e por dentro (52–57) | row 46 escura, HUD sem cor (`71b404f1d722`, 86,9%, 691 cores) | 11 linhas + a cor (BB17) |
| Mystic Ruins (58–61) | row 58 (`17e45c00ea25`, FIELD, 98,7%, 2.587 cores) | rows 59–61 |
| Past (62–64) | row 62 (`6f20d39ebe77`, FIELD, 99,5%, 3.605 cores) | rows 63–64 + Tikal jogável (BB11) |
| Chefes | Chaos 0 (35) ok; Chaos 2 (36) e Egg Viper 1 (38) claros; Chaos 4 (37) escuro (média 27,50,49); "Zero" (39) 0,9% | corrigir 37/39; Chaos 6/7, EGM2/3, E101, E101-R **sem linha** (BB4) |
| Chao Garden (69–72) | row 69 (`89c67795ce0b`, 99,2%, 2.518 cores) | rows 70–72; 27 `alife_*` + 9 `garden_*` |
| Minijogos | Sky Chase 1/2 e Sand Hill com medida de 23/11 **em dúvida** (BB16); Twinkle Circuit (65) **16,6%** | pista do Circuit; re-medir 66/67/68 |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1`; o filme do boot **toca** (3,4–10,6 s) | 448 linhas (`zplm.c:146-153`) + cortar o filme (BB2) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes, pitch bend (`src/sound/sound.c:757`), 259 SFX (BB13) |
| Save | cartão emulado + RTC; **agora 8 cartões**, `vmu_build.py` pronto | save no campo nunca exercitado; 9 `card_*`; o VMU novo **sem commit nem teste in-game** (BB1/BB12) |
| Texto/2D | `tex_names[]` (`37c1974`); 2D e depth do preview ok | texto do aviso do cartão (`present_record`) |

#### 20 quick wins (BB1–BB20, abertos em 25/11 15:25; P ≤ 1 h, M ≤ meio dia)

**Fechados desde a lista AA:** **AA2 (parcial)** — a varredura recomeçou por b1 e ganhou as rows 4 e 15, mas a
receita de tempo fixo falhou na 15 e restam 58 linhas; **AA15 (parcial)** — o ato 2 da Emerald Coast (row 4) saiu.
Os demais seguem abertos, renumerados.

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| BB1 | Commitar o VMU de 8 cartões (código + ferramenta + teste) antes da próxima medida | P | `zshinobi.c` +330/−110 (687 linhas), `tools/vmu_build.py` 245 linhas, `tests/vmu_build_test.py` 5 testes `OK`; `.mod` de hoje já o contém e nada está commitado desde `8d99cc4` |
| BB2 | Cortar o filme do boot (ou achar a tecla) para o fluxo normal chegar ao título e a um New Game | M | boot normal: LOGO→MOVIE em 3.373 ms e fica lá; row 15 com b1 em 14 s/17 s parou em MENU_RUN; `zmovie.c`, `zplm.c` |
| BB3 | Varredura da matriz: 58 linhas restantes, com teclas cronometradas pelo log (não em tempo fixo) | M | 18/76 medidas; 0–2, 5, 7–8, 10–11, 13–14, 16–17, 18–35, 41–45, 47–57, 59–61, 63–64, 66, 67, 70, 72, 73–75 faltam; `tools/stage_matrix.sh:17` (`KEYS=14000:b1,17000:b1`) |
| BB4 | 7 linhas no menu de debug: Hot Shelter (`stg12`), CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R | P | `src/progress/title.c:202` (`DEBUG_STAGES 76`), lista em `:384-460`; 0/7; os 7 já em `PROGRAMS` (`game.mk:165-166`) |
| BB5 | `tools/frame_hash.sh:15`: default do `ZEEBX` aponta para `~/projects/zeebx-v060`, que **não existe** | P | `FALHOU: sem quadro (rc=127)`; hoje só `~/projects/zeebx-emu-copytex/target/release/zeebx`; **toda** medição desta rodada precisou de `ZEEBX=` na mão |
| BB6 | Gate por `--frames` em vez de tempo de parede e só então refazer o `.ref` | M | `tools/hash_ref.sh:14` (`--seconds=100`); duas execuções do mesmo `.mod` divergiram aos 3,4 s (MOVIE × TITLE) |
| BB7 | Logar `zprs_skipped` (o skip do SH-4 existe de verdade) | P | `zgdfs.c:129` define, `assets/prs.c:163,169` incrementa (extern weak), **zero leitores**; `zinst.c:26` só reporta `zprs_calls` |
| BB8 | Comparar a row 3 (`60d270dfd772`) com o PC antes de declarar Emerald Coast boa | P | 1.836 × 1.829 quadros, 7.871 cores; PNG `matrix_r/row3_2026-11-25c.png`; nada medido contra o decomp nativo |
| BB9 | Achar a causa de o mesmo `.mod` divergir no boot (3,4 s: MOVIE × TITLE) | M | `matrix/row15.log` (MOVIE 3.373–10.650 ms) × retry (TITLE 3.490 ms), mesmo `DEBUG_TITLE=15`, `.mod` sha `35ced877…` |
| BB10 | Chaos 4 (37) escuro/esverdeado e "Zero" (39) 0,9% não-preto | M | row 37 média RGB **27,50,49** (`70b135d59597`); row 39 **0,9%**; rows 36/38 claras |
| BB11 | Tikal jogável em Mystic Ruins/Past (rows 59–61, 63–64) | M | `tikal_object`, `tikal_player_step` (14 refs), `tikal_walk_action`; row 62 mediu, 59–61/63–64 nunca |
| BB12 | Save no campo com o cartão novo: carregar `sa1vmu_a1.bin` e gravar; 9 símbolos `card_*` | M | `card_drive_object` indefinido no ELF; `CHECK_GAME=1` para em `ava_vmssel`; `vmu_build.py build` + `list` prontos, nada exercitado in-game |
| BB13 | Filmes em 448 linhas inteiras (`step=1`) | M | `ports/zeebo/zplm.c:147-152`: `while height/step > maxh: step++` → 224 de 448 |
| BB14 | Som: SFX e pitch bend | M | 17/179 clipes; `src/sound/sound.c:757`; 259 SFX; áudio ADX e de filme já tocam |
| BB15 | Chao Garden rows 70–72 | M | 27 `alife_*` + 9 `garden_*` indefinidos; row 69 mediu 2.518 cores (contra 10.868 da Windy Valley) |
| BB16 | Re-medir Sand Hill (68) e Twinkle Circuit (65) com o `.mod` atual; a atribuição de `8f52bbe305a4` caiu | M | `8f52bbe305a4` = quadro do **MENU** (4.177 quadros, 6.225 cores, média 183/192/205); `matrix/row68.log` diz `be11bba75cea` (build de 518 objetos); row 65 **16,6%**, 285 cores |
| BB17 | Egg Carrier por dentro (rows 47–57) e a cor do HUD da row 46 | M | row 46 `71b404f1d722`, 86,9% >24 mas **691 cores**; 11 linhas sem medida |
| BB18 | Perf: derrubar as rows 3/4/15 de 2 vsyncs para 1 | M | logs de hoje: `modo vsyncs por quadro 0x2`; alvo 60 Hz (row 3: 59–61 quadros/janela) |
| BB19 | Instalador: `.mod` 26,48 MB + 249 `.ovl` 34,75 MiB num pacote que caiba no aparelho | M | `gen_ovl` regrava 36.439.238 B de `.ovl` por stage; `build/zeebo` em 18 GB |
| BB20 | Limpar as **20.598** cópias de `.ovl` de `build/zeebo` (`keep` 6,5 G + `game_o2` 1,2 G) — **destrutiva, só com autorização** | P | `/` a **98%**, **9,1 GB** livres; `build/zeebo` = **18 GB** |

Fora dos 20, ainda abertos: ms no aparelho (sem Zeebo em mão), o aviso de apóstrofo em
`engine/native/prelude.h:18` (cosmético, suja os `.err`), `MISSING_PROGRAMS`/`stg11` (não existe no decomp), os
114 símbolos dos overlays (34 `dat_0c…`, 27 `alife_*`, 9 `garden_*`, 9 `card_*`, 7 `egg*`, 5 `chaos7_*`, 4
`warp_*`, 3 `tikal_*`) e a limpeza de `keep/`/`game_o2/` — destrutiva (BB20).

## Atualização 2026-11-25 17:25 — a Emerald Coast fecha os três atos (row 5) e o MENUFPS30 está no HEAD

Rodada do cron (17:25). `HEAD` = `3cde956` (16:18), branch `zeebo-port`; **3 commits desde as 15:25** e a árvore
limpa (só `sa1n.bmp` e `todo.md` não rastreados). O `.mod` normal foi reconstruído três vezes nesta rodada (duas
delas depois dos builds `DEBUG_TITLE`) para voltar do estágio ao boot e conferir o determinismo.

| Item | Medido nesta rodada | Observação |
|---|---|---|
| `make -C ports/zeebo` | nada a refazer (rc=0) | applet de 16:10: `sa1z.elf` 303.908 B, `sa1z.mod` 7.745.420 B |
| `make -f game.mk mod` | **752 de 752 objetos**, `duplicatas: 0`, **indefinidos: 19** | nenhum `FALHOU` nos `.err` |
| `sa1g.mod` | **26.479.884 B**, sha256 `a765fe7af4fa…` **igual em três builds** (17:27, 17:31, 17:37) | +240 B do `.mod` de 15:25 — é o código do `MENUFPS30` no applet |
| `.ovl` | **249 = 36.439.382 B** (34,74 MiB) | +144 B do de 15:25 |
| `host-selftest` | **PASSED (0 failures)** | não cobre o VMU nem o `MENUFPS30` |
| `PROGRAMS` | **251** = 47 programas + 204 eventos; `MISSING_PROGRAMS` **vazio** | `config/` tem 254 diretórios; só `1st_read`, `loader` e `movie` ficam fora, **por desenho** (`game.mk:40`) |
| Indefinidos | **5** no ELF (`card_drive_object`, `dat_0ca4ec58`, `dat_0ca4f790`, `dat_0ca50678`, `dat_0cb2d478`); **114** nos overlays | `card_drive_object` continua indefinido **depois** do `99ea015` |
| Matriz | **19 de 76** — nova **row 5** (Emerald Coast ato 3) | era 18 |
| Disco | `/` a **100%**, **4,1 GB** livres; `build/zeebo` = **19 GB**, **22.092** cópias de `.ovl` | pior que 15:25 (98%, 9,1 GB) |

### O que foi feito desde as 15:25 (3 commits, `d96831a`..`3cde956`)

- **`99ea015` (15:43) — o VMU de 8 cartões entrou no HEAD, e com ele a porta do cartão**: um `struct vmu_card`
  de 200 blocos × 512 B por drive (A1 = `sa1vmu.bin`, os outros = `sa1vmu_<a-d><1|2>.bin`), pendências por drive
  (`vmu_pending_op`/`vmu_pending_status`), `BUD_ERR_DISK_FULL_` = `0xffffff04` e `vmu_pack`/`vmu_store`;
  `kbCfgIni`/`kbCfgGetId` saíram de `stubnames.txt` (implementados na porta); `tools/vmu_build.py` (245 linhas) e
  `tests/vmu_build_test.py` (5 testes) commitados. `zshinobi.c` +330/−110.
- **`dc680f5` (16:18) — `MENUFPS30`, padrão 1**: nos modos `game_mode` 10/11/16 (os que pedem 1 vsync) o applet
  apresenta um quadro fechado em dois (`sa1z_menu_skip`, `app.c:336`; `zstrip_frame_drop`, `zstrip.c:875`,
  descarta translúcidos e lotes 3D); a lógica do jogo segue a 60 Hz. Flags em `Makefile:89` e `tools/build.sh:17`;
  o make não rastreia a flag, `build.sh` a grava em `.flags`.
- **`3cde956` (16:18)** documentou o `MENUFPS30` no handoff, com a medida do autor (**5.077 → 2.801 quadros
  apresentados** em 90 s virtuais, mesmas teclas) e os gates verdes (752/752, host-selftest, novfp, `hash_ref`
  IGUAL). **Não medido: ms de CPU poupados no console.**

### Medido nesta rodada

- **Row 5 (Emerald Coast, ato 3)** — `DEBUG_TITLE=5`, b1 em 14 s/17 s, 75 s: **ACTION aos 14.539 ms**, **2.287
  quadros**, hash **`c85b44278fbf`**, **99,5%** não-preto (>24), **6.441 cores**, média RGB
  **150,8/169,4/172,1**; `traps=1` (stub `cache_ram_init`), `ausente=0`. PNG/log em
  `build/zeebo/matrix/row5.{png,log}`. Com isso a **Emerald Coast tem os três atos medidos** (rows 3, 4 e 5).
- **Row 3 re-medida com o `HEAD`** — 2.286 quadros, `1a7fb384ffa8`, **99,6%** >24, **7.945 cores**, média
  **171,4/171,3/182,5**. O hash **não é comparável** ao de 13:07 (`60d270dfd772`): aquele run tinha 678 quadros
  (17 s) e este 2.286 (75 s), então o quadro final é outro instante da animação; o que dá para afirmar é que a
  imagem ficou no mesmo lugar (mesma média RGB, 99,6% não-preto, 7.945 contra 7.871 cores) — **não** é regressão.
- **O boot foi reprodutível duas vezes hoje** — row 5 e row 3, runs separados: LOGO 190→3.300 ms (191 quadros),
  TITLE 3.490→11.049 ms (638), ACTION **14.539 ms** nas duas. O caso MOVIE × TITLE das 15:10 **não se repetiu**;
  a divergência continua sem causa (CC9), agora com 2 × 2 medidas a favor do caminho TITLE.
- **1 vsync por quadro no estágio**: nos dois logs a sequência é `vsyncs por quadro 0x0` (boot), `0x2` (filme/
  título) e **`0x1`** no fim. É a primeira vez que a linha aparece nas medições; o alvo do BB18 (60 Hz) parece
  alcançado no emulador, falta medir quadros/s e confirmar em hardware.
- `card_drive_object` segue **indefinido** no ELF (`nm -u build/zeebo/sa1g.elf` → 5 símbolos) mesmo com o cartão
  por drive commitado; o save no campo continua sem prova in-game (CC12).

#### O que falta para o OBJETIVO FINAL (port completo, todo o conteúdo)

| Conteúdo | Estado em 17:25 | Falta |
|---|---|---|
| **RAM / pacote** | `.mod` **26,48 MB**, pico proxy 3,54 MB → **30,0 MB** de 32: cabe com ~2 MB | instalador (CC19); disco do PC a **100%** |
| Emerald Coast (3–5) | rows **3, 4 e 5** medidas (99,6% / 98,7% / 99,5%) | comparar a row 3 com o PC (CC8) |
| Windy Valley (6–8) | row 6 (`3be948f582a6`, 98,6%, build de 518 objetos) | rows 7–8 + re-medir a 6 |
| Twinkle Park (9–11) | row 9 (`09e03f1bc6e2`, 97,5%, build antigo) | rows 10–11 + re-medir |
| Speed Highway (12–14) | row 12 (`3a0ac207e776`, 95,8%, build antigo) | rows 13–14 + re-medir |
| Red Mountain (15–17) | row **15** (`11a5fc3a08dc`, 97,7%, 3.190 cores) | rows 16–17 |
| Sky Deck (18–20) | **nenhuma** das 3 linhas medida com o build atual | 3 linhas |
| Lost World (21–23) | só a row 21 rodou — e parou no **menu** (1.438 quadros, `c8d000b7037c`) | 3 linhas de verdade |
| Ice Cap (24–27) | **nenhuma** medida com o build atual | 4 linhas |
| Casinopolis (28–31) | **nenhuma** medida com o build atual | 4 linhas |
| Final Egg (32–34) | **nenhuma** medida com o build atual | 3 linhas |
| Hot Shelter (`stg12`) | linkado (`game.mk:165`), **sem linha no menu** | linha + medida (CC4) |
| Station Square (40–45) | row 40 (`9f1f67d18d0e`, 99,6%, build antigo) | rows 41–45 |
| Egg Carrier (46–51) e por dentro (52–57) | row 46 escura, HUD sem cor (`71b404f1d722`, 86,9%, 691 cores) | 11 linhas + a cor (CC17) |
| Mystic Ruins (58–61) | row 58 (`17e45c00ea25`, FIELD, 98,7%, 2.587 cores) | rows 59–61 |
| Past (62–64) | row 62 (`6f20d39ebe77`, FIELD, 99,5%, 3.605 cores) | rows 63–64 + Tikal jogável (CC11) |
| Chefes | Chaos 0 (35) ok; Chaos 2 (36) e Egg Viper 1 (38) claros; Chaos 4 (37) escuro (média 27,50,49); "Zero" (39) 0,9% | corrigir 37/39; Chaos 6/7, EGM2/3, E101, E101-R **sem linha** (CC4) |
| Chao Garden (69–72) | row 69 (`89c67795ce0b`, 99,2%, 2.518 cores) | rows 70–72; 27 `alife_*` + 9 `garden_*` (CC15) |
| Sand Hill (68) | medida de 23/11 **em dúvida**: `matrix/row68.log` (518 objetos) diz `be11bba75cea`; `8f52bbe305a4` é o **menu** | re-medir com o build de 752 (CC16) |
| Twinkle Circuit (65) | **16,6%** não-preto, 285 cores | pista que desenhe (CC16) |
| Sky Chase 1/2 (66/67) | medida de 23/11, build de 518 objetos | re-medir (CC2) |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1`; o filme do boot **toca** (3,3–11,0 s) e **atravessa o título** | 448 linhas (`zplm.c:146-153`, CC13) + cortar o filme (CC3) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes, pitch bend (`src/sound/sound.c:757`), 259 SFX (CC14) |
| Save | **VMU de 8 cartões no HEAD** (`99ea015`), `vmu_build.py` + 5 testes | save no campo nunca exercitado; `card_drive_object` ainda indefinido (CC12) |
| Texto/2D | `tex_names[]` (`37c1974`); 2D e depth do preview ok | texto do aviso do cartão (`present_record`) |

#### 20 quick wins (CC1–CC20, abertos em 25/11 17:25; P ≤ 1 h, M ≤ meio dia)

**Fechado desde a lista BB:** **BB1** — o VMU de 8 cartões, `vmu_build.py` e os testes entraram em `99ea015`, e
`DISK_FULL`/`kbCfg*` vieram junto. **BB18 tem medida nova**: os logs terminam em 1 vsync por quadro no estágio.
**BB9 enfraqueceu**: duas duplas de runs (row 3 e row 5) seguiram o mesmo caminho LOGO→TITLE→ACTION. Os demais
seguem abertos, renumerados.

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| CC1 | Varredura da matriz: **57 linhas** restantes, com teclas cronometradas pelo log (não em tempo fixo) | M | 19/76 (3, 4, 5, 6, 9, 12, 15, 36, 37, 38, 39, 40, 46, 58, 62, 65, 68, 69, 71); faltam 0–2, 7–8, 10–11, 13–14, 16–35, 41–45, 47–57, 59–61, 63–64, 66, 67, 70, 72–75; `tools/stage_matrix.sh:17` |
| CC2 | Re-medir com o `.mod` de 752 objetos as linhas medidas em builds antigos (518/542) | M | `matrix/summary.txt` (linhas "518 de 518") × `matrix_r/*.log` (752); rows 6, 9, 12, 21, 36, 37, 38, 39, 40, 46, 58, 62, 65, 68, 69, 71 |
| CC3 | Cortar o filme do boot (ou achar a tecla) para o fluxo normal chegar ao título e a um New Game | M | boot: LOGO 190→3.300 ms, **TITLE 3.490→11.049** (o filme toca dentro do título), `zmovie.c`, `zplm.c` |
| CC4 | 7 linhas no menu de debug: Hot Shelter (`stg12`), CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R | P | `src/progress/title.c:202` (`DEBUG_STAGES 76`), lista em `:384-460`; 0/7; os 7 já em `PROGRAMS` (`game.mk:165-166`) |
| CC5 | `tools/frame_hash.sh:15`: default do `ZEEBX` aponta para `~/projects/zeebx-v060`, que **não existe** | P | `ls` do caminho falha; só há `~/projects/zeebx-emu-copytex/target/release/zeebx`; **toda** medição de hoje precisou de `ZEEBX=` na mão |
| CC6 | Gate por `--frames` em vez de tempo de parede; só então refazer o `.ref` | M | `tools/hash_ref.sh:14` (`--seconds=100`); a row 3 de 13:07 tinha 678 quadros e a de hoje 2.286 — hashes de quadros diferentes |
| CC7 | Logar `zprs_skipped` (o skip do SH-4 existe de verdade?) | P | `zgdfs.c:129` define; **zero leitores** no `ports/zeebo` (`grep -rn zprs_skipped ports/zeebo/*.c` → só a definição); `zinst.c:26` reporta só `zprs_calls` |
| CC8 | Comparar a row 3 com o PC (decomp nativo) antes de declarar a Emerald Coast boa | P | PNG de hoje 99,6% >24, 7.945 cores, média 171,4/171,3/182,5; nada medido contra o decomp |
| CC9 | Achar a causa da divergência de boot MOVIE × TITLE | M | `matrix/row15.log` (MOVIE 3.373–10.650 ms) × retry (TITLE 3.490 ms); hoje os dois runs novos (row 3 e row 5) foram TITLE nos mesmos 3.490 ms |
| CC10 | Chaos 4 (37) escuro/esverdeado e "Zero" (39) 0,9% não-preto | M | row 37 média RGB **27,50,49** (`70b135d59597`); row 39 **0,9%**; rows 36/38 claras |
| CC11 | Tikal jogável em Mystic Ruins/Past (rows 59–61, 63–64) | M | `tikal_object`, `tikal_player_step` (14 refs), `tikal_walk_action`; row 62 mediu, 59–61/63–64 nunca |
| CC12 | Save no campo com o cartão novo: `card_drive_object` ainda **indefinido** no ELF | M | `nm -u build/zeebo/sa1g.elf` → `card_drive_object` + 4 `dat_*`; `99ea015` implementou o cartão por drive, mas o símbolo do objeto do save não veio |
| CC13 | Filmes em 448 linhas inteiras (`step=1`) | M | `ports/zeebo/zplm.c:147-152`: `while height/step > maxh: step++` → 224 de 448 |
| CC14 | Som: SFX e pitch bend | M | 17/179 clipes; `src/sound/sound.c:757`; 259 SFX; áudio ADX e de filme já tocam |
| CC15 | Chao Garden rows 70–72 | M | 27 `alife_*` + 9 `garden_*` indefinidos; row 69 mediu 2.518 cores (contra 10.868 da Windy Valley) |
| CC16 | Re-medir Sand Hill (68) e Twinkle Circuit (65) com o `.mod` atual | M | row 68: `matrix/row68.log` (518 objetos) diz `be11bba75cea`; `8f52bbe305a4` = **MENU** (4.177 quadros); row 65 **16,6%**, 285 cores |
| CC17 | Egg Carrier por dentro (rows 47–57) e a cor do HUD da row 46 | M | row 46 `71b404f1d722`, 86,9% >24 mas **691 cores**; 11 linhas sem medida |
| CC18 | Perf: fixar **1 vsync** nas rows 3/4/15 e medir o que o `MENUFPS30` poupa | M | logs de hoje terminam em `vsyncs por quadro 0x1` (0 no boot, 2 no filme/título), mas o BB18 vinha de logs com `0x2`; `MENUFPS30` tem só contagem de quadros (5.077 → 2.801), nenhum ms |
| CC19 | Instalador: `.mod` 26,48 MB + 249 `.ovl` 34,74 MiB num pacote que caiba no aparelho | M | `gen_ovl` regrava 36.439.382 B de `.ovl` por stage; `build/zeebo` em **19 GB** |
| CC20 | Limpar as **22.092** cópias de `.ovl` de `build/zeebo` — **destrutiva, só com autorização** | P | `/` a **100%**, **4,1 GB** livres; `build/zeebo` = **19 GB** (era 18 GB às 15:25) |

Fora dos 20, ainda abertos: ms no aparelho (sem Zeebo em mão), o aviso de apóstrofo em
`engine/native/prelude.h:18` (cosmético, suja os `.err`), `stg11` (não existe no decomp) e os 114 símbolos dos
overlays (34 `dat_0c…`, 27 `alife_*`, 9 `garden_*`, 9 `card_*`, 7 `egg*`, 5 `chaos7_*`, 4 `warp_*`, 3 `tikal_*`).

## Atualização 2026-11-25 19:43 — build determinístico no 4º sha, `host-selftest` verde e o gate `hash_ref` **vermelho** (referência de 12:19, antes do `MENUFPS30`)

Rodada do cron (19:39–19:43). `HEAD` = **`914306f`** (17:38, o commit de docs da rodada anterior), branch
`zeebo-port`; **nenhum commit de código desde as 16:18** (`3cde956`). Árvore limpa (só `sa1n.bmp` e `todo.md`
não rastreados, ambos de sessões antigas). Esta rodada não mudou código: reconstruiu o `.mod` para conferir
determinismo e **achou um gate vermelho na árvore commitada** — a doc do `MENUFPS30` afirma `hash_ref` IGUAL
(`3cde956`), e a medição de agora, no mesmo binário, dá DIFERENTE.

| Item | Medido nesta rodada (19:39–19:43) | Observação |
|---|---|---|
| `make -C ports/zeebo` | nada a refazer (rc=0) | applet de 16:10: `sa1z.elf` 303.908 B, `sa1z.mod` 7.745.420 B |
| `make -f game.mk mod` | **752 de 752 objetos**, `duplicatas: 0`, **indefinidos: 19** | 13 `.err`, **0** com `error:`/`FALHOU` |
| `sa1g.mod` | **26.479.884 B**, sha256 `a765fe7af4fa…` **igual ao de 17:27/17:31/17:37** | 4º build idêntico: determinismo confirmado de novo |
| `.ovl` | **249 = 36.439.382 B** (34,74 MiB) | inalterado desde as 15:25 |
| `host-selftest` | **PASSED (0 failures)** | cobre blend/matriz, não o VMU |
| **`hash_ref`** | **DIFERENTE** — **3.100** quadros apresentados contra **3.036** da referência; 58 `estado hash` contra 47; os 3 primeiros batem (`0xf03ecc58`, `0x5ed4d058`, `0x710a83b5`) e o **4º diverge** (`0xe4064257` × `0x16ce3a75`) | **determinístico**: duas execuções seguidas, saída byte a byte igual |
| `PROGRAMS` | **251** = 47 programas + 204 eventos; `MISSING_PROGRAMS` **vazio** | `config/` tem 254 diretórios |
| Indefinidos | **5** no ELF (`card_drive_object` + 4 `dat_0c…`) | os mesmos das 17:25 |
| Matriz | **19 de 76** (sem linha nova) | rows 3, 4, 5, 6, 9, 12, 15, 36, 37, 38, 39, 40, 46, 58, 62, 65, 68, 69, 71 |
| Disco | `/` em **100%**, **4,3 GB** livres; `build/zeebo` = **19 GB**, **22.092** cópias de `.ovl` | 4,1 → 4,3 GB desde as 17:25 |

### O que foi feito desde as 17:25 (nenhum commit de código)

- **`914306f` (17:38) — docs**: o commit da rodada anterior (roadmap de 17:25). É o único commit na janela.
- **Verificação de determinismo**: `make mod` reconstruiu `sa1g.mod` às 19:41 e o sha256 saiu **idêntico** ao
  dos três builds de 17:27–17:37; nenhum `.err` com erro. O caminho de build está reprodutível com o `HEAD`.
- **`host-selftest` de novo verde** (`PASSED (0 failures)`), como às 17:25.

### Medido nesta rodada — o gate `hash_ref` está vermelho, e não é ruído

`./tools/hash_ref.sh check` (`ports/zeebo/tools/hash_ref.sh:14`, comparação em `:19-25`) roda o `sa1g.mod` atual 100 s virtuais com
entrada fixa (`14000:down,14500:down,15000:down,20000:b1`) e compara os `estado hash` com
`tools/estado_hash.ref`. Resultado de hoje: **DIFERENTE**.

- **Duas execuções novas, byte a byte iguais** (`3100 frames presented`, 58 hashes) — não é flakiness do
  emulador; é o binário que mudou em relação à referência.
- **A referência é de 12:19** (`8d99cc4`), ou seja, **antes** do `MENUFPS30` (`dc680f5`, 16:18). E o
  `MENUFPS30` entra no `sa1g.mod`: o alvo `mod` linka `$(APPLET_OBJS)` (`game.mk:420-427`), então `app.c` está
  no ELF e `zstrip_frame_drop` sai como símbolo global em `sa1g.elf` (`arm-none-eabi-nm` → `T 1d3c`). O
  `estado_hash.ref` versionado nunca foi regravado depois do `dc680f5`.
- O **+64 quadros** (3.036 → 3.100) é coerente com o menu apresentando um quadro em dois: em 100 s virtuais a
  taxa efetiva sobe de ~30,4 para ~31,0 quadros/s, e o `estado hash` passa a ser amostrado em outros quadros.
- **Correção da doc anterior**: o "`hash_ref` IGUAL" de `3cde956` não se reproduz na árvore commitada. Ou foi
  medido com `MENUFPS30=0` (que também não é o padrão de `Makefile:89`), ou com uma referência local não
  commitada. Regra do próprio projeto (`todo.md`, sessão de 24/11): *quando o pacing muda, `hash_ref.sh update`
  no mesmo commit* — o `dc680f5` não fez isso.

### O que falta para o OBJETIVO FINAL (port completo, todo o conteúdo)

| Conteúdo | Estado em 19:43 | Falta |
|---|---|---|
| **Gate de desenho** | **VERMELHO**: `hash_ref` DIFERENTE (3.100 × 3.036 quadros, 4º hash diverge), determinístico | regravar a referência **ou** decidir `MENUFPS30=0/1` e regravar junto (DD3) |
| **RAM / pacote** | `.mod` **26,48 MB** + 249 `.ovl` **36,44 MB**; pico proxy 3,54 MB → **30,0 MB** de 32 | instalador num pacote que caiba no aparelho (DD19); disco do PC a **100%** |
| Emerald Coast (3–5) | rows **3, 4 e 5** medidas (99,6% / 98,7% / 99,5% não-preto) | comparar com o decomp/PC (DD9) |
| Windy Valley (6–8) | row 6 (`3be948f582a6`, 98,6%, build de 518 objetos) | rows 7–8 + re-medir a 6 (DD2) |
| Twinkle Park (9–11) | row 9 (`09e03f1bc6e2`, 97,5%, build antigo) | rows 10–11 + re-medir (DD2) |
| Speed Highway (12–14) | row 12 (`3a0ac207e776`, 95,8%, build antigo) | rows 13–14 + re-medir (DD2) |
| Red Mountain (15–17) | row **15** (`11a5fc3a08dc`, 97,7%, 3.190 cores) | rows 16–17 (DD1) |
| Sky Deck (18–20) | **nenhuma** medida com o build atual | 3 linhas (DD1) |
| Lost World (21–23) | só a row 21 rodou e parou no **menu** (`c8d000b7037c`) | 3 linhas de verdade (DD1) |
| Ice Cap (24–27) | **nenhuma** com o build atual | 4 linhas (DD1) |
| Casinopolis (28–31) | **nenhuma** com o build atual | 4 linhas (DD1) |
| Final Egg (32–34) | **nenhuma** com o build atual | 3 linhas (DD1) |
| Hot Shelter (`stg12`) | linkado (`game.mk:165`), **sem linha no menu** | linha + medida (DD5) |
| Station Square (40–45) | row 40 (`9f1f67d18d0e`, 99,6%, build antigo) | rows 41–45 (DD1/DD2) |
| Egg Carrier (46–51) e por dentro (52–57) | row 46 escura, HUD sem cor (`71b404f1d722`, 86,9%, 691 cores) | 11 linhas + a cor (DD18) |
| Mystic Ruins (58–61) | row 58 (`17e45c00ea25`, FIELD, 98,7%, 2.587 cores) | rows 59–61 (DD12: Tikal) |
| Past (62–64) | row 62 (`6f20d39ebe77`, FIELD, 99,5%, 3.605 cores) | rows 63–64 + Tikal jogável (DD12) |
| Chefes | Chaos 0 (35) ok; Chaos 2 (36) e Egg Viper 1 (38) claros; Chaos 4 (37) escuro (média 27,50,49); "Zero" (39) 0,9% | corrigir 37/39 (DD11); Chaos 6/7, EGM2/3, E101, E101-R **sem linha** (DD5) |
| Chao Garden (69–72) | row 69 (`89c67795ce0b`, 99,2%, 2.518 cores) | rows 70–72; 27 `alife_*` + 9 `garden_*` (DD16) |
| Sand Hill (68) | medida de 23/11 **em dúvida**: `matrix/row68.log` (518 objetos) diz `be11bba75cea`; `8f52bbe305a4` é o **menu** | re-medir com o build de 752 (DD17) |
| Twinkle Circuit (65) | **16,6%** não-preto, 285 cores | pista que desenhe (DD17) |
| Sky Chase 1/2 (66/67) | medida de 23/11, build de 518 objetos | re-medir (DD2) |
| Mini-jogos / Chao Races | `al_race`, `minicart`, `sboard`, `shooting` linkados (`game.mk:166`) | nenhuma linha de menu medida |
| Filmes (SFD) | imagem decodifica; som só com `AUDIO=1`; o filme do boot toca (3,3–11,0 s) e **atravessa o título** | 448 linhas (`zplm.c:146-152`, DD14) + cortar o filme (DD4) |
| Som | música e áudio de filme tocam; efeitos por clipe | 17/179 clipes, pitch bend (`src/sound/sound.c:757`), 259 SFX (DD15) |
| Save (VMU) | **VMU de 8 cartões no HEAD** (`99ea015`), `vmu_build.py` + 5 testes | save no campo nunca exercitado; `card_drive_object` ainda indefinido (DD13) |
| Texto/2D | `tex_names[]` (`37c1974`); 2D e depth do preview ok | texto do aviso do cartão (`present_record`) |

#### 20 quick wins (DD1–DD20, abertos em 25/11 19:43; P ≤ 1 h, M ≤ meio dia)

**Fechados e fora da lista:** o **VMU de 8 cartões** (`99ea015`, com `DISK_FULL` e `kbCfgIni/kbCfgGetId`),
o **`MENUFPS30`** (`dc680f5`) com a doc de `3cde956`, e o **1 vsync por quadro no estágio** medido nos logs de
17:25 (era BB18; falta só confirmar quadros/s e o aparelho). **Nada foi fechado nesta janela de 2 h** — não
houve commit de código. A lista anterior (CC1–CC20) continua a base; os itens que mudaram de evidência estão
reescritos abaixo, com a linhagem indicada.

| # | Entrega | Esf. | Evidência |
|---|---|---|---|
| DD1 | Varredura da matriz: **57 linhas** restantes, com teclas cronometradas pelo log | M | 19/76 (3, 4, 5, 6, 9, 12, 15, 36–40, 46, 58, 62, 65, 68, 69, 71); 75 s de run + 88 s de `make mod` por linha ⇒ não cabe em uma rodada; `tools/stage_matrix.sh:17` |
| DD2 | Re-medir com o `.mod` de 752 objetos as linhas que a tabela marca como build antigo | M | `matrix/summary.txt` linhas 1–29 são de builds de 518/542 objetos e **17** delas carregam o hash do menu `be11bba75cea`; re-medir 6, 9, 12, 40, 66, 67 (e 65/68, DD17); `matrix_r/*.log` é o build novo |
| DD3 | **Refazer o `estado_hash.ref`** e decidir a política do `MENUFPS30` (1 ou 0) no mesmo commit | P | `hash_ref.sh check` → DIFERENTE: 3.100 × 3.036 quadros, 3 hashes iguais e o 4º diverge; ref de `8d99cc4` (12:19) < `dc680f5` (16:18); 2 runs idênticos |
| DD4 | Cortar o filme do boot (ou achar a tecla) para o fluxo normal chegar ao título e a um New Game | M | boot: LOGO 190→3.300 ms, **TITLE 3.490→11.049** (o filme toca dentro do título), `zmovie.c`, `zplm.c` |
| DD5 | 7 linhas no menu de debug: Hot Shelter (`stg12`), CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R | P | `src/progress/title.c:202` (`DEBUG_STAGES 76`), lista em `:384-460`; 0/7; os 7 já em `PROGRAMS` (`game.mk:165-166`) |
| DD6 | `tools/frame_hash.sh:15`: default do `ZEEBX` aponta para `~/projects/zeebx-v060`, que **não existe** | P | caminho inexistente (conferido 19:40); só há `~/projects/zeebx-emu-copytex/target/release/zeebx`; toda medição precisa de `ZEEBX=` na mão |
| DD7 | Gate por `--frames` em vez de tempo de parede; só então confiar no `.ref` | M | `tools/hash_ref.sh:16` (`--seconds=100`); a row 3 de 13:07 tinha 678 quadros e a de hoje 2.286 — hashes de instantes diferentes |
| DD8 | Logar `zprs_skipped` (o skip do SH-4 existe de verdade?) | P | `zgdfs.c:129` define; **zero leitores** (`grep -rn zprs_skipped ports/zeebo/*.c` → só a definição); `zinst.c:26` reporta só `zprs_calls` |
| DD9 | Comparar a row 3 com o PC (decomp nativo) antes de declarar a Emerald Coast boa | P | PNG de 17:25: 99,6% >24, 7.945 cores, média 171,4/171,3/182,5; nada medido contra o decomp |
| DD10 | Achar a causa da divergência de boot MOVIE × TITLE | M | `matrix/row15.log` (MOVIE 3.373–10.650 ms) × retry (TITLE 3.490 ms); os dois runs de 17:25 foram TITLE |
| DD11 | Chaos 4 (37) escuro/esverdeado e "Zero" (39) 0,9% não-preto | M | row 37 média RGB **27,50,49** (`70b135d59597`); row 39 **0,9%**; rows 36/38 claras |
| DD12 | Tikal jogável em Mystic Ruins/Past (rows 59–61, 63–64) | M | `tikal_object`, `tikal_player_step` (14 refs), `tikal_walk_action`; row 62 mediu, 59–61/63–64 nunca |
| DD13 | Save no campo: `card_drive_object` ainda **indefinido** no ELF | M | `nm -u build/zeebo/sa1g.elf` → `card_drive_object` + 4 `dat_*`; `99ea015` trouxe o cartão por drive, não o objeto do save |
| DD14 | Filmes em 448 linhas inteiras (`step=1`) | M | `ports/zeebo/zplm.c:146-152`: `while height/step > maxh: step++` → 224 de 448 |
| DD15 | Som: SFX e pitch bend | M | 17/179 clipes; `src/sound/sound.c:757`; 259 SFX; áudio ADX e de filme já tocam |
| DD16 | Chao Garden rows 70–72 | M | 27 `alife_*` + 9 `garden_*` indefinidos; row 69 mediu 2.518 cores (contra 10.868 da Windy Valley) |
| DD17 | Re-medir Sand Hill (68) e Twinkle Circuit (65) com o `.mod` atual | M | `matrix/row68.log` (518 objetos) diz `be11bba75cea`; `8f52bbe305a4` = **MENU**; row 65 **16,6%**, 285 cores |
| DD18 | Egg Carrier por dentro (rows 47–57) e a cor do HUD da row 46 | M | row 46 `71b404f1d722`, 86,9% >24 mas **691 cores**; 11 linhas sem medida |
| DD19 | Instalador: `.mod` 26,48 MB + 249 `.ovl` 34,74 MiB num pacote que caiba no aparelho | M | `gen_ovl` regrava 36.439.382 B de `.ovl` por stage; `build/zeebo` em **19 GB** |
| DD20 | Limpar as **22.092** cópias de `.ovl` de `build/zeebo` — **destrutiva, só com autorização** | P | `/` a **100%**, **4,3 GB** livres; `build/zeebo` = **19 GB**; `find build/zeebo -name '*.ovl' | wc -l` = 22.092 |

Fora dos 20: ms no aparelho (sem Zeebo em mão), o aviso de apóstrofo em `engine/native/prelude.h:18`
(cosmético), `stg11` (não existe no decomp), os 114 símbolos dos overlays (34 `dat_0c…`, 27 `alife_*`,
9 `garden_*`, 9 `card_*`, 7 `egg*`, 5 `chaos7_*`, 4 `warp_*`, 3 `tikal_*`) e a limpeza do `todo.md` de 24/11
(bug de textura do pause/sol e o crash ao vencer o Chaos 0, ambos sem log novo).
