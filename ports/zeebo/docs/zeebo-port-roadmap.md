# Roadmap — Sonic Adventure 1 no Zeebo

Branch: `sa1-reshaped-port` (criada em 2026-11-21 a partir de `decomp`).
Documento canônico de planejamento. As notas de implementação ficam em
`docs/zeebo-port-plan.md`.

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
| Memória | a confirmar no alvo; o jogo assume o layout Dreamcast (16 MB de RAM principal + 8 MB de VRAM) |
| Build | `arm-none-eabi-gcc` → `elf2mod` (Wine) → `.mod` + `.mif` |
| Validação | **zeebx** standalone (ARM/IGLES11) com `--dump-gl`, `--dump-audio`, `--trace` |

## 3. Princípios

1. O jogo, o Ninja e o seam de draw são o **decomp do reshaped**, compilado
   com `-DDREAMCAST -DNONMATCHING`. Nós escrevemos só a camada de hardware.
2. Zeebo usa **T&L de hardware**: vértices em espaço de objeto, `GL_FIXED`, e
   a matriz do Ninja convertida para 16.16 (`glMultMatrixx`). Nada de projeção
   por vértice no CPU (sem FPU).
3. O port é **autocontido**: `ports/zeebo/` não referencia o `src/platform/`
   antigo. Dependências externas: decomp, SH4ZAM (MIT, backend SW), SDK do
   Zeebo e o disco do usuário.
4. Nenhum asset proprietário entra no git: o build gera arrays de
   `extract/` localmente; sem o disco, cai no fallback.
5. Cada marco termina com gate verificável no zeebx (e, quando houver,
   hardware real).

## 3.1 Pré-requisitos de build

- `a raiz deste repositório` — clone do decomp (fontes do jogo/Ninja). Não
  versionado.
- `thirdparty/sh4zam` — clone do SH4ZAM (MIT), backend SW. Não versionado.
- `VENDOR` — BREW 4.0.2 + Zeebo SDK 1.2.4 + Toolset. Padrão do Makefile:
  `../zeebo_demoscene/vendor` (pode sobrescrever com `make VENDOR=...`).
- Disco JP HDR-0001 v1.007 extraído em `extract/` (nunca versionado); os
  assets do `.mod` são gerados dele. Sem o disco, o build usa arrays vazios.
- Os decoders `ports/zeebo/assets/{prs,pvm,pvr}.c` vêm do trabalho anterior
  deste repositório (`src/platform/`), copiados para o port ficar autocontido.

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
| M3b | Texturas por texlist/texid | quads/sprite com texturas distintas do `FROG.prs` | este commit |
| M4b | Assets via IFileMgr (FROG.prs, FISHMISS.adx fora do .mod) | mesmo resultado que embutido | este commit |
| M4c | Shim de GDFS (`gdFsOpen`/`Read`/...) sobre IFileMgr | PRS/ADX lidos por `gdFsOpen`, mesmo resultado | este commit |
| M5c | Clipe ADX inteiro pelo IMedia | envoltória de `FISHMISS.ADX` igual à do ffmpeg | este commit |

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
  do `.mod` no zeebx; no console o caminho real do pacote ainda precisa ser
  confirmado). Buffers fixos: PRS 64 KB, ADX 192 KB, PRS descomprimido 96 KB;
  maiores caem no fallback (checker/sem áudio). `zgdfs.c` implementa o
  subconjunto de GDFS que o loader do jogo usa (Open/GetFileSize/Seek/Tell/
  CheckEof/Read/GetStat/StopRd/Close, setores de 2048, síncrono); nome sem
  `;N`, exato e depois minúsculo. Faltam diretórios, `gdFsOpenRange`, DMA/
  Trans32/ReqRd, TOC e `.afs`.
- `nj_globals.c` define o estado do Ninja (`_unit_matrix_`, pilha) para o
  bring-up; remover quando o `njSystem` do jogo entrar, para não duplicar.
- `present_record` do shim devolve sempre TRUE (sem replay/frame-split).
- Sem validação em hardware físico: zeebx não substitui o console; usar o
  GEC/console quando possível.

