# Roadmap — Sonic Adventure 1 no Zeebo

Repositório de trabalho: `requeijaum/sonic-adventure-zeebo`, branch
`zeebo-port` (fork de `reshaped/sonic-adventure`), código em `ports/zeebo/`.
Commits locais; um cron envia ao remoto a cada 4 h (`~/.local/bin/zeebo-push.sh`).
O histórico anterior (hashes abaixo) vive no repositório `sa1-zeebo`.
Documento canônico de estado e riscos; a ordem de execução está em
`roadmap-ate-o-titulo.md`. As notas de implementação ficam em
`docs/zeebo-port-plan.md`. Última revisão: 2026-11-21.

## 1. Objetivo

Rodar o Sonic Adventure no Zeebo a partir das **fontes C do decomp matching**
(`reshaped/sonic-adventure`), com backend gráfico próprio em OpenGL ES 1.x e
plataforma BREW, sem rehost do binário SH-4 e sem depender do `engine/pc`
(Vulkan) do upstream.

Alvo de referência: **JP HDR-0001 v1.007** — é a release que o decomp reproduz
byte a byte (o `1ST_READ.BIN` do `extract/jp` bate com o `SHA256SUMS` do
upstream). Os assets vêm desse disco.

## 2. Alvo técnico (fatos)

| Item | Valor |
|---|---|
| CPU | ARM1136J-S, ARMv6, **sem VFP/NEON** (float em soft) |
| GPU | Adreno 130 (Imageon 3D): **GLES 1.x fixo**, T&L de hardware, **sem shaders e sem iluminação de hardware** |
| OS/API | BREW 4.0.2, Zeebo SDK 1.2.4 |
| Tela | 640×480, ~35 fps |
| Memória | hardware: SDRAM 32 MB (SMI) + 128 MB (EBI1), 96 MB para o lado APPS; **orçamento de heap de um app não medido**. zeebx: heap de 64 MB (escolha do emulador). O jogo assume RAM principal do DC (16 MB, só 7 MB endereçados) |
| Build | `arm-none-eabi-gcc` → `elf2mod` (Wine) → `.mod` + `.mif` |
| Validação | **zeebx** standalone (ARM/IGLES11) com `--dump-gl`, `--dump-audio`, `--trace`; **BREW Simulator** do SDK 4.0.2 (x86/DLL, via Wine; não roda `.mod` ARM) como segunda opinião da lógica, se houver alvo x86; **sem hardware real por ora** |

## 3. Princípios

1. O jogo, o Ninja e o seam de draw são o **decomp do reshaped**, compilado
   com `-DNONMATCHING` (sem `DREAMCAST`: essa flag traz instruções do SH-4)
   para ARM32. Nós escrevemos só a camada de hardware.
2. Zeebo usa **T&L de hardware**: vértices em espaço de objeto, `GL_FIXED`, e
   a matriz do Ninja convertida para 16.16 (`glMultMatrixx`). Nada de projeção
   por vértice no CPU (sem FPU).
3. O port é **autocontido**: `ports/zeebo/` não referencia o `src/platform/`
   antigo. Dependências externas: decomp, SH4ZAM (MIT, backend SW), SDK do
   Zeebo e o disco do usuário.
4. Nenhum asset proprietário entra no git: o build gera arrays de
   `extract/` localmente; sem o disco, cai no fallback.
5. Cada marco termina com gate verificável no zeebx (e, quando houver,
   GEC; hardware real, se um dia houver).

## 3.1 Pré-requisitos de build

- A raiz deste repositório é o decomp (fontes do jogo/Ninja); `ports/zeebo/`
  é o port.
- `tools/sh4zam` (ou `SHZAM=`) — SH4ZAM (MIT), backend SW. Não versionado.
- `make pc` do decomp, a partir do disco, gera `build/gen/` (necessário para
  `game.mk`).
- `VENDOR` — BREW 4.0.2 + Zeebo SDK 1.2.4 + Toolset. Padrão do Makefile:
  `../zeebo_demoscene/vendor` (pode sobrescrever com `make VENDOR=...`).
