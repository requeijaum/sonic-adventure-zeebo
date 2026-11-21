# Port SA1 → Zeebo: plano de engenharia

Roadmap canônico: `docs/zeebo-port-roadmap.md`.

Estado: **Z0, Z1 e a base do Z2 concluídos** (2026-11-21). O `njDrawTexture`
real e um cubo 3D transformado pelo hardware já desenham no zeebx; a
matemática do Ninja (13 `engine/dc/ninja/*.c`) compila e linka para ARM; e o
port é autocontido em `ports/zeebo/`. Documento de
trabalho; não é promessa de resultado.

## Objetivo

Rodar Sonic Adventure 1 no Zeebo a partir das fontes C do decomp matching
(`reshaped/sonic-adventure`), com assets do disco japonês, sem rehost de
binário SH-4.

## Fatos que definem o desenho

- Alvo: ARM1136J-S (ARMv6, **sem VFP/NEON**), BREW 4.0.2, **OpenGL ES 1.x
  fixo** (sem shaders), 640×480, ~35 fps. Tectoy/Qualcomm.
- O decomp já separa jogo e hardware: o jogo + SDK em C fala Ninja/Kamui, e
  `engine/dc` implementa a submissão em PVR.
- **A costura certa não é a ABI `sa_engine.h`.** Com `DREAMCAST` +
  `NONMATCHING`, o próprio `sdk/ninja/njDraw.c` despacha para
  `sa_present_live`, e `engine/dc/sa1/polygon.h` termina em
  `sa1_send_vertex` (`engine/dc/sa1/strip.h`), que escreve na store queue do
  PowerVR. Logo:
  - `sa1_send_vertex` → acumulador de vértices GLES1;
  - abertura/fecho de strip (`njGetkmPointer`, `njStartVertex*`,
    `njEndVertex` em `sdk/ninja/njkm.c`) → estado GLES1;
  - `engine/dc/batch.h` (kernels SH4ZAM de projeção/iluminação de modelo) →
    C portátil;
  - resto de `engine/dc` (overlay/wide/owner/crash) é recurso do port e pode
    ser omitido no primeiro marco.
- Toolchain já validada neste repo: `arm-none-eabi-gcc` → `elf2mod` (Wine) →
  `.mod` + `.mif`, rodando no **zeebx** standalone (`~/.local/bin/zeebx`) com
  IGLES11 (148 slots, variantes float). O `sa1hello` prova isso ponta a ponta.
- SDK proprietário fora do git: BREW 4.0.2 SP19, Zeebo SDK 1.2.4, Toolset em
  `projects/zeebo-emulator/research/docs/sdk-extract/`; `vendor/` por symlink.
- Licença: este repositório é um fork do decomp (`reshaped/sonic-adventure`, sem
  licença declarada upstream). O port vive em `ports/zeebo/`; assets, ROM e
  `1ST_READ.BIN` nunca entram no git, e os shims derivados são gerados no build.

## Convenção de vértice do SA1 (o contrato do sink)

`x, y` em pixels de tela (0..640, 0..480), `z` é **1/profundidade** (maior =
mais perto), `u, v` normalizados, `color` ARGB. O sink GLES1 mapeia:

- NDC `x = x/320 - 1`, `y = 1 - y/240`
- janela `z = z` com `glDepthFunc(GL_GEQUAL)` e clear depth 0 (mesma ordem do
  PVR)
- ARGB → RGBA float por vértice; `GL_MODULATE` para cor × textura.

## Marcos

- **Z0 (atual)** — `ports/zeebo/`: applet BREW standalone que desenha strips
  texturizados pelo sink GLES1, verificado no zeebx (dump de frames). Prova
  toolchain + EGL + sink + convenção de vértice.
- **Z1 (concluído)** — `sdk/ninja/njDraw.c` real compilado para ARM com
  `DREAMCAST+NONMATCHING`; shims em `ports/zeebo/ninja/` (`machine.h`,
  `sa1/strip.h`, `ninja_zam.h` vazio, `njkm_zeebo.c`, `port_shim.c`).
  `njDrawTexture` desenha no zeebx (2 quads por `njDrawTexture` + 1 banda).