## 6. Marcos restantes

### M5c — Fechar o áudio (curto prazo)
- Corrigir duração/streaming da reprodução (hoje só ~0,1 s do clipe de 0,5 s
  é capturado). Caminhos: `AEEMediaDataEx` cru, notificações do `IMedia`,
  ou reabastecimento por `ISource`.
- Feito: `zaudio_stop` libera o `IMedia` no suspend/stop e antes de recriar
  (sem vazamento entre resumes).
- **Gate:** `--dump-audio` contém exatamente o trecho decodificado (mesma
  duração e forma de onda), e o applet não bloqueia o frame.

### M6 — Vídeo SFD (Sofdec)
- Compilar `sdk/sofdec/sj_*` (MPEG) e `mw_*` (demux/ADPCM) para ARM.
- Decodificar um `.SFD` do disco e apresentar o quadro como textura GLES1
  (YUV→RGB em software ou paleta; sem shader).
- Áudio do vídeo pelo caminho do M5c.
- **Gate:** um filme do jogo (ex.: `SLH448_1.SFD`) roda com vídeo e áudio
  sincronizados no zeebx por N segundos.

### M7 — Composição do jogo (o salto)
- Criar a composição ARM equivalente ao `make dc`: fontes de `src/`, `sdk/`,
  `modules/`, `events/` com as substituições do `config/dreamcast.mk`.
- Fornecer o que o port Dreamcast tira do hardware:
  - memória: mapa de RAM/VRAM que o jogo assume (alocar/mapear no BREW);
  - GDFS → `IFileMgr` (ler do pacote de assets no armazenamento);
  - timer (`syTmrGetCount`), input (pad Zeebo), save (VMU → arquivo);
  - frame/Kamui (`kmframe`), task/scheduler e sincronização;
  - áudio (`sd_*`/ADX) pelo caminho do M5c.
- **Gate:** `1ST_READ.BIN` reconstruído roda no zeebx até o **título**
  (menus), lendo assets pelo BREW FS.

### M8 — Primeiro stage jogável
- Carregar Emerald Coast (land, modelos, texturas, HUD) sob demanda.
- Câmera, colisão e áudio do stage.
- **Gate:** entrar no stage, mover o Sonic e ver HUD/frame por N segundos sem
  corromper memória.

### M9 — Performance e pacote
- Medir no alvo: soft-float, listas de draw, VBOs, tamanho de frame.
- Reduzir trabalho de CPU (T&L já no hardware; iluminação só onde o jogo
  exige).
- Empacotar `.mod` + `.mif` e o pacote de assets.
- **Gate:** ~35 fps estável no stage, com memória dentro do orçamento.

### M10 — Validação final
- Hardware real (se disponível) e/ou emulador oficial (GEC/GEC_Device, via
  Wine) como segunda opinião.
- **Gate:** título + um stage + áudio/vídeo em execução contínua.

### Spike M7: o C do jogo compila para ARM32 (medido)

Com `arm-none-eabi-gcc -std=gnu89 -DNONMATCHING -mcpu=arm1136j-s -Os`, sem
`DREAMCAST`, nas fontes de `config/*/splits.txt` (`src/`, `modules/`,
`events/`, 2659 arquivos, includes do slice nativo + `build/gen`):

- 2654 compilam; falham 5: `collision/polygon.c` (usa `mesh_find`, do
  `engine/native/mesh.c`), `path/path.c` (`INFINITY`), `save/card.c` (asm SH-4),
  2 filmes (`text.inc` ausente).
- 1ST_READ (235 fontes): text 4,1 MB, data 1,7 MB, bss 1,1 MB = 6,9 MB. Todos
  os programas somam 10,9 MB, mas só um stage fica carregado por vez.
- Link do 1ST_READ: 1890 símbolos indefinidos, em sua maioria dados do disco
  (texlists, modelos, em `build/gen/*/content.c`), mais SDK (ADX, Ninja, Kamui),
  `engine/native/*.c` e libgcc de soft-float (`__aeabi_*`).