- Disco JP HDR-0001 v1.007 extraído em `extract/` (nunca versionado); os
  assets do `.mod` são gerados dele. Sem o disco, o build usa arrays vazios.
- Os decoders `ports/zeebo/assets/{prs,pvm,pvr}.c` vêm do trabalho anterior
  (`sa1-zeebo`, `src/platform/`), copiados para o port ficar autocontido.

## 3.2 Fatos do Zeebo Developer Guide 0.97 (2009; confirmar)

Fonte: `zeebo-emulator/research/docs/sdk-extract/ZeeboDeveloperGuide0.97.md`.

- **Heap de 32 MB** por app, e o **binário inteiro é carregado no heap**: um `.mod`
  de 10 MB (arena em bss incluída) deixa ~22 MB, menos o que outros apps usam.
  Este é o orçamento que o zeebx (64 MB) não impõe. Sistema de arquivos de
  1 GB compartilhado; save de até 64 KB.
- **Módulos dinâmicos "não têm segmento de leitura-escrita"**: o guia desencoraja
  variáveis estáticas/globais; o `elf2mod` mais novo suporta algumas com
  fix-up (é o que o `.mod` usa, e roda no zeebx). O jogo tem ~1,7 MB de data +
  bss e a arena de 7 MB: risco real no console, não medido.
- **Sem libc** no alvo: usar `MALLOC/FREE/MEMCPY` e a stdlib do BREW.
- **Cooperativo**: o app deve devolver o controle rápido; laço longo faz a
  plataforma reiniciar o app. O `sa1_main` como corrotina cede a cada `native_vsync`,
  mas um quadro longo pode estourar o limite (medir).
- **Pilha** do alvo menor que a do simulador: dados grandes no heap.
- ARM com **float emulado** (lento), como já assumimos.
- O guia afirma que o ARM do Zeebo é **big endian**; isso contradiz o zeebx e o
  `.mod` que já roda (little-endian) e o MSM7201A em modo padrão. Tratar como erro
  do guia até o `zeebo-lle` ou outro teste confirmar.

## 4. Arquitetura atual

```
jogo/sdk (C do decomp)  --Ninja-->  sa1/*.h do decomp
                                      |  (única troca: sa1_send_vertex)
                                      v
                              zstrip (sink GLES1)
                                      ^
njDraw.c / njSprite.c (reais) --------+
engine/dc/ninja/*.c (13 reais) --SH4ZAM SW--> matriz/projeção
zcube.c (T&L hardware)  ---------+--> glMultMatrixx
zaudio.c (ADX do decomp) --------+--> IMedia (WAV em memória)
assets: PRS -> PVM -> PVRT ------)--> textura GLES1
arquivos: gdFsOpen/Read (zgdfs.c) --> IFileMgr (zfile.c)
jogo (game.mk, ARM32) + zthreads.c + arena zdc_ram (zrebase)  [ainda não linkado ao applet]
plataforma: ports/zeebo/platform (EGL/BREW, string, softfloat)
```

## 5. Estado (o que já foi provado)

