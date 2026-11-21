# Port Zeebo (BREW/ARM11/GLES 1.1)

Port do Sonic Adventure para o Zeebo a partir deste decomp. Estado, marcos e
riscos: [docs/zeebo-port-roadmap.md](docs/zeebo-port-roadmap.md); plano em
[docs/zeebo-port-plan.md](docs/zeebo-port-plan.md).

O que roda hoje no zeebx (`zeebx run build/zeebo/sa1z.mod`): sprites e quads
do `njSprite.c`/`njDraw.c` reais do decomp sobre GLES 1.1, texturas PVM por
texlist/texid, um cubo 3D, ADX decodificado e tocado pelo IMedia, assets lidos
por IFileMgr através de um shim de GDFS. O jogo em si ainda não está linkado:
`game.mk` compila o 1ST_READ para ARM32 (500 de 503 objetos) e lista o que falta.

## Pré-requisitos (nada disso está no repositório)

- `arm-none-eabi-gcc`, `python3`, `make`, e o `zeebx` para executar.
- SDK Zeebo/BREW (`VENDOR=`, padrão `../zeebo_demoscene/vendor`): toolset
  `elf2mod`, `sdk40/src/EGL_1x.c` etc.
- SH4ZAM em `tools/sh4zam` (ou `SHZAM=`).
- Assets do **seu** disco, fora do git: `PRS_ASSET=`, `ADX_ASSET=`, `PVR_ASSET=`.
- Para `game.mk`: `make pc` do decomp (gera `build/gen/`, a partir do disco).

## Uso

    make -C ports/zeebo                  # build/zeebo/sa1z.mod + .mif
    make -C ports/zeebo host-selftest    # matemática do Ninja no host
    make -C ports/zeebo -f game.mk link  # 1ST_READ em ARM32 (game.o parcial)

## Layout

- `app.c`, `z*.c`: applet de bring-up (sink GLES1, áudio, arquivos, GDFS,
  threads, arena de RAM do DC).
- `ninja/`, `assets/`, `platform/`: shims do Ninja, PRS/PVM/PVR, EGL/BREW.
- `tools/`: `gen_rebase.py` (endereços fixos -> arena), `gen_sprite_shim.py`,
  `gen_mif.py`. O texto do upstream nunca é copiado para o repositório.
- `game.mk`: compila o C do jogo para ARM32.

Nenhum asset, ROM ou `1ST_READ.BIN` faz parte deste repositório.