- Segundo passo (SDK + dados): `sdk/` compila 306/448 de cara e +124 com
  `-Isdk -I../sh4zam/include` (sobram 3: `engine/dc/overlay.h`, `pthread.h`,
  `entry_walk`; os 15 `gen/1st_read/d_*.c` são cobertos pelo `content.c`); `build/gen/1st_read/content.c` (10 MB de fonte) compila em
  3,5 s (6,2 MB de objeto). Linkando só os objetos do 1ST_READ: 1204
  indefinidos; 268 são nomes de endereço fixo de `symbols.txt` (resolvidos
  por script de linker), 470 vêm de outros programas (stages, personagens,
  que o loader traz) e restam 466 após compilar todos os `src/modules/events`:
  25 `__aeabi_*` (libgcc), 12 `_builtin_*` (intrinsics SH-4 de
  `engine/native/intrinsics.h`), ~94 `dat_0c...` e tabelas `*_action`/`*_land`
  (dados do `content.c` dos outros programas, ainda não compilados), e poucos
  nomes soltos (`loader_entry`, `frame_*`, `event_skipping`: shims de
  `engine/native`).
- Ressalvas do spike: só checa compilação e símbolos, não comportamento.
  Não aplicou o passe `tools/floats.py` (constantes/expressões float com a
  semântica do SHC) nem `-double=float` das fontes do SDK que o slice nativo
  usa; sem isso a física pode divergir do original. Os objetos de
  `sdk/shinobi` (172) mexem em registradores do SH-4: no port devem ser
  trocados por shims (zgdfs, timer, pad), não linkados.
- Passo 3 (`engine/native`, `engine/sa1`, com os headers do newlib e o SDK do
  jogo por último no include, `-idirafter sdk/include`, senão o `stdio.h` do
  SDK esconde o do newlib): compilam sem mudança `bare`, `loads`, `names`,
  `pads`, `settings`, `vibration` (engine/native) e `sa1/hooks`. Pedem ajuste
  pequeno: `card.c` (`native_fpu_host`), `mesh.c` (marcas `__uptr` do
  `marks.py`), `shim.c` e `sa1/sprites.c` (`play_math.h`), `sa1/*.c`
  (conflito de `strupr` com o newlib). São do host e precisam de
  substituto Zeebo: `gdfs.c` (dirent: usar `zgdfs.c`), `runner.c` e
  `threads.c` (mmap/ucontext), `symbols.c` (elf).
- Mapa de memória (levantado em `~/projects`, só leitura de docs/código):
  - Zeebo real (`zeebo-lle/notes/tripleoxygen_efs_nand_mmu.md`, `zeebo_weird_os/
    notes/PLANO.md`): MSM7201A; SDRAM 32 MB (SMI) em 0x0 e 128 MB (EBI1) em
    0x10000000; o lado APPS usa 96 MB em 0x10000000; SMEM 0x01F00000; kernel
    OKL4 em 0xf0000000, servers Iguana em 0xb0000000. Um app BREW não escolhe
    endereço: o MMU é do OEM, não há mmap de endereço fixo.
  - zeebx (`zeebx-emu/src/loader/mod.rs`): `.mod` em 0x10000, heap em
    0x10000000 com 64 MB, pilha em 0x20000000 com 1 MB, extensões em
    0x08000000. São escolhas do emulador, não prova do mapa do console.
  - NÃO achei o orçamento de heap de um app no hardware real (nem
    `GetMemStats` medido). Medir antes do M8/M9.
  - Lado Dreamcast: o jogo só fixa 35 endereços de RAM principal (contagem sem comentários: 127 ocorrências 0x0c/8c/ac/a5/a0/e0 no código, o resto é registrador do PVR/SQ do hardware, cores e atributos Ninja), todas em
    0x0c000000..0x0d000000 (16 MB): `WORK_AREA 0x0cd00000..0x0cf00000`,
    `MODULE_AREA 0x0c900000`, `FIELD_AREA 0x0c920000`, `CAMERA_AREA_*`
    0x0cb50000.., `SET_AREA_*` 0x0cb68000.., `EVENT_MODULE 0x0cb80000`,
    `MOTION_AREA 0x0cc00000`, `CAPTURE_BUFFER 0x0ce00000`,
    `MODEL_A/B_BUFFER 0x8cca4000/6000` (espelho cacheado 0x8c), etc. (todas
    `#define ... ((tipo *)0x0cXXXXXX)` em `src/`). Os 10589 outros números
    0x8c/0x0c são, em geral, comentários/anotações. Os ponteiros dos dados do
    disco em `content.c` são nomes (relocados pelo linker), não números.
  - Plano de rebase: uma arena única de 16 MB via `MALLOC` no start e um
    header gerado que reescreve essas ~28 macros para
    `(arena + (endereço - 0x0c000000))` (mesma ideia do domínio do slice
    nativo, em 32 bits). Risco aberto: os programas (stages/personagens) são
    carregados por `gdFsRead` sobre essas áreas; seus ponteiros internos
    precisam de relocação ao carregar (`engine/native/loads.c`).
