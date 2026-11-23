# Roadmap: do estado atual até o título e o primeiro stage

Complementa `zeebo-port-roadmap.md` (estado, evidências, riscos). Este arquivo
é a **ordem de execução**: fases, entregas, gates e dependências. Estimativas
são de esforço de trabalho focado, não de calendário, e valem só como ordem de
grandeza (o que não foi medido está marcado).

Estado (2026-11-22, manhã; superado, ver a atualização de 2026-11-23 14:35 abaixo): o jogo (1ST_READ do decomp em ARM32) roda no
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

Ordem restante: linkar os 202 eventos → re-medir a matriz inteira com 542
objetos → overlays por área (tirar 39,8 MB de `.data`) → som e filmes →
desempenho e pacote. Os programas de stage deixaram de ser o gargalo: o que
ainda para um stage é o **evento de entrada** (`stage_clear_event`,
`src/stage/stage.c:1082` → `event_load` → `ev%04X.prs`), que não está no `.mod`.

#### OBJETIVO FINAL (pedido do Rafael, 23/11): port completo do SA1 para o Zeebo

"Linkado" quer dizer que compila e entra no `.mod`; só a coluna Estado diz o que
foi **visto** rodando no zeebx. As 12 linhas medidas do build de 518 objetos
cobrem 1 ato por stage; o resto dos atos, campos e chefes nunca rodou.

| Conteúdo | Programa(s) | Estado |
|---|---|---|
| Emerald Coast | stg01 | ato 1 rodou (row 3, 680 quadros, `fc050a4575d8`); atos 2 e 3 não testados |
| Windy Valley | stg02 | ato 1 rodou (row 6); atos 2 e 3 não testados |
| Twinkle Park | stg03 | ato 1 rodou (row 9); atos 2 e 3 não testados |
| Speed Highway | stg04 | ato 1 rodou (row 12); atos 2 e 3 não testados |
| Red Mountain | stg05 | **ainda reseta** com `programa ausente` (row 15, quadro 814) |
| Sky Deck | stg06 | ato 1 rodou (row 18); atos 2 e 3 não testados |
| Lost World | stg07 | ato 1 rodou (row 21); atos 2 e 3 não testados |
| Ice Cap | stg08 | ato 1 rodou (row 24); atos 2 a 4 não testados |
| Casinopolis | stg09 | ato 1 rodou (row 28); atos 2 a 3 não testados |
| Final Egg | stg10 | ato 1 rodou (row 32); atos 2 e 3 não testados |
| Hot Shelter | stg12 | linkado; **não tem linha no menu de debug** (`title.c:384-459`) |
| Hedgehog Hammer | stg00 | rodou (row 0) |
| Station Square (6 áreas) | adv00 | linkado; rows 40-45 resetavam por `adv00` ausente → re-medir |
| Mystic Ruins (4 áreas) | adv02 + tikal_prog | linkado; rows 58-61 resetavam |
| Egg Carrier (6 áreas) | adv01obj + adv0100 | linkado; rows 46-51 resetavam |
| Egg Carrier por dentro (6) | adv01obj + adv0130 | linkado; rows 52-57 resetavam |
| Past (3 áreas) | adv03 + tikal_prog | linkado; rows 62-64 resetavam |
| Chaos 0 / 2 / 4 | b_chaos0 / b_chaos2 / b_chaos4 | Chaos 0 rodou (row 35); 2 e 4 resetavam (rows 36-37) |
| Chaos 6 / Perfect Chaos | b_chaos6 / b_chaos7 | linkados; **sem linha no menu** |
| Egg Viper 1-3 | b_egm1 / b_egm2 / b_egm3 | linkados; 1 tem linha (row 38, resetava), 2 e 3 não |
| Zero (robô do Eggman) | b_robo | linkado; row 39 resetava |
| E101 / E101-R | b_e101 / b_e101_r | linkados; **sem linha no menu** |
| Chao Garden | al_main + al_garden00-02 | linkados; rows 69-71 (ALIFE_SS/EG/MR) nunca rodaram |
| Corrida de Chao | al_race | linkado; row 72 nunca rodou |
| Twinkle Circuit | minicart | linkado; row 65 nunca rodou |
| Sky Chase 1 e 2 | shooting | linkado; rows 66-67 nunca rodaram |
| Sand Hill / snowboard | sboard | linkado; row 68 nunca rodou |
| Filmes (SFD) | zmovie.c + pl_mpeg | imagem decodifica (log `movie slh448_1.sfd`, 22/11); som do filme só com `AUDIO=1` |
| Som | zaudio.c | música 40 s sem laço (`zaudio.c:315`); efeitos e vozes são stubs (`sd_*` = 0) |
| Save | bu* / `sa1vmu.bin` | cartão emulado; save no campo não exercitado |
| Desempenho | — | ~80 ms de jogo por quadro no Emerald Coast (~12 fps) |
| Pacote | — | `.mod` de 62,4 MiB sem assets separados; nenhum pacote instalável |