- **Z2 (em andamento)** — camada de matemática pronta:
  - SH4ZAM (`tools/sh4zam` ou `SHZAM=`, MIT; não versionado) com o backend **SW** em ARM
    (`-DSHZ_TLS_MODEL=0`), no lugar do asm SH-4.
  - `ports/zeebo/ninja/ninja_zam.h` reimplementa os helpers do decomp em C
    (FIPR/FSCA/FSRRA/transform) sobre o SW; os 13 `engine/dc/ninja/*.c`
    compilam sem mudança.
  - `ports/zeebo/ninja/selftest.c` (host) valida identidade, translação,
    push/pop, equivalência XMTRX×memória, round-trip set/get e
    `njMultiMatrix(NULL, A)`: **PASSED**.
  - **T&L de hardware provado** (`ports/zeebo/zcube.c`): vértices em espaço de
    objeto e `GL_FIXED`, matriz do Ninja convertida para 16.16 e
    `glMultMatrixx`; o Adreno 130 transforma. Nada de projeção por vértice no
    CPU. Isso substitui a necessidade de portar `zam_xmtrx_project_points`
    para o caminho geral.
  - Shims derivados do upstream (`sa1/sprite.h`, njsprite.h) são gerados no
    build por `ports/zeebo/tools/gen_sprite_shim.py`; o repo não versiona o texto do
    decomp.
  - **Sprites 2D reais** (`zsprite.c`): `njDrawSprite2D` do decomp
    (`engine/dc/njSprite.c` + `sa1/sprite.c`) desenha no zeebx. A única
    costura trocada é `sprite_send_corner` (escrita na store queue) pelo
    sink; o resto da matemática de sprite/clip é do decomp, intacto.
  - Autocontido: `brew_egl`, `string` e `softfloat` foram copiados para
    `ports/zeebo/platform/`; o build não referencia o `src/platform/` antigo.
  - Falta: kernels de `batch.h` só onde o jogo precisar de resultado no CPU
    (contagem de near-clip, paletas), `sa1/model.c`/`easy.h`, e a composição
    do jogo (`config/zeebo.mk`).
- **Z3 (levantado)** — decodificação:
  - **Textura PVR/PVRT: feito.** `ports/zeebo/assets/pvr.c` (cópia nossa,
    autocontida) decodifica twiddled/VQ/small VQ/rectangle e ARGB1555/
    RGB565/ARGB4444. Cobre os 441 PVRs do jogo (0 paletizados). Provado no
    zeebx com `SSTX_BODY.PVR` do disco (quads, sprite e banda com a textura
    real).
  - **PRS/PVM: feito.** `ports/zeebo/assets/{prs,pvm,pvr}.c` são autocontidos.
    No host, `FROG.prs` -> 63264 B -> 8 texturas (64x64/32x32). No device, o
    `.mod` embute `FROG.prs` e desenha a cadeia PRS->PVM->PVRT->GLES1 (a
    textura verde do sapo nos quads e no sprite).
  - **ADX: decodifica no alvo.** `adx_dcd/dcd2/dcda/xpnd/qtbl` compilados
    para ARM; `FISHMISS.ADX` (estéreo 32 kHz, 156653 amostras) decodifica e
    a forma de onda aparece no zeebx.
  - **Saída de áudio: parcial.** O trecho decodificado vira WAV em memória e
    toca via `IMedia` (`MMD_BUFFER` + `IMEDIA_Play`). O zeebx captura o som
    (`--dump-audio` mostra o burst), mas só ~0,1 s do clipe de 0,5 s foi
    capturado — falta acertar duração/streaming e o ritmo do dump.
  - **Vídeo SFD**: o decomp traz `sdk/sofdec/sj_*` (MPEG) e `mw_*`
    (demux/ADPCM). Compilar para ARM e apresentar quadro como textura
    GLES1; é o maior bloco.
  - **Arquivos**: trocar GDFS por BREW FS (`IFileMgr`).
- **Z4** — performance (soft-float, listas, frame pacing) e pacote `.mod`.

## Como construir/rodar Z0

```sh
make -C ports/zeebo            # build/zeebo/sa1z.mod + .mif
~/.local/bin/zeebx run build/zeebo/sa1z.mod --seconds=3 --dump-gl=/tmp/z0 --trace
```

## Notas de implementação

- O host x86-64 LP64 **não serve** para validar o sofdec: `Sint32` é `long`,
  então o layout de `ADX_HDR` muda. A validação é no alvo (ARM32) ou com
  toolchain `-m32`.
- No passe 2D o depth test fica desligado (ordem de desenho); o cubo 3D liga
  o seu com clear próprio.

## Riscos

- ARM11 **sem FPU**: float em soft-float é caro. O Adreno 130 tem **T&L de
  hardware** (sem iluminação), então o desenho 3D deve mandar a matriz em
  `GLfixed` e deixar o vertex engine transformar, em vez de portar os kernels
  de projeção do `batch.h` para rodar por vértice no CPU. O CPU fica com
  matriz (uma por objeto) e, quando o jogo exigir, iluminação.
- `batch.h`/SH4ZAM não compilam fora do SH-4: a substituição portátil é
  obrigatória, não opcional.
- Sem GD-ROM: assets precisam de pipeline (BREW FS / NAND / SD) e o jogo
  original do Zeebo é outra build (reduzida); não assumir equivalência.
- `sa1hello`/`brew_egl` já mostram que EGL/IGLES11 funcionam, mas só para
  cenas pequenas.