| Marco | Entrega | Evidência | Commit |
|---|---|---|---|
| M0 | Toolchain ARM → `.mod` → zeebx | `sa1hello` e `sa1z` rodando | — |
| M1 | Sink GLES1 + `njDrawTexture` real | quads texturizados, blend, depth | `2ed91c3` |
| M2 | Matemática do Ninja em ARM | 13 `engine/dc/ninja/*.c` + selftest host PASSED | `2ed91c3` |
| M2b | Cubo 3D com T&L de hardware | frame no zeebx, matriz do Ninja | `2ed91c3` |
| M3 | Sprites 2D reais | `njDrawSprite2D` girando no frame | `2ed91c3` |
| M4 | Texturas do disco | `FROG.prs` → 8 texturas; textura no frame | `9f1ef81` |
| M5 | ADX decodifica no alvo | forma de onda de `FISHMISS.ADX` | `bd89207` |
| M5b | Saída de áudio via IMedia | `--dump-audio` captura o burst | `557879e` (parcial) |
| M3b | Texturas por texlist/texid | quads/sprite com texturas distintas do `FROG.prs` | `cdd5345` |
| M4b | Assets via IFileMgr (fora do .mod) | mesmo resultado que embutido | `52cb248` |
| M4c | Shim de GDFS (`gdFsOpen`/`Read`/...) sobre IFileMgr | PRS/ADX lidos por `gdFsOpen` | `cfadeab` |
| M5c | Clipe ADX inteiro pelo IMedia | envoltória de `FISHMISS.ADX` igual à do ffmpeg (blocos de 50 ms, <2%) | `59ea2a3` |
| M7a | Arena `zdc_ram` + rebase dos 34 endereços fixos | `.mod` de 9 MB carrega no zeebx, `zdc_selftest` passa | `ac15304`, `4584f71` |
| M7b | Threads/corrotinas ARM (`zctx_switch`) | selftest ping-pong; falha forçada => 0 SwapBuffers | `09398c6`, `ef1034a` |
| M7c | `game.mk`: 1ST_READ do decomp em ARM32 | 509/510 objetos; `full.o` (+libgcc) com 1132 indefinidos | `20bf1ae`, `8d5d6cd` |
| M7d | Timer `syTmr*` e pad `pd*` | `--teclas` move o sprite; indicador do timer verde | `a83bfb3` |

M5b (IMedia) fechou no M5c. Os marcos M7a-c são infraestrutura: o jogo ainda
**não roda**; nada do `thread.c`/`sa1_main` real foi executado.

### 5.1 Limitações conhecidas do estado atual

- Texturas: `njLoadTexturePvmMemory` cria uma textura GL por entrada (até
  512 globais, cada uma até 256x256); `njSetTexture`/`njSetTextureNum`/
  `njSetkmTextureNumG` selecionam. Sem cache/VRAM budget nem mipmaps; a
  seleção fica "grudada" até o próximo set (como no Ninja).
- Sprite: a intensidade por vértice é ignorada; vale a cor constante do
  strip (intensidade 1).
- Passe 2D sem depth test (ordem de desenho). Revisar quando o jogo ordenar
  2D por z.
- Arquivos: `zfile.c` lê/grava via `IFileMgr` (caminho relativo = diretório
  do `.mod` no zeebx; no GEC/console o caminho real do pacote ainda precisa ser
  confirmado). Buffers fixos: PRS 64 KB, ADX 192 KB, PRS descomprimido 96 KB;
  maiores caem no fallback (checker/sem áudio). `zgdfs.c` implementa o
  subconjunto de GDFS que o loader do jogo usa (Open/GetFileSize/Seek/Tell/
  CheckEof/Read/GetStat/StopRd/Close, setores de 2048, síncrono); nome sem
  `;N`, exato e depois minúsculo. Faltam diretórios, `gdFsOpenRange`, DMA/
  Trans32/ReqRd, TOC e `.afs`.
- `nj_globals.c` define o estado do Ninja (`_unit_matrix_`, pilha) para o
  bring-up; remover quando o `njSystem` do jogo entrar, para não duplicar.
- `present_record` do shim devolve sempre TRUE (sem replay/frame-split).
- `zsys.c` (timer `syTmr*` e pad `pd*`) é só rascunho no `sa1-zeebo`: não está neste repositório.
- Arena `zdc_ram` em bss custa ~7 MB no `.mod` (elf2mod grava bss como zeros); limite do console não medido.
- Sem hardware físico por ora: zeebx e GEC são os validadores. O GEC ainda não
  foi localizado/rodado nesta máquina. Afirmações que dependem do console
  (tempo real, limite de `.mod`, memória física) ficam como risco.

## 6. Marcos restantes

### M6 — Vídeo SFD (Sofdec)
- Compilar `sdk/sofdec/sj_*` (MPEG) e `mw_*` (demux/ADPCM) para ARM.
- Decodificar um `.SFD` do disco e apresentar o quadro como textura GLES1
  (YUV→RGB em software ou paleta; sem shader).