#### Atualização 23/11 15h — W1, W3, W7 fechados (`32033db`, `HEAD`)

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
- Medida de outros jogos (fullset, 62 jogos): maior `.mod` = 8,7 MB (Quake II),
  mediana 0,5 MB. Developer Guide 0.97: o binario inteiro vai para o heap de 32 MB.
  Meta de `.mod`+bss residente: ~15 MB (hoje 35,5 MB + bss 13,6 MB).

#### 20 quick wins (abertos em 23/11 14:35; P <= 1 h, M <= meio dia)

| # | Entrega | Esforco | Evidencia |
|---|---|---|---|
| W1 | Linkar os 202 eventos `ev*` (cada um que falta faz o `gdFs` resetar o jogo) | M | `game.mk:147`; 202 nomes em `build/zeebo/game/gen/zprogs.c` |
| W2 | Medir o tamanho de cada evento antes de linkar todos (`.text`/`.data` por programa) | P | `arm-none-eabi-size build/zeebo/game/program-*.o` (42 = 45,5 MB hoje) |
| W3 | Aplicar o overlay de `.data` por programa e medir o `.mod` | M | `docs/wip/overlay-dados.patch`; `.data` dos programas = 39,8 MB |
| W4 | `make mod` falhar quando "N de M objetos" vier com N < M | P | `game.mk:310` (só imprime) |
| W5 | Linhas no menu de debug para Hot Shelter e os 6 chefes sem linha (CHAOS6, CHAOS7, EGM2, EGM3, E101, E101_R) | P | `src/progress/title.c:384-459` (tabela sem STAGE_SHELTER) |
| W6 | Rodar a matriz completa (73 linhas) com o build de 542 objetos e colar o `summary.txt` no handoff | M | `tools/stage_matrix.sh:13`; `build/zeebo/matrix/summary.txt` |
| W7 | Logar o nome do arquivo pedido quando o `gdFs` diz "programa ausente" e fechar o caso da row 15 | P | `zgdfs.c` (zlog com `fname`), row 15 = Red Mountain |
| W8 | Save no campo: entrar em Station Square e gravar/ler o `sa1vmu.bin` | P | Fase E3; `cdd0c24` |
| W9 | Música em laço (o ADX toca 40 s e para) | M | `zaudio.c:315` (`MUSIC_SECONDS 40`) |
| W10 | Efeitos e vozes do jogo (`sd_*` devolvem 0) | M | A5; `tools/gen_stubs.py` |
| W11 | `AUDIO=1`: medir o custo em ms/quadro da música | P | `game.mk:251` (`AUDIO ?= 0`) |
| W12 | Filme com som (`AUDIO=1` no `movie_entry`) e ms/quadro | M | `zmovie.c:208`; log `movie us audio decode` de 22/11 |
| W13 | Translúcidos pelo GL em vez do caminho de CPU | M | `zstrip.c:312-344` (`defer_translucent`/`zstrip_flush`) |
| W14 | `kmSetFog*` → `GL_FOG` | P | `zkamui.c:63-65` |
| W15 | `kmSetUserClipping` → `glScissor` | P | `zkamui.c:62` |
| W16 | `kmRenderTexture` → render-para-textura (reflexos) | M | `zkamui.c:29` |
| W17 | Culling em software com quadro fixo e contagem de faces descartadas (refazer o teste inválido do Q15) | M | `zlive.c:340` (`send_strip`, sem culling) |
| W18 | Triar os 614 indefinidos por frequência no run; os 10 mais chamados viram stubs reais | M | `build/zeebo/game/missing.txt` |
| W19 | `PROF=1` no Emerald Coast e atacar a função mais cara | M | ~80 ms de jogo por quadro |
| W20 | Chao Garden rodando (row 69, ALIFE_SS, com `al_main` + `al_garden00`) | P | `modules/al_main`, `modules/al_garden00` |

Fechados desde a atualização de 12:21: **Q1** (matriz de 29 linhas rodada;
12 rodaram, 17 resetavam), **Q2/Q3/Q4** (chefes, campos, jardim e mini-jogos
linkados — `4a9047c`), **Q18** (tamanho medido: 45,5 MB em 42 programas),
**Q20** (PRS × bruto já medido; decisão de manter PRS).

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
