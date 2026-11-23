# Roadmap: do estado atual até o título e o primeiro stage

Complementa `zeebo-port-roadmap.md` (estado, evidências, riscos). Este arquivo
é a **ordem de execução**: fases, entregas, gates e dependências. Estimativas
são de esforço de trabalho focado, não de calendário, e valem só como ordem de
grandeza (o que não foi medido está marcado).

Estado (2026-11-22, manhã; superado, ver a atualização de 2026-11-23 18:50 abaixo): o jogo (1ST_READ do decomp em ARM32) roda no
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
| Programas/eventos no `.mod` | 244 | 40 módulos + 204 eventos; `MISSING_PROGRAMS` = 0 (16:46) |
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
| Hot Shelter | stg12 | linkado; **não tem linha no menu de debug** (`title.c:384-459`) |
| Hedgehog Hammer | stg00 | rodou (row 0) |
| Station Square (6 áreas) | adv00 | linkado; row 40 rodou em 13:2x (câmera num pilar, sem HUD); rows 41-45 não medidas |
| Mystic Ruins (4 áreas) | adv02 + tikal_prog | linkado; rows 58-61 nunca rodaram com os programas linkados |
| Egg Carrier (6 áreas) | adv01obj + adv0100 | linkado; row 46 não reseta mais, sai **sem HUD**; rows 47-51 não medidas |
| Egg Carrier por dentro (6) | adv01obj + adv0130 | linkado; rows 52-57 não medidas |
| Past (3 áreas) | adv03 + tikal_prog | linkado; rows 62-64 não medidas |
| Chaos 0 / 2 / 4 | b_chaos0 / b_chaos2 / b_chaos4 | Chaos 0 rodou (row 35); row 37 (Chaos 2/4) sai **lavada em verde**; re-medir |
| Chaos 6 / Perfect Chaos | b_chaos6 / b_chaos7 | linkados; **sem linha no menu** |
| Egg Viper 1-3 | b_egm1 / b_egm2 / b_egm3 | linkados; 1 tem linha (row 38), 2 e 3 **sem linha no menu** |
| Zero (robô do Eggman) | b_robo | linkado; **preto com HUD**: `SimpleCalcModelMatrix` em armadilha (row 39) |
| E101 / E101-R | b_e101 / b_e101_r | linkados; **sem linha no menu** |
| Chao Garden | al_main + al_garden00-02 | linkados; rows 69-70 nunca rodaram; row 71 com **céu preto** |
| Corrida de Chao | al_race | linkado; row 72 não medida |
| Twinkle Circuit | minicart | linkado; row 65 **preta com HUD** (`SimpleCalcModelMatrix`) |
| Sky Chase 1 e 2 | shooting | **rodam** (`941fd33`; rows 66-67), sem log de matriz no laptop |
| Sand Hill / snowboard | sboard | **roda**: row 68, 1206 quadros, `8f52bbe305a4`, `ausente=0` (16:34) |
| Filmes (SFD) | zmovie.c + pl_mpeg | imagem decodifica (log `movie slh448_1.sfd`, 22/11); som do filme só com `AUDIO=1` |
| Som | zaudio.c | música 40 s sem laço (`zaudio.c:315`); efeitos e vozes são stubs (`sd_*` = 0) |
| Controle | zhid.c + zboom.c | IHID: 16 botões, 4 eixos e gatilhos somados ao `EVT_KEY` (`5f70e0d`); Boomerang na árvore (teste de PC ok, zeebx pendente) |
| Relógio | zsys.c | RTC real por `GETJULIANDATE` (`b7865dd`); id de Chao e data de save deixam de sair da pilha |
| Save | bu* / `sa1vmu.bin` | cartão emulado; save no campo não exercitado (a data do save agora vem do RTC) |
| Desempenho | — | ~80 ms de jogo por quadro no Emerald Coast (~12 fps) |
| Pacote | — | `.mod` de 36,45 MB (34,8 MiB) + 32,1 MB de `.ovl`; **~6 MB acima** do orçamento de 30-32 MB; nada instalável |

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

#### 20 quick wins (abertos em 23/11 18:50; P <= 1 h, M <= meio dia)

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

## Atualização 2026-11-23 (noite) — 20 quick wins vigentes

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

