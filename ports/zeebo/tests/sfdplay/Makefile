# sfdplay: o ARM do Zeebo decodifica os filmes Sofdec (MPEG-1) do SA1? (ver sfdplay.c)
#
#   make           -> build/zeebo/sfdplay/{sfdplay.mod,sfdplay.mif,sa1_600.sfd}
#
# sem áudio: probe.mp4 (MPEG-4 simple profile) e probe264.mp4 (H.264 baseline).
ROOT   := $(abspath $(CURDIR)/../../../..)
-include $(CURDIR)/../../local.mk
VENDOR ?= $(abspath $(ROOT)/../zeebo_demoscene/vendor)
OUT    := $(ROOT)/build/zeebo/sfdplay
CC     := arm-none-eabi-gcc
LD     := arm-none-eabi-ld
SFD    ?= /tmp/opencode/sa1-intro-candidate/SA1_600.SFD

GEN := $(VENDOR)/zeebo/samples/samples/conftest_source/conftest
CFLAGS := -c -Os -fno-builtin -ffreestanding -nostdinc -fno-common -ffunction-sections \
          -march=armv6 -mcpu=arm1136j-s -mapcs-frame -mno-thumb-interwork \
          -DDYNAMIC_APP -DBREW_STATIC_APP -DAEE_SIMULATOR=0 -D'__declspec(x)=' -Wall \
          -I$(OUT) -I$(CURDIR)/inc -I$(VENDOR)/shadow40/inc -I$(VENDOR)/inc -I$(VENDOR)/brew40/sdk/inc \
          -I$(shell $(CC) -print-file-name=include)
LIBGCC := $(dir $(shell $(CC) -march=armv6 -mcpu=arm1136j-s -print-libgcc-file-name))
OBJS := $(OUT)/sfdplay.o $(OUT)/AEEModGen.o $(OUT)/AEEAppGen.o

all: $(OUT)/sfdplay.mod $(OUT)/sfdplay.mif $(OUT)/sa1_600.sfd

$(OUT):
	@mkdir -p $@
$(OUT)/sfdplay.o: sfdplay.c $(OUT)/pl_mpeg.h | $(OUT)
	$(CC) $(CFLAGS) -o $@ $<
$(OUT)/%.o: $(GEN)/%.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $<
$(OUT)/sfdplay.elf: $(OBJS)
	$(LD) --emit-relocs -T $(VENDOR)/toolset/bin/elf2mod/src/gnu/elf2mod.x -o $@ $(OBJS) -L$(LIBGCC) -lgcc
$(OUT)/sfdplay.mod: $(OUT)/sfdplay.elf
	WINEDEBUG=-all wine $(VENDOR)/toolset/bin/elf2mod.exe -output $@ $<
$(OUT)/sfdplay.mif: ../../tools/gen_mif.py | $(OUT)
	cd $(OUT) && python3 $(abspath ../../tools/gen_mif.py) sfdplay.mif 0x0BE07005
# pl_mpeg (MIT) baixado no build; não vai no git
$(OUT)/pl_mpeg.h: | $(OUT)
	curl -sSL -o $@ https://raw.githubusercontent.com/phoboslab/pl_mpeg/master/pl_mpeg.h
# o filme do disco, por hardlink (o zeebx ignora symlinks)
$(OUT)/sa1_600.sfd: | $(OUT)
	ln -f $(ROOT)/build/zeebo/sa1_600.sfd $@