- Rebase implementado (spike): `ports/zeebo/tools/gen_rebase.py` reescreve os 34
  literais 0x0c/0x8c do código (13 arquivos; comentários e strings intactos)
  para `ZDC(...)`, e `ports/zeebo/zrebase.h` define `ZDC(a)` como `zdc_ram +
  (a & 0xffffff)`. Os 13 arquivos compilam em ARM. Três usam o endereço em
  inicializador estático (`sound/stream.c`, `progress/recorder.h`,
  `shooting.c`), o que exige `zdc_ram` como array em .bss (endereço
  constante de link) e não ponteiro de MALLOC: custo de 16 MB de bss no
  `.mod`. Risco: limite de bss/tamanho do `.mod` no console (não medido).
  `sbinit.c` (`MEMORY_END`, computado) e o `loader/` (outro programa) ficam
  fora. Falta `zdc_ram` definido, o gerador ligado ao Makefile e a carga dos
  programas nas áreas.
- Medido no zeebx: arena `zdc_ram` de 16 MB em bss gerou `.mod` de 18,5 MB
  (elf2mod grava o bss como zeros) e carregou; com 7 MB (só
  0x0c900000..0x0d000000 é endereçado) o `.mod` fica com 9,1 MB e roda igual
  (83 SwapBuffers, 9 GenTextures, `zdc_selftest` passa). O `.mod` de 9 MB
  ainda precisa ser validado no console (limite de tamanho/bss desconhecido).
- `build/engine/native/asm.c` (asm2c.py: o asm do SH-4 do jogo em C, 9362
  linhas, 116 funções) compila em ARM com `-DNATIVE_DOMAIN=0 -include
  engine/native/prelude.h`: os endereços do registrador/memória do SH-4 viram
  ponteiros ARM diretos, já que os ponteiros do jogo compilado são nativos de
  32 bits. Depende de `sa_f*` do `play_math`, `native_atan2` e
  `_nj_*_matrix_` (shims). Restam 462 indefinidos no link do 1ST_READ, 105 sem
  contar tabelas `*_action`/`*_land`, `dat_0c*` e `_builtin_*`: `frame_*`,
  `present_*`, `port_*`, `loop*_body`, `prepare/resume/run` (threads),
  `submit_*`, texlists/objects soltos.
- Threads (feito, `ports/zeebo/zthreads.c`): troca de contexto ARM
  (`zctx_switch`: push/pop de r4-r11+lr, soft-float) e as quatro rotinas
  asm do `thread.c` do jogo (`prepare`, `run`, `thread_yield`, `resume`) no
  mesmo desenho de `engine/native/threads.c`, sem ucontext/mmap. 32
  contextos x 32 KB em bss (sem guard page: só um canário no fim da pilha,
  `zthreads_check`; ajustar após medir o uso real). `sa1_main` roda numa
  corrotina de 128 KB: `zgame_step()` (timer do BREW) entra, `native_vsync()`
  (fim de frame) devolve. Verificado no zeebx por ping-pong entre duas
  pilhas (`zthreads_selftest`; confirmado de verdade: forçando a falha o applet
  não desenha, 0 SwapBuffers, e passando dá 82). `thread_current/next/exit` são
  fracos até o `thread.c` do jogo ser linkado; o `thread.c` real do jogo ainda não foi
  ligado, então yield/prepare/run não foram exercidos com ele.