- Áudio do vídeo pelo caminho do M5c.
- **Gate:** um filme do jogo (ex.: `SLH448_1.SFD`) roda com vídeo e áudio
  sincronizados no zeebx por N segundos.

### M7 — Composição do jogo (o salto) — EM ANDAMENTO

Gate: `1ST_READ` reconstruído roda no zeebx até o **título** (menus), lendo
assets pelo BREW FS. Checklist (ordem de execução):

- [x] Arena de RAM do DC e rebase dos endereços fixos (M7a).
- [x] Threads e laço do jogo como corrotinas (M7b), sem uso com o `thread.c` real.
- [x] Compilar o 1ST_READ em ARM32 (M7c).
- [x] Objetos que falhavam: `polygon.c` (marks.console), `card.c` (alias de asm
  em ARM), `mesh.c` (P32) via `tools/gen_patch.py`; 509/510 objetos. Resta
  `sdk/ninja/njModelE.c` (exige o backend SH-4 do SH4ZAM: easy models; o slice
  nativo usa o lado enviado do jogo) e `src/draw/model.c` (precisa do
  `engine/dc/sa1/live.h`, ver abaixo).
- [x] `NONMATCHING_C` (`pace`, `port`, `present`, `skip`): resolveu `present_*` e
  `port_*` sem escrever shim.
- [x] Intrínsecos `_builtin_*`, `native_atan2`, `sa_f*` (`zshim.c`,
  `engine/numeric/math.c`), libgcc (`full.o`), `strchr/strspn/strcspn/
  strcasecmp/abort`.
- [x] Timer (`syTmr*`) e pad (`pd*` por EVT_KEY) em `zsys.c`, provados no zeebx
  (`--teclas=300:right` muda o quadro; indicador do timer verde).
- [x] `check.sh` (build, host-selftest, zeebx, pad, timer, game.mk).
- [x] `GETRAMFREE` logado no start (zeebx: 64 MB, sem total/maior; o GEC
  pode dar outro número).
- [x] SFD/Sofdec: `sdk/sofdec` (26 objetos) compila e não deixa símbolo
  `mw_/sj_/adx_` indefinido; falta decodificar/apresentar (M6 em si).
- [ ] Modelo 3D: `src/draw/model.c` + `engine/dc/sa1/{live,model}.c` (o laço de
  modelo do DC, `draw_*`, `submit_*`, `view_*`, `writeback_line`,
  `loop*_body`, `NJM_SET_*_STRIP`) sobre o sink GLES1. É o maior item.
- [ ] Integrar o link jogo + applet: `ld` com `full.o` + objetos do applet dá 69
  definições duplicadas (ADX, `njDraw`, `njSprite`, `nj_globals`, `memcpy/
  memset/cos`, `njLoadTexturePvmMemory`...): decidir o dono de cada uma
  (o applet sai ou o jogo sai) e ligar `gdFs*`, `kmRender/kmFlipFrameBuffer`,
  `bu*` (save), libc que os shims de host pedem (`malloc/free/fopen/...`).
- [ ] Ligar `zthreads.c` ao `thread.c` do jogo (tirar as pontas fracas) e
  exercitar prepare/run/yield de verdade.
- [ ] Os 244 nomes por endereço do `symbols.txt` (script de linker/ZDC).
- [ ] Programas (stages, personagens, filmes): `content.c` de cada um, carga por
  `gdFsRead` nas áreas rebaseadas e relocação dos ponteiros internos
  (`engine/native/loads.c` como referência), `native_programs`, `native_hook_*`.
- [ ] Passe de floats do slice nativo (`tools/floats.py`, `-double=float` do SDK).
- [ ] Salvar (VMU -> arquivo; `bu*`/`save_*`).
- [ ] Linkar jogo + applet num `.mod`, rodar `sa1_main` no zeebx até a primeira falha.

Evidências do spike (números medidos, sem executar o jogo) estão em 6.1.

### M8 — Primeiro stage jogável
- Carregar Emerald Coast (land, modelos, texturas, HUD) sob demanda.
- Câmera, colisão e áudio do stage.
- **Gate:** entrar no stage, mover o Sonic e ver HUD/frame por N segundos sem
  corromper memória.

### M9 — Performance e pacote
- Medir no zeebx (`--perfil`) e, se possível, no GEC: soft-float, listas de draw, VBOs, tamanho de frame.
- Reduzir trabalho de CPU (T&L já no hardware; iluminação só onde o jogo
  exige).
- Empacotar `.mod` + `.mif` e o pacote de assets.
- **Gate:** sem hardware, FPS real não é medível: o gate é o orçamento de CPU
  por quadro no `--perfil` do zeebx (meta: caber em ~28 ms a ~35 fps no
  ARM11) e memória dentro do orçamento enxuto; FPS real fica como risco.

### M10 — Validação final
- Emulador oficial (GEC/GEC_Device, via
  Wine) como segunda opinião.
- **Gate:** título + um stage + áudio/vídeo em execução contínua.

### 6.1 Evidências do spike M7 (medido)

**Compilação.** `arm-none-eabi-gcc -std=gnu89 -DNONMATCHING -mcpu=arm1136j-s
-Os`, sem `DREAMCAST`: 2654 de 2659 fontes de `src/ modules/ events/` compilam
(falham `polygon.c`, `path.c` [INFINITY, resolvido com `-DINFINITY`], `card.c` e
2 filmes por `text.inc`). `sdk/` (exceto `sdk/shinobi`, que mexe em registrador
do SH-4 e vira shim) e `build/gen/1st_read/content.c` (10 MB de fonte, 3,5 s)
compilam. O include do SDK do jogo precisa vir por último
(`-idirafter sdk/include`) quando se usam headers do newlib.

**Tamanho.** 1ST_READ: text 4,1 MB, data 1,7 MB, bss 1,1 MB (6,9 MB). Todos os
programas somam 10,9 MB, mas só um stage fica carregado por vez.

**Link do 1ST_READ (`game.o`, parcial).** 1207 indefinidos: ~930 dados de
outros programas (`*_action`, `*_land`, `*_texlist`, `dat_0c*`, `script_*`),
244 nomes do `symbols.txt` por endereço, 25 `__aeabi_*` e 11 `_builtin_*`.
`engine/native/{loads,names,pads,settings,vibration}.c` e `engine/sa1/hooks.c`
compilam sem mudança; `bare.c` repete `loader_read_*` e fica de fora; `gdfs.c`
(dirent), `runner.c`/`threads.c` (mmap/ucontext) e `symbols.c` (elf) são do
host e têm substituto Zeebo (`zgdfs.c`, `zthreads.c`).

**`asm.c`.** `asm2c.py` escreve o asm do SH-4 do jogo como C (9362 linhas,
116 funções); compila em ARM com `-DNATIVE_DOMAIN=0 -include
engine/native/prelude.h`, os endereços do registrador/memória do SH-4 viram
ponteiros ARM diretos. Depende de `sa_f*`, `native_atan2` e `_nj_*_matrix_`.

**Mapa de memória.** Zeebo real (`zeebo-lle`, `zeebo_weird_os`): SDRAM 32 MB
(SMI) em 0x0 e 128 MB (EBI1) em 0x10000000; lado APPS 96 MB em 0x10000000;
SMEM 0x01F00000; OKL4 em 0xf0000000, Iguana em 0xb0000000. Um app BREW não
escolhe endereço. zeebx: `.mod` em 0x10000, heap 0x10000000 de 64 MB, pilha
0x20000000 de 1 MB (escolhas do emulador). Lado DC: 35 endereços de RAM
principal no código (127 ocorrências de endereço no código sem comentários;
as demais são registradores PVR/SQ, cores e atributos), todos em
0x0c000000..0x0d000000; o menor é `MODULE_AREA 0x0c900000`. Ponteiros dos
dados do disco em `content.c` são nomes, relocados pelo linker.

**Rebase.** `tools/gen_rebase.py` reescreve 34 literais (13 arquivos) para
`ZDC(...)`; `zrebase.h` mapeia `0x0c900000..0x0d000000` para `zdc_ram` (7 MB em
bss: 3 inicializadores estáticos exigem endereço constante de link). Com 16 MB o
`.mod` ficou com 18,5 MB; com 7 MB, 9,1 MB; ambos carregam no zeebx.

**Ressalvas.** O spike só verifica compilação e símbolos, não comportamento.
Não aplicou o passe `tools/floats.py` nem `-double=float`. O `thread.c` e o
`sa1_main` reais nunca rodaram.

## 7. Dependências e ordem

```
M4/M5c (feitos) ──> M7 (composição, em andamento) ──> M8 (stage) ──> M9 (perf) ──> M10
M6 (SFD, paralelo) ──┘
```

M7 é o maior salto; M4 e M5c já estão fechados. M6 pode andar em paralelo a M7,
mas compartilha o caminho de áudio.

## 8. Riscos e mitigação

| Risco | Impacto | Mitigação |
|---|---|---|
| ARM11 sem FPU: soft-float caro | baixo FPS | T&L no hardware; medir cedo; evitar float por vértice; ponto fixo no GL |
| Sem GD-ROM: assets de 1,2 GB | não cabe/streaming | pacote de assets + leitura sob demanda via BREW FS; priorizar por stage |
| Memória do jogo (layout Dreamcast) | crash/overlap | mapear RAM/VRAM cedo (M7) e validar com o memory.py do decomp |
| Iluminação sem hardware | visual errado/lento | portar só o necessário; comparar com o PC/DC |
| SFD (MPEG+ADPCM) pesado | vídeo travado | decodificar em thread/por quadro; validar com `--dump-gl` |
| zeebx ≠ hardware (e sem hardware) | falso positivo | exigir concordância zeebx + GEC; tratar o que depende de hardware como risco com margem; hardware real só se surgir |
| `.mod` de ~10 MB (arena em bss + pilhas) | pode exceder limite do console | sem hardware: reduzir por precaução (checar o limite do GEC); alternativa: MALLOC + reescrever 3 inicializadores estáticos |
| Heap de 32 MB por app, com o `.mod` carregado nele (guia); zeebx dá 64 MB | jogo não cabe | orçamento: `.mod` + arena + pilhas + assets < ~28 MB; reduzir a arena (7 MB) e as pilhas |
| Sem segmento RW em módulo dinâmico (guia) | `.mod` com muito data/bss pode não carregar | validar com `elf2mod` e o zeebx; minimizar estáticos; alternativa: heap |
| Floats sem o passe do SHC | física diverge | decidir sobre `floats.py`; comparar com o original |
| Host LP64 engana o sofdec | testes inválidos | validar no ARM32 (ou `-m32`) |
| Licença do decomp | não redistribuir assets | este repositório é fork do decomp (sem licença declarada upstream); assets/ROM/`1ST_READ.BIN`/`extract/` nunca vão ao git; shims derivados (`sa1/sprite.h`, `njsprite.h`) são **gerados no build** |

## 9. Convenções de trabalho

- Um commit por incremento verificável; mensagem no estilo do repo
  (`zeebo: ...`), com a evidência no corpo.
- Gate antes de avançar: build limpo, execução no zeebx e imagem/áudio
  inspecionados.
- Assets e `build/` nunca versionados; arrays gerados do `extract/` local.
- Notas de implementação e decisões vão para `docs/zeebo-port-plan.md`.
- Trabalho em `ports/zeebo` no fork; push automático a cada 4 h (só commits).
- Antes de afirmar "verificado": confira que o build gerou `Created` e que o
  teste falha quando forçado a falhar (houve um falso positivo nas threads).

## 10. Próxima ação

1. Modelo 3D: `model.c` + `engine/dc/sa1` sobre o sink GLES1 (maior item do M7).
2. Integrar o link jogo + applet (69 duplicatas) e rodar `sa1_main` até a
   primeira falha no zeebx.
3. Programas/carga e `symbols.txt`. Em paralelo: M6 (decodificar/apresentar SFD).
4. Localizar o GEC e rodar o `sa1hello`/`sa1z` nele (segunda opinião).
