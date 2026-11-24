# ============================================================================
# Z0 — applet Zeebo que exercita o sink GLES1 do SA1 (src/zeebo).
#
#   make            constroi ../../build/zeebo/sa1z.mod + .mif
#   make limpar     remove os objetos do Z0
#
# Mesmo pipeline validado do sa1hello: arm-none-eabi-gcc -> elf2mod (wine) ->
# .mod, rodando no zeebx standalone com IGLES11.
# ============================================================================

ROOT    := $(abspath $(CURDIR)/../..)
-include $(CURDIR)/local.mk
VENDOR  ?= $(abspath $(ROOT)/../zeebo_demoscene/vendor)
BUILD   := $(ROOT)/build/zeebo
SRC     := $(CURDIR)

UPSTREAM := $(ROOT)
SHZAM    ?= $(ROOT)/tools/sh4zam

APP     := sa1z
CC      := arm-none-eabi-gcc
LD      := arm-none-eabi-ld
NM      := arm-none-eabi-nm

ifeq ($(wildcard $(VENDOR)/toolset/bin/elf2mod/src/gnu/elf2mod.x),)
$(error VENDOR vazio ou SDK ausente em $(VENDOR). Veja docs/zeebo-port-plan.md)
endif

GEN_DIR := $(VENDOR)/zeebo/samples/samples/conftest_source/conftest
GEN_SRC := $(GEN_DIR)/AEEModGen.c $(GEN_DIR)/AEEAppGen.c
SDK_SRC := $(VENDOR)/sdk40/src/EGL_1x.c $(VENDOR)/sdk40/src/GLES_1x.c \
           $(VENDOR)/sdk40/src/GLES_ext.c

ifdef ZBENCH
CFLAGS_BENCH := -DZBENCH=$(ZBENCH)
endif
OWN_SRC := $(if $(ZBENCH),$(SRC)/zbench.c) $(SRC)/app.c $(SRC)/zstrip.c $(SRC)/zfile.c $(SRC)/zrebase.c $(SRC)/zthreads.c $(SRC)/zlibc.c $(SRC)/zkamui.c $(SRC)/ztexture.c
PLAT_SRC := $(SRC)/platform/brew_egl.c

# os 13 engine/dc/ninja/*.c reais do non-matching (C puro sobre SH4ZAM SW)
# decodificador ADX do decomp (C portavel; long=32 no alvo)
ADX_SRC := adx_dcd adx_dcd2 adx_dcda adx_xpnd adx_qtbl
ADX_OBJ := $(patsubst %,$(BUILD)/%.o,$(ADX_SRC))

NINJA_MATH_SRC := $(wildcard $(UPSTREAM)/engine/dc/ninja/*.c)
NINJA_MATH_OBJ := $(patsubst $(UPSTREAM)/engine/dc/ninja/%.c,$(BUILD)/njm-%.o,$(NINJA_MATH_SRC))

OBJS := $(patsubst $(SRC)/%.c,$(BUILD)/%.o,$(OWN_SRC)) \
        $(BUILD)/zninja.o $(BUILD)/zcube.o $(BUILD)/zsprite.o $(BUILD)/zsys.o  $(BUILD)/njDraw.o \
        $(BUILD)/njSprite.o $(BUILD)/nj_sprite.o $(BUILD)/njkm_zeebo.o \
        $(BUILD)/port_shim.o $(BUILD)/nj_globals.o $(BUILD)/shz_xmtrx_sw.o \
        $(BUILD)/softfloat.o $(BUILD)/string.o $(BUILD)/fmaf.o \
        $(BUILD)/pvr.o $(BUILD)/pvm.o $(BUILD)/prs.o \
        $(BUILD)/zaudio.o $(BUILD)/zmix.o $(BUILD)/zinst.o $(BUILD)/zhid.o $(BUILD)/zboom.o $(BUILD)/zhud.o $(BUILD)/zgdfs.o $(BUILD)/libm.o $(ADX_OBJ) \
        $(BUILD)/asset_pvr.o $(NINJA_MATH_OBJ) \
        $(BUILD)/platform-brew_egl.o \
        $(BUILD)/AEEModGen.o $(BUILD)/AEEAppGen.o \
        $(BUILD)/sdk-egl-1x.o $(BUILD)/sdk-gles-1x.o $(BUILD)/sdk-gles-ext.o


LDSCRIPT := $(VENDOR)/toolset/bin/elf2mod/src/gnu/elf2mod.x
ELF2MOD  := $(VENDOR)/toolset/bin/elf2mod.exe

INCS := -I$(SRC) -I$(VENDOR)/shadow40/inc -I$(VENDOR)/inc \
        -I$(VENDOR)/brew40/sdk/inc -I$(VENDOR)/brew40/sdk/inc/gles/gles_1_1 \
        -I$(shell $(CC) -print-file-name=include)

# AUDIO=1 liga o som; desligado por padrão (o IMedia/decodificação ADX pesa
# no Zeebo enquanto a performance é medida)
AUDIO ?= 0
# DYNRES=50|75: o 3D do jogo em 50% ou 75% de 640x480, esticado; o 2D (HUD,
# menus) em resolucao cheia. Padrao 0 (desligado). O make nao rastreia a flag:
# ao trocar, apague build/zeebo/zstrip.o (e o do jogo, se houver).
DYNRES ?= 0
# FPSHUD=1: overlay de FPS / ms de jogo / RAM livre no canto (zhud.c); padrao 0.
# O make nao rastreia a flag: ao trocar, apague build/zeebo/zhud.o e app.o.
FPSHUD ?= 0
CFLAGS_FPSHUD := -DZ_FPSHUD=$(FPSHUD)
# STATECACHE=0 desliga o cache de estado GL do passe translucido (zstrip.c); padrao 1.
STATECACHE ?= 1
CFLAGS_STATECACHE := -DZ_STATECACHE=$(STATECACHE)
# TEX16=1 envia as texturas em 16 bpp sem perda (565/5551/4444) em vez de RGBA8888; padrao 0.
TEX16 ?= 0
CFLAGS_TEX16 := -DZ_TEX16=$(TEX16)
CFLAGS_DYNRES := -DZ_DYNRES=$(DYNRES)
ifeq ($(AUDIO),0)
CFLAGS_AUDIO := -DZAUDIO_OFF
endif
CFLAGS := $(CFLAGS_BENCH) $(CFLAGS_AUDIO) $(CFLAGS_DYNRES) $(CFLAGS_FPSHUD) $(CFLAGS_STATECACHE) $(CFLAGS_TEX16) -c -Os -fno-builtin -ffreestanding -nostdinc -fno-common \
          -ffunction-sections \
          -march=armv6 -mcpu=arm1136j-s -mapcs-frame -mno-thumb-interwork \
          -DDYNAMIC_APP -DBREW_STATIC_APP -DAEE_SIMULATOR=0 \
          -D'__declspec(x)=' \
          -Wall -Wno-unused-function \
          $(INCS)


# Recorte do decomp que o Z1 compila: njDraw.c real + os shims da costura.
SHIM_INC := $(BUILD)/include
DCINC := -I$(SRC)/ninja -I$(SHIM_INC) -I$(SHZAM)/include -I$(UPSTREAM)/sdk/include \
         -I$(UPSTREAM)/engine/dc -I$(UPSTREAM)/engine/sa1 \
         -I$(UPSTREAM)/engine/include -I$(UPSTREAM)/src -I$(UPSTREAM)
DCFLAGS := $(CFLAGS) -DNDEBUG -ffp-contract=off -DSHZ_TLS_MODEL=0 -DDREAMCAST -DNONMATCHING $(DCINC)

LDFLAGS := --emit-relocs -T $(LDSCRIPT)

LIBGCC := $(dir $(shell $(CC) -print-libgcc-file-name))
LIBGCC_MULTILIB := $(dir $(shell $(CC) -march=armv6 -mcpu=arm1136j-s \
                                  -print-libgcc-file-name))
LDLIBS := -L$(LIBGCC_MULTILIB) -L$(LIBGCC) -lgcc

TARGET := $(BUILD)/$(APP).mod
MIF    := $(BUILD)/$(APP).mif

.SECONDARY:

.DEFAULT_GOAL := tudo
.PHONY: tudo limpar host-selftest

tudo: $(TARGET) $(MIF)

$(BUILD):
	@mkdir -p $(BUILD)

# HOTOPT: nivel de otimizacao dos arquivos quentes do desenho (zstrip.c, zkamui.c);
# o resto do port fica em -Os. -O2 e o padrao: -1,8% de instrucoes no zeebx, imagem identica; -O3 nao passa do -O2.
HOTOPT ?= -O2
HOTFILES ?= zstrip zkamui
$(addprefix $(BUILD)/,$(addsuffix .o,$(HOTFILES))): CFLAGS += $(HOTOPT)

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	@echo "  CC   $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/zninja.o: $(SRC)/zninja.c | $(BUILD)
	@echo "  CC   zninja.c (DREAMCAST+NONMATCHING)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/zcube.o: $(SRC)/zcube.c | $(BUILD)
	@echo "  CC   zcube.c (T&L de hardware)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

# shims gerados do upstream (o repo guarda só o gerador, não o texto)
$(SHIM_INC)/sa1/sprite.h: $(UPSTREAM)/engine/dc/sa1/sprite.h $(CURDIR)/tools/gen_sprite_shim.py | $(BUILD)
	@mkdir -p $(SHIM_INC)/sa1
	@python3 $(CURDIR)/tools/gen_sprite_shim.py $< $@

$(SHIM_INC)/engine/dc/njsprite.h: $(UPSTREAM)/engine/dc/njsprite.h | $(BUILD)
	@mkdir -p $(SHIM_INC)/engine/dc
	@cp $< $@

$(BUILD)/njSprite.o: $(UPSTREAM)/engine/dc/njSprite.c $(SHIM_INC)/sa1/sprite.h $(SHIM_INC)/engine/dc/njsprite.h | $(BUILD)
	@echo "  CC   njSprite.c (decomp)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/nj_sprite.o: $(SRC)/ninja/nj_sprite.c | $(BUILD)
	@echo "  CC   nj_sprite.c (estado do Ninja)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/zgdfs.o: $(SRC)/zgdfs.c | $(BUILD)
	@echo "  CC   zgdfs.c (GDFS sobre IFileMgr)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/zsys.o: $(SRC)/zsys.c | $(BUILD)
	@echo "  CC   zsys.c (timer e pad do Shinobi)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/zsprite.o: $(SRC)/zsprite.c | $(BUILD)
	@echo "  CC   zsprite.c (njDrawSprite2D)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/njDraw.o: $(UPSTREAM)/sdk/ninja/njDraw.c | $(BUILD)
	@echo "  CC   njDraw.c (decomp, DREAMCAST+NONMATCHING)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/njkm_zeebo.o: $(SRC)/ninja/njkm_zeebo.c | $(BUILD)
	@echo "  CC   njkm_zeebo.c (shim)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/nj_globals.o: $(SRC)/ninja/nj_globals.c | $(BUILD)
	@echo "  CC   nj_globals.c (estado do Ninja)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/njm-%.o: $(UPSTREAM)/engine/dc/ninja/%.c | $(BUILD)
	@echo "  CC   ninja/$*.c (decomp, SH4ZAM SW)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/shz_xmtrx_sw.o: $(SHZAM)/source/sw/shz_xmtrx_sw.c | $(BUILD)
	@echo "  CC   shz_xmtrx_sw.c (SH4ZAM SW)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

# assets de exemplo: gerados do disco local do usuário, nunca versionados.
# o build sem o disco gera arrays vazios e a cena cai no checker.
PVR_ASSET ?= $(ROOT)/extract/pvr/SSTX_BODY.PVR;1
PRS_ASSET ?= $(ROOT)/extract/prs/FROG.prs
ADX_ASSET ?= $(ROOT)/extract/adx/FISHMISS.ADX;1

# o ';1' do nome não pode aparecer num pré-requisito do make: copia antes
$(BUILD)/asset_pvr.bin: | $(BUILD)
	@if [ -f "$(PVR_ASSET)" ]; then cp "$(PVR_ASSET)" $@; else : > $@; fi

# PRS/ADX não vão no .mod: ficam ao lado dele, lidos via IFileMgr em runtime
DATA_FILES := $(BUILD)/FROG.prs $(BUILD)/FISHMISS.adx
tudo: $(DATA_FILES)

$(BUILD)/FROG.prs: | $(BUILD)
	@if [ -f "$(PRS_ASSET)" ]; then cp "$(PRS_ASSET)" $@; fi

$(BUILD)/FISHMISS.adx: | $(BUILD)
	@if [ -f "$(ADX_ASSET)" ]; then cp "$(ADX_ASSET)" $@; fi

$(BUILD)/asset_%.c: $(BUILD)/asset_%.bin
	@python3 -c 'import sys,os;n="z"+os.path.basename(sys.argv[2]).split(".")[0];d=open(sys.argv[1],"rb").read();o=open(sys.argv[2],"w");o.write("const unsigned char %s[]={"%n+",".join(str(b) for b in d)+"};\nconst unsigned int %s_len=%d;\n"%(n,len(d)))' $< $@

$(BUILD)/asset_%.o: $(BUILD)/asset_%.c | $(BUILD)
	@echo "  CC   $*.c (asset do disco)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/adx_%.o: $(UPSTREAM)/sdk/sofdec/adx_%.c | $(BUILD)
	@echo "  CC   sofdec/adx_$*.c (ADX)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/libm.o: $(SRC)/ninja/libm.c | $(BUILD)
	@echo "  CC   libm.c (sqrt/cos double)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/zhid.o: $(SRC)/zhid.c $(SRC)/zhid.h $(SRC)/zboom.h | $(BUILD)
	@echo "  CC   zhid.c (controle pelo IHID)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/zboom.o: $(SRC)/zboom.c $(SRC)/zboom.h | $(BUILD)
	@echo "  CC   zboom.c (Boomerang)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/zhud.o: $(SRC)/zhud.c $(SRC)/zstrip.h | $(BUILD)
	@echo "  CC   zhud.c (overlay de FPS)"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/zaudio.o: $(SRC)/zaudio.c | $(BUILD)
	@echo "  CC   zaudio.c (ADX -> onda)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/zmix.o: $(SRC)/zmix.c $(SRC)/zmix.h | $(BUILD)
	@echo "  CC   zmix.c (mixer de som)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/zinst.o: $(SRC)/zinst.c $(SRC)/zstrip.h | $(BUILD)
	@echo "  CC   zinst.c (instrumentacao)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/pvm.o: $(SRC)/assets/pvm.c | $(BUILD)
	@echo "  CC   pvm.c (decoder PVMH)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/prs.o: $(SRC)/assets/prs.c | $(BUILD)
	@echo "  CC   prs.c (decoder LZ77)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/pvr.o: $(SRC)/assets/pvr.c | $(BUILD)
	@echo "  CC   pvr.c (decoder PVRT)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/fmaf.o: $(SRC)/ninja/fmaf.c | $(BUILD)
	@echo "  CC   fmaf.c (__builtin_fmaf)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/string.o: $(SRC)/platform/string.c | $(BUILD)
	@echo "  CC   string.c (memset)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/softfloat.o: $(SRC)/platform/softfloat.c | $(BUILD)
	@echo "  CC   softfloat.c (sinf/cosf/sqrtf)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/port_shim.o: $(SRC)/ninja/port_shim.c | $(BUILD)
	@echo "  CC   port_shim.c (port.h minimo)"
	@$(CC) $(DCFLAGS) -c -o $@ $<

$(BUILD)/platform-brew_egl.o: $(SRC)/platform/brew_egl.c | $(BUILD)
	@echo "  CC   brew_egl.c (platform)"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/AEEModGen.o: $(GEN_DIR)/AEEModGen.c | $(BUILD)
	@echo "  CC   AEEModGen.c (SDK)"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/AEEAppGen.o: $(GEN_DIR)/AEEAppGen.c | $(BUILD)
	@echo "  CC   AEEAppGen.c (SDK)"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/sdk-egl-1x.o: $(VENDOR)/sdk40/src/EGL_1x.c | $(BUILD)
	@echo "  CC   EGL_1x.c (SDK)"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/sdk-gles-1x.o: $(VENDOR)/sdk40/src/GLES_1x.c | $(BUILD)
	@echo "  CC   GLES_1x.c (SDK)"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/sdk-gles-ext.o: $(VENDOR)/sdk40/src/GLES_ext.c | $(BUILD)
	@echo "  CC   GLES_ext.c (SDK)"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/$(APP).elf: $(OBJS) $(LDSCRIPT)
	@echo "  LD   $@"
	@$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(TARGET): $(BUILD)/$(APP).elf $(ELF2MOD)
	@echo "  MOD  $@"
	@WINEDEBUG=-all wine $(ELF2MOD) -output $@ $<
	@echo "  ---- $$($(NM) $< | grep -E ' T AEEMod_Load')"

$(MIF): $(CURDIR)/tools/gen_mif.py | $(BUILD)
	@python3 $(CURDIR)/tools/gen_mif.py $@ 0x0BE07003

limpar:
	@rm -rf $(BUILD)

# valida a matemática do Ninja no host (x86) contra os 13 .c reais
HOSTCC ?= cc
host-selftest: | $(BUILD)
	@$(HOSTCC) -O2 -DDREAMCAST -DNONMATCHING -DNDEBUG -I$(SRC)/ninja \
	    -I$(SHZAM)/include -idirafter $(UPSTREAM)/sdk/include \
	    -idirafter $(UPSTREAM)/engine/dc -idirafter $(UPSTREAM)/engine/sa1 \
	    -idirafter $(UPSTREAM)/engine/include -idirafter $(UPSTREAM)/src \
	    -I$(UPSTREAM)/engine/numeric -I$(UPSTREAM)/build/zeebo/game/gen \
	    $(SRC)/ninja/selftest.c $(UPSTREAM)/engine/dc/ninja/*.c \
	    $(UPSTREAM)/engine/numeric/fast.c \
	    $(SHZAM)/source/sw/shz_xmtrx_sw.c -lm -o $(BUILD)/ninja_selftest
	@$(BUILD)/ninja_selftest

# game.mk compila os arquivos de desenho do jogo no sabor Dreamcast com estas flags
dcflags:
	@echo $(DCFLAGS)
# a lista de objetos do applet, que game.mk linka com o jogo
objs:
	@echo $(OBJS)
.PHONY: dcflags objs