- Build do jogo em ARM (feito, `ports/zeebo/game.mk`): `make -f game.mk link`
  compila `src/ modules/ events/ sdk/` do 1ST_READ (exceto `sdk/shinobi`),
  `content.c`, `asm.c`, os shims `loads/names/pads/settings/vibration`,
  `sa1/hooks`, com o rebase aplicado, e faz `ld -r` em `build/zeebo/game/game.o`
  (500 de 503 objetos; falham `collision/polygon.c`, `save/card.c`,
  `sdk/ninja/njModelE.c`). Sem definições duplicadas (`bare.c` fica de fora:
  repete o `loader_read_*` do jogo). 1207 indefinidos: ~930 dados de
  outros programas (`*_action`, `*_land`, `*_texlist`, `dat_0c*`, `script_*`:
  o loader os traz), 244 nomes por endereço do `symbols.txt`, 25 `__aeabi_*`
  e 11 `_builtin_*`. Os que cabem a nós: `frame_*`, `present_*`, `port_*`,
  `submit_*`, `loop*_body`, `sa_f*` (play_math), `native_atan2`,
  `native_hook_*`, `native_programs`, `prepare/resume/run` (zthreads.c, ainda
  não ligado a este link), `loader_entry/movie_entry`, `writeback_line`.
- Ainda sem resolver: mapa de endereços fixos (0x8Cxxxxxx nos dados do disco),
  troca de contexto das threads (`engine/native/threads.c` usa ucontext) e o
  GDFS do `engine/native` (usa dirent): trocar pelo `zgdfs.c`.

## 7. Dependências e ordem

```
M5c (áudio) ──┐
M6 (SFD)  ────┼──> M7 (composição) ──> M8 (stage) ──> M9 (perf) ──> M10
M4 (assets) ──┘
```

M7 é o maior salto e depende de M4/M5c. M6 pode andar em paralelo a M5c, mas
compartilha o caminho de áudio.

## 8. Riscos e mitigação

| Risco | Impacto | Mitigação |
|---|---|---|
| ARM11 sem FPU: soft-float caro | baixo FPS | T&L no hardware; medir cedo; evitar float por vértice; ponto fixo no GL |
| Sem GD-ROM: assets de 1,2 GB | não cabe/streaming | pacote de assets + leitura sob demanda via BREW FS; priorizar por stage |
| Memória do jogo (layout Dreamcast) | crash/overlap | mapear RAM/VRAM cedo (M7) e validar com o memory.py do decomp |
| Iluminação sem hardware | visual errado/lento | portar só o necessário; comparar com o PC/DC |
| SFD (MPEG+ADPCM) pesado | vídeo travado | decodificar em thread/por quadro; validar com `--dump-gl` |
| zeebx ≠ hardware | falso positivo | usar o GEC/console quando possível; medir no alvo |
| Host LP64 engana o sofdec | testes inválidos | validar no ARM32 (ou `-m32`) |
| Licença do decomp | não redistribuir | fonte só em `thirdparty/` (ignorado); shims derivados (`sa1/sprite.h`, `njsprite.h`) são **gerados no build** e o repo guarda só o gerador |

## 9. Convenções de trabalho

- Um commit por incremento verificável; mensagem no estilo do repo
  (`zeebo: ...`), com a evidência no corpo.
- Gate antes de avançar: build limpo, execução no zeebx e imagem/áudio
  inspecionados.
- Assets e `build/` nunca versionados; arrays gerados do `extract/` local.
- Notas de implementação e decisões vão para `docs/zeebo-port-plan.md`.

## 10. Próxima ação

Fechar o **M5c** (duração/streaming do áudio) e, em seguida, **M6** (SFD),
antes de começar a composição do jogo (M7).
