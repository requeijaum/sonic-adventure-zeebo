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
PT       := $(OUT)/patched
VENDOR   ?= $(abspath $(ROOT)/../zeebo_demoscene/vendor)
CC       := arm-none-eabi-gcc
LD       := arm-none-eabi-ld
NM       := arm-none-eabi-nm

SPLITS   := $(UPSTREAM)/config/1st_read/splits.txt
# C do jogo que nenhum SHC compila (config/compilers.mk: NONMATCHING_C): frame,
# present, port, skip e o model.c que substitui model.src
EXTRA_C  := sdk/shinobi/vmsutil.c src/system/pace.c src/system/port.c src/system/present.c \
            src/event/skip.c
ALL_C    := $(EXTRA_C) $(shell awk '!/^[#]/ && NF {print $$1}' $(SPLITS) | grep '\.c$$' | grep -v '^gen/' | grep -v '^sdk/shinobi/' | grep -v '_kamui_lib\.c$$' | grep -v '^sdk/ninja/njModelE\.c$$' | sort -u)
# njModelE.c (modelos "easy") exige o backend SH-4 do SH4ZAM: fase C5
# paralelismo dos make internos (o laptop esquenta com -j alto)
JOBS     ?= 2
REBASE_F := src/stage/stage.c src/event/play.c src/object/download.c \
            src/sound/stream.c src/progress/recorder.h src/progress/recorder.c \
            src/system/loader.h src/system/loader.c src/debug/host.c \
            modules/movie/movie.c modules/shooting/shooting.c \
            events/shared/sound.h events/ev0045/script.c
REBASE_C := $(filter %.c,$(REBASE_F))

INC := -I$(PT)/sdk/include -I$(RB)/src -I$(RB)/modules -I$(RB)/events \
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
          -fwrapv -Wimplicit-function-declaration -include zrebase.h \
          -include engine/native/prelude.h -include zmath_decl.h $(INC)

# fontes remendadas por tools/gen_patch.py (polygon.c, card.c, mesh.c, shinobi.h)
$(PT)/.done: $(CURDIR)/tools/gen_patch.py
	@mkdir -p $(PT)
	@python3 $(CURDIR)/tools/gen_patch.py $(UPSTREAM) $(PT) >/dev/null
	@touch $@
$(OUT)/obj/src/collision/polygon.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/src/collision/polygon.c 2>$(@:.o=.err) || { echo "FALHOU polygon.c"; rm -f $@; exit 1; }
# opção C: os arquivos de desenho no sabor Dreamcast (com as flags do applet,
# make dcflags), cópias de gen_patch.py com os rótulos __asm__ do SHC ajustados
DC_SET := sdk/ninja/njDraw.c sdk/ninja/njDrawLine2D.c sdk/ninja/njDrawLine3D.c \
          sdk/ninja/njDrawPolygon2D.c sdk/ninja/njDebug.c engine/dc/sprite.c
DS := -D__declspec(x)=
DC_CFLAGS := $(subst $(DS),'$(DS)',$(shell $(MAKE) -s -C $(CURDIR) -f Makefile dcflags)) -I$(UPSTREAM)/build/gen/src -I$(UPSTREAM)/sdk -w
DC_OBJ := $(patsubst %.c,$(OUT)/obj/dc/%.o,$(DC_SET))
$(DC_OBJ): $(OUT)/obj/dc/%.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(DC_CFLAGS) -c -o $@ $(PT)/$*.c 2>$(@:.o=.err) || { echo "FALHOU dc/$*.c"; rm -f $@; exit 1; }
$(OUT)/obj/src/system/timer.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/src/system/timer.c 2>$(@:.o=.err) || { echo "FALHOU timer.c"; rm -f $@; exit 1; }
$(OUT)/obj/src/main.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/src/main.c 2>$(@:.o=.err) || { echo "FALHOU main.c"; rm -f $@; exit 1; }
$(OUT)/obj/sdk/ninja/njSystem.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/sdk/ninja/njSystem.c 2>$(@:.o=.err) || { echo "FALHOU njSystem.c"; rm -f $@; exit 1; }
$(OUT)/obj/src/system/frame.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/src/system/frame.c 2>$(@:.o=.err) || { echo "FALHOU frame.c"; rm -f $@; exit 1; }
$(OUT)/obj/src/save/card.o: CFLAGS += -fno-common
$(OUT)/obj/src/save/card.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/src/save/card.c 2>$(@:.o=.err) || { echo "FALHOU card.c"; rm -f $@; exit 1; }

OBJS := $(filter-out $(OUT)/obj/sdk/ninja/njSystem.o $(OUT)/obj/src/sound/stream.o $(patsubst %.c,$(OUT)/obj/%.o,$(DC_SET)),$(patsubst %.c,$(OUT)/obj/%.o,$(ALL_C))) $(OUT)/obj/content.o

# shims do slice nativo que já compilam em ARM, e o asm do SH-4 em C (asm2c.py)
SHIMS := engine/native/loads engine/native/names engine/native/vibration \
         engine/sa1/hooks engine/numeric/math
OBJS += $(DC_OBJ) $(OUT)/obj/src/sound/stream_z.o $(OUT)/obj/zninjasys.o $(OUT)/obj/sdk/ninja/njSystem_w.o $(OUT)/obj/zgame.o $(OUT)/obj/zstubs.o $(OUT)/obj/zshinobi.o $(OUT)/obj/engine/native/mesh.o $(patsubst %,$(OUT)/obj/%.o,$(SHIMS)) $(OUT)/obj/asm.o $(OUT)/obj/zshim.o

# programas linkados no .mod (fase D1): cada um compila, linka sozinho (ld -r)
# e só mantém global o que o 1ST_READ lê (a tabela de entrada); o resto fica
# local, como engine/tools/programs.py faz no slice nativo. O jogo lê o binário
# SH-4 do disco em MODULE_AREA e zgame.c põe a tabela ARM por cima.
PROGRAMS := advertise summary
PROGRAM_KEEP_advertise := advertise_entries
PROGRAM_KEEP_summary := summary_entries
program_c = $(shell awk '!/^[#]/ && NF {print $$1}' $(UPSTREAM)/config/$(1)/splits.txt | grep '\.c$$' | grep -v '^gen/' | sort -u) \
            $(if $(wildcard $(UPSTREAM)/config/$(1)/data.txt),build/gen/$(1)/content.c)
define program_rules
$(1)_OBJ := $$(patsubst %.c,$(OUT)/obj/prog/$(1)/%.o,$$(call program_c,$(1)))
$(OUT)/obj/prog/$(1)/%.o: $(UPSTREAM)/%.c
	@mkdir -p $$(@D)
	@$$(CC) $$(CFLAGS) -c -o $$@ $$< 2>$$(@:.o=.err) || { echo "FALHOU $(1): $$*.c"; rm -f $$@; exit 1; }
$(OUT)/program-$(1).o: $$($(1)_OBJ) tools/program.ld
	@sed s/@P@/$(1)/g tools/program.ld > $(OUT)/program-$(1).ld
	@$$(LD) -r -d -T $(OUT)/program-$(1).ld -o $(OUT)/program-$(1).r $$(filter %.o,$$^)
	@arm-none-eabi-objcopy $$(addprefix --keep-global-symbol=,$$(PROGRAM_KEEP_$(1))) \
	    $$(foreach s,data_start data_end bss_start bss_end,--keep-global-symbol=$(1)_$$(s)) \
	    $(OUT)/program-$(1).r $$@
endef
$(foreach p,$(PROGRAMS),$(eval $(call program_rules,$(p))))
OBJS += $(PROGRAMS:%=$(OUT)/program-%.o) $(OUT)/obj/zprogs.o
# os programas com código que o .mod não tem: zgdfs.c reinicia o jogo quando
# um é lido (os de movimento, *_mot, são só dados e funcionam do disco)
MISSING_PROGRAMS := $(filter-out 1st_read loader movie %_mot %_sbmot $(PROGRAMS),$(notdir $(patsubst %/,%,$(wildcard $(UPSTREAM)/config/*/))))
$(OUT)/gen/zprogs.c: game.mk
	@mkdir -p $(@D)
	@{ echo 'const char *const zmissing_programs[] = {'; \
	   for p in $(MISSING_PROGRAMS); do echo "    \"$$p\","; done; echo '    0,'; echo '};'; } > $@
$(OUT)/obj/zprogs.o: $(OUT)/gen/zprogs.c
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OUT)/obj/asm.o: $(UPSTREAM)/build/engine/native/asm.c
	@mkdir -p $(@D)
	@$(CC) $(filter-out -std=gnu89 -nostdinc,$(CFLAGS)) -std=gnu99 -D_GNU_SOURCE -DNATIVE_DOMAIN=0 \
	    -include engine/native/prelude.h -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU asm.c"; rm -f $@; exit 1; }
	@# o desenho do asm (PVR) cede ao njDraw.c do sabor Dreamcast (DC_SET)
	@arm-none-eabi-objcopy $(addprefix --weaken-symbol=,njDrawPolygon njDrawTexture njDrawTextureMemList) $@

# os shims do host usam libc: headers do newlib (o SDK do jogo fica por último)
SHIM_CFLAGS := $(filter-out -std=gnu89 -nostdinc -isystem%,$(CFLAGS)) -std=gnu99 -D_GNU_SOURCE \
               -DNATIVE_DOMAIN=0 -include engine/native/prelude.h
$(patsubst %,$(OUT)/obj/%.o,$(SHIMS)): $(OUT)/obj/%.o: $(UPSTREAM)/%.c
	@mkdir -p $(@D)
	@$(CC) $(SHIM_CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU $*.c"; rm -f $@; exit 1; }

$(OUT)/obj/engine/native/mesh.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(SHIM_CFLAGS) -c -o $@ $(PT)/engine/native/mesh.c 2>$(@:.o=.err) || { echo "FALHOU mesh.c"; rm -f $@; exit 1; }

# stubs gerados dos headers (bu/fm/kb/pd/sd/sy); a lista vem de stubnames.txt
$(OUT)/gen/zstubs.c: $(CURDIR)/tools/gen_stubs.py $(CURDIR)/stubnames.txt $(PT)/.done
	@mkdir -p $(@D)
	@python3 $(CURDIR)/tools/gen_stubs.py $@ $(CURDIR)/stubnames.txt $(UPSTREAM)/sdk/include/shinobi.h $(UPSTREAM)/sdk/include/ninja.h $(wildcard $(UPSTREAM)/sdk/include/sg_*.h) > $(OUT)/gen/zstubs.log
$(OUT)/obj/zninjasys.o: $(CURDIR)/zninjasys.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU zninjasys.c"; rm -f $@; exit 1; }
# o njSystem do jogo fica, mas as funções de vídeo do DC cedem a zninjasys.c
$(OUT)/obj/sdk/ninja/njSystem_w.o: $(OUT)/obj/sdk/ninja/njSystem.o
	@arm-none-eabi-objcopy $(addprefix --weaken-symbol=,njInitSystem njExitSystem njSetVSyncFunction njSetEORflag njWaitVSync) $< $@
$(OUT)/obj/zgame.o: $(CURDIR)/zgame.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU zgame.c"; rm -f $@; exit 1; }
$(OUT)/obj/zstubs.o: $(OUT)/gen/zstubs.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU zstubs.c"; rm -f $@; exit 1; }
$(OUT)/obj/zshinobi.o: $(CURDIR)/zshinobi.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -I$(VENDOR)/shadow40/inc -I$(VENDOR)/inc -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU zshinobi.c"; rm -f $@; exit 1; }

$(OUT)/obj/zshim.o: $(CURDIR)/zshim.c
	@mkdir -p $(@D)
	@$(CC) $(SHIM_CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU zshim.c"; rm -f $@; exit 1; }

$(OUT)/obj/sdk/ninja/njModelE.o: CFLAGS += -std=gnu99 -include zmath_decl.h

.DEFAULT_GOAL := all
all: $(OUT)/game.o

$(RB)/.done: $(CURDIR)/tools/gen_rebase.py
	@mkdir -p $(RB)
	@python3 $(CURDIR)/tools/gen_rebase.py $(UPSTREAM) $(RB) $(REBASE_F) >/dev/null
	@touch $@

# fontes reescritas pelo rebase vêm de $(RB); as outras, do decomp
$(addprefix $(OUT)/obj/,$(REBASE_C:.c=.o)): $(OUT)/obj/%.o: $(RB)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(RB)/$*.c || { echo "FALHOU $*.c"; rm -f $@; exit 1; }

# a música do jogo: o stream.c chama o ADXT do Sofdec, que sai pelo driver de
# som (stubs). zaudio.c toca o mesmo ADX pelo IMedia: as chamadas do stream.c
# passam por ele antes (zADXT_*)
$(OUT)/obj/src/sound/stream_z.o: $(OUT)/obj/src/sound/stream.o
	@arm-none-eabi-objcopy --redefine-sym ADXT_StartFname=zADXT_StartFname \
	    --redefine-sym ADXT_Stop=zADXT_Stop $< $@
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

# --- integração com o applet -------------------------------------------------
# Símbolos que o jogo (game.o) e o applet definem os dois. O dono é o applet
# (plataforma Zeebo: sink GLES1, shims do Ninja, string): no jogo ficam fracos.
# O resto das duplicatas se resolve tirando um dos objetos (ver INTEGRATE_DROP).
# do applet saem: ADX (vem do jogo), port_shim (present_* vêm do jogo)
INTEGRATE_DROP := adx_dcd adx_dcd2 adx_dcda adx_xpnd adx_qtbl port_shim asset_adx asset_prs

# os objetos do applet vêm da lista do Makefile (make objs), não de um wildcard:
# um .o velho em build/zeebo/ entrava no link e mascarava o código novo
APPLET_OBJS0 := $(filter-out $(addprefix $(ROOT)/build/zeebo/,$(addsuffix .o,$(INTEGRATE_DROP))),$(shell $(MAKE) -s -C $(CURDIR) -f Makefile objs))
# o que o applet define para a cena de teste e que, com o jogo, cede ao Ninja
# real do jogo: as texturas por texlist (o jogo usa _nj_texmemlist). Local, não
# fraco: no .mod o jogo inteiro fica fraco (--weaken) e o primeiro fraco venceria
GAME_YIELD := njSetTexture njSetTextureNum njSetkmTextureNum njLoadTexturePvmMemory
APPLET_OBJS := $(patsubst $(ROOT)/build/zeebo/%.o,$(OUT)/applet/%.o,$(APPLET_OBJS0))
$(OUT)/applet/%.o: $(ROOT)/build/zeebo/%.o
	@mkdir -p $(@D)
	@arm-none-eabi-objcopy $(addprefix --localize-symbol=,$(GAME_YIELD)) $< $@

# game + applet: o applet (make) tem de estar construído. Gera game/integrated.elf
integrate: link $(APPLET_OBJS)
	@python3 $(CURDIR)/tools/integrate.py $(NM) arm-none-eabi-objcopy $(OUT)/full.o $(OUT)/full_w.o $(APPLET_OBJS)
	@LG=$$($(CC) -mcpu=arm1136j-s -mfloat-abi=soft -print-libgcc-file-name); \
	 $(LD) --emit-relocs -T $(VENDOR)/toolset/bin/elf2mod/src/gnu/elf2mod.x -o $(OUT)/integrated.elf \
	   $(APPLET_OBJS) $(OUT)/full_w.o $$LG 2>$(OUT)/integrate.err; rc=$$?; \
	 echo "duplicatas: $$(grep -c 'multiple definition' $(OUT)/integrate.err)  indefinidos: $$(grep -c 'undefined reference' $(OUT)/integrate.err)"; exit 0

# .mod com jogo + applet. Símbolos ainda sem definição (programas, modelo 3D)
# ficam em 0 (--unresolved-symbols=ignore-all): rodar só serve para ver até onde o
# jogo chega no zeebx. Gera build/zeebo/sa1g.mod
mod: integrate
	@arm-none-eabi-objcopy --weaken $(OUT)/full_w.o $(OUT)/full_weak.o
	@python3 $(CURDIR)/tools/gen_traps.py $(NM) arm-none-eabi-readelf $(OUT)/ztraps.c $(APPLET_OBJS) $(OUT)/full_weak.o
	@$(CC) -mcpu=arm1136j-s -mfloat-abi=soft -marm -O1 -fno-common -w -c -o $(OUT)/ztraps.o $(OUT)/ztraps.c
	@$(LD) --emit-relocs --unresolved-symbols=ignore-all -T $(VENDOR)/toolset/bin/elf2mod/src/gnu/elf2mod.x \
	   -o $(ROOT)/build/zeebo/sa1g.elf $(APPLET_OBJS) $(OUT)/full_weak.o $(OUT)/ztraps.o $$($(CC) -mcpu=arm1136j-s -mfloat-abi=soft -print-libgcc-file-name) 2>$(OUT)/mod.err || { head $(OUT)/mod.err; exit 1; }
	@WINEDEBUG=-all wine $(VENDOR)/toolset/bin/elf2mod.exe -output $(ROOT)/build/zeebo/sa1g.mod $(ROOT)/build/zeebo/sa1g.elf 2>&1 | tail -2
	@cp $(ROOT)/build/zeebo/sa1z.mif $(ROOT)/build/zeebo/sa1g.mif 2>/dev/null; ls -l $(ROOT)/build/zeebo/sa1g.mod

link:
	@$(MAKE) -f game.mk -k -j$(JOBS) all >/dev/null 2>&1; \
	 $(LD) -r -o $(OUT)/game.o $$(ls $(OBJS) 2>/dev/null) && \
	 echo "game.o: $$(ls $(OBJS) 2>/dev/null | wc -l) de $(words $(OBJS)) objetos"
	@LG=$$($(CC) -mcpu=arm1136j-s -mfloat-abi=soft -print-libgcc-file-name); \
	 $(LD) -r -o $(OUT)/full.o $(OUT)/game.o $$LG && echo "full.o: game.o + libgcc"
	@for o in $(OBJS); do [ -f $$o ] || echo $${o#$(OUT)/obj/}; done > $(OUT)/falhas.txt; cat $(OUT)/falhas.txt

indef:
	@$(NM) -u $(OUT)/full.o | awk '{print $$2}' | sort -u > $(OUT)/undef.txt
	@$(NM) --defined-only $(OUT)/full.o | awk '$$2 ~ /[TDBRCGVWS]/ {print $$3}' | sort -u > $(OUT)/def.txt
	@comm -23 $(OUT)/undef.txt $(OUT)/def.txt > $(OUT)/missing.txt
	@echo "indefinidos: $$(wc -l < $(OUT)/missing.txt) (lista em missing.txt)"

.PHONY: all indef link integrate mod
.DELETE_ON_ERROR:
