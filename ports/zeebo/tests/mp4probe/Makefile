# mp4probe: o Zeebo toca MPEG-4 / H.264 pelo IMedia? (ver mp4probe.c)
#
#   make           -> build/zeebo/mp4probe/{mp4probe.mod,mp4probe.mif,probe*.mp4}
#
# Os vídeos de teste saem de um .SFD do disco (SFD=caminho), 10 s a 320x240,
# sem áudio: probe.mp4 (MPEG-4 simple profile) e probe264.mp4 (H.264 baseline).
ROOT   := $(abspath $(CURDIR)/../../../..)
-include $(CURDIR)/../../local.mk
VENDOR ?= $(abspath $(ROOT)/../zeebo_demoscene/vendor)
OUT    := $(ROOT)/build/zeebo/mp4probe
CC     := arm-none-eabi-gcc
LD     := arm-none-eabi-ld
SFD    ?= /tmp/opencode/sa1-intro-candidate/SA1_600.SFD

GEN := $(VENDOR)/zeebo/samples/samples/conftest_source/conftest
CFLAGS := -c -Os -fno-builtin -ffreestanding -nostdinc -fno-common -ffunction-sections \
          -march=armv6 -mcpu=arm1136j-s -mapcs-frame -mno-thumb-interwork \
          -DDYNAMIC_APP -DBREW_STATIC_APP -DAEE_SIMULATOR=0 -D'__declspec(x)=' -Wall \
          -I$(VENDOR)/shadow40/inc -I$(VENDOR)/inc -I$(VENDOR)/brew40/sdk/inc \
          -I$(shell $(CC) -print-file-name=include)
LIBGCC := $(dir $(shell $(CC) -march=armv6 -mcpu=arm1136j-s -print-libgcc-file-name))
OBJS := $(OUT)/mp4probe.o $(OUT)/AEEModGen.o $(OUT)/AEEAppGen.o

all: $(OUT)/mp4probe.mod $(OUT)/mp4probe.mif $(OUT)/probe.mp4 $(OUT)/probe264.mp4

$(OUT):
	@mkdir -p $@
$(OUT)/mp4probe.o: mp4probe.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $<
$(OUT)/%.o: $(GEN)/%.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $<
$(OUT)/mp4probe.elf: $(OBJS)
	$(LD) --emit-relocs -T $(VENDOR)/toolset/bin/elf2mod/src/gnu/elf2mod.x -o $@ $(OBJS) -L$(LIBGCC) -lgcc
$(OUT)/mp4probe.mod: $(OUT)/mp4probe.elf
	WINEDEBUG=-all wine $(VENDOR)/toolset/bin/elf2mod.exe -output $@ $<
$(OUT)/mp4probe.mif: ../../tools/gen_mif.py | $(OUT)
	cd $(OUT) && python3 $(abspath ../../tools/gen_mif.py) mp4probe.mif 0x0BE07004
$(OUT)/probe.mp4: | $(OUT)
	nice ffmpeg -v error -y -ss 5 -t 10 -i "$(SFD)" -an -c:v mpeg4 -profile:v 0 -level 3 \
	    -b:v 384k -r 15 -s 320x240 -pix_fmt yuv420p $@
$(OUT)/probe264.mp4: | $(OUT)
	nice ffmpeg -v error -y -ss 5 -t 10 -i "$(SFD)" -an -c:v libx264 -profile:v baseline \
	    -level 1.3 -b:v 384k -r 15 -s 320x240 -pix_fmt yuv420p $@
