# ============================================================================
# O C do jogo (decomp) compilado para ARM32 — objetos do 1ST_READ.
#
#   make -f game.mk -j8        compila e faz o link parcial (game.o)
#   make -f game.mk indef      lista os símbolos ainda indefinidos
#
# Fontes: config/1st_read/splits.txt do decomp (src/, modules/, events/, sdk/
# exceto sdk/shinobi, que mexe em registrador do SH-4 e vira shim) mais
# build/gen/1st_read/content.c. Os 34 endereços fixos de RAM passam por
# tools/gen_rebase.py (ZDC) antes de compilar.
# ============================================================================

ROOT     := $(abspath $(CURDIR)/../..)
-include $(CURDIR)/local.mk
UPSTREAM := $(ROOT)
SHZAM    ?= $(ROOT)/tools/sh4zam
OUT      := $(ROOT)/build/zeebo/game
RB       := $(OUT)/rebased
CC       := arm-none-eabi-gcc
LD       := arm-none-eabi-ld
NM       := arm-none-eabi-nm

SPLITS   := $(UPSTREAM)/config/1st_read/splits.txt
# C do jogo que nenhum SHC compila (config/compilers.mk: NONMATCHING_C): frame,
# present, port, skip e o model.c que substitui model.src
EXTRA_C  := src/system/pace.c src/system/port.c src/system/present.c \
            src/event/skip.c
ALL_C    := $(EXTRA_C) $(shell awk '!/^[#]/ && NF {print $$1}' $(SPLITS) | grep '\.c$$' | grep -v '^gen/' | grep -v '^sdk/shinobi/' | sort -u)
REBASE_F := src/stage/stage.c src/event/play.c src/object/download.c \
            src/sound/stream.c src/progress/recorder.h src/progress/recorder.c \
            src/system/loader.h src/system/loader.c src/debug/host.c \
            modules/movie/movie.c modules/shooting/shooting.c \
            events/shared/sound.h events/ev0045/script.c
REBASE_C := $(filter %.c,$(REBASE_F))

INC := -I$(RB)/src -I$(RB)/modules -I$(RB)/events \
       -I$(UPSTREAM)/sdk -I$(UPSTREAM)/src -I$(UPSTREAM)/modules -I$(UPSTREAM)/events \
       -I$(UPSTREAM)/build/gen/src -I$(UPSTREAM)/build/gen/modules \
       -I$(UPSTREAM)/build/gen/events -I$(UPSTREAM)/build/gen/sdk \
       -I$(UPSTREAM)/engine/include -I$(UPSTREAM)/engine/dc -I$(UPSTREAM)/engine/sa1 \
       -I$(UPSTREAM)/engine/native -I$(UPSTREAM)/engine/numeric -I$(SHZAM)/include \
       -I$(UPSTREAM) -I$(CURDIR) -idirafter $(UPSTREAM)/sdk/include

CFLAGS := -std=gnu89 -march=armv6 -mcpu=arm1136j-s -mfloat-abi=soft -Os \
          -ffunction-sections -fdata-sections -fcommon -ffreestanding -nostdinc \
          -isystem $(shell $(CC) -print-file-name=include) \
          -DNONMATCHING -DSHZ_TLS_MODEL=0 -DINFINITY=__builtin_inff\(\) -fms-extensions -ffp-contract=off -fno-strict-aliasing \
          -fwrapv -w -include zrebase.h $(INC)

OBJS := $(patsubst %.c,$(OUT)/obj/%.o,$(ALL_C)) $(OUT)/obj/content.o

# shims do slice nativo que já compilam em ARM, e o asm do SH-4 em C (asm2c.py)
SHIMS := engine/native/loads engine/native/names \
         engine/native/pads engine/native/settings engine/native/vibration \
         engine/sa1/hooks
OBJS += $(patsubst %,$(OUT)/obj/%.o,$(SHIMS)) $(OUT)/obj/asm.o

$(OUT)/obj/asm.o: $(UPSTREAM)/build/engine/native/asm.c
	@mkdir -p $(@D)
	@$(CC) $(filter-out -std=gnu89 -nostdinc,$(CFLAGS)) -std=gnu99 -D_GNU_SOURCE -DNATIVE_DOMAIN=0 \
	    -include engine/native/prelude.h -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU asm.c"; rm -f $@; exit 1; }

# os shims do host usam libc: headers do newlib (o SDK do jogo fica por último)
SHIM_CFLAGS := $(filter-out -std=gnu89 -nostdinc -isystem%,$(CFLAGS)) -std=gnu99 -D_GNU_SOURCE \
               -DNATIVE_DOMAIN=0 -include engine/native/prelude.h
$(patsubst %,$(OUT)/obj/%.o,$(SHIMS)): $(OUT)/obj/%.o: $(UPSTREAM)/%.c
	@mkdir -p $(@D)
	@$(CC) $(SHIM_CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU $*.c"; rm -f $@; exit 1; }

$(OUT)/obj/sdk/ninja/njModelE.o: CFLAGS += -std=gnu99 -Dsqrtf=__builtin_sqrtf -Dfabsf=__builtin_fabsf

all: $(OUT)/game.o

$(RB)/.done: $(CURDIR)/tools/gen_rebase.py
	@mkdir -p $(RB)
	@python3 $(CURDIR)/tools/gen_rebase.py $(UPSTREAM) $(RB) $(REBASE_F) >/dev/null
	@touch $@

# fontes reescritas pelo rebase vêm de $(RB); as outras, do decomp
$(addprefix $(OUT)/obj/,$(REBASE_C:.c=.o)): $(OUT)/obj/%.o: $(RB)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(RB)/$*.c || { echo "FALHOU $*.c"; rm -f $@; exit 1; }

$(OUT)/obj/content.o: $(UPSTREAM)/build/gen/1st_read/content.c $(RB)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OUT)/obj/%.o: $(UPSTREAM)/%.c $(RB)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU $*.c"; rm -f $@; exit 1; }

# link parcial só dos objetos que compilaram
# tolera objetos que não compilaram (lista em falhas.txt): rode com -k
$(OUT)/game.o: $(OBJS)
	@$(LD) -r -o $@ $(wildcard $(OBJS)) 2>$(OUT)/link.err || { head $(OUT)/link.err; exit 1; }
	@echo "game.o: $$(ls $(wildcard $(OBJS)) | wc -l) de $(words $(OBJS)) objetos"

link:
	@$(MAKE) -f game.mk -k -j12 all >/dev/null 2>&1; \
	 $(LD) -r -o $(OUT)/game.o $$(ls $(OBJS) 2>/dev/null) && \
	 echo "game.o: $$(ls $(OBJS) 2>/dev/null | wc -l) de $(words $(OBJS)) objetos"
	@for o in $(OBJS); do [ -f $$o ] || echo $${o#$(OUT)/obj/}; done > $(OUT)/falhas.txt; cat $(OUT)/falhas.txt

indef:
	@$(NM) -u $(OUT)/game.o | awk '{print $$2}' | sort -u > $(OUT)/undef.txt
	@wc -l < $(OUT)/undef.txt

.PHONY: all indef link
.DELETE_ON_ERROR:
