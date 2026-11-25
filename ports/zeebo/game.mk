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
            src/event/skip.c src/draw/model.c
ALL_C    := $(EXTRA_C) $(shell awk '!/^[#]/ && NF {print $$1}' $(SPLITS) | grep '\.c$$' | grep -v '^gen/' | grep -v '^sdk/shinobi/' | grep -v '_kamui_lib\.c$$' | grep -v '^sdk/ninja/njModelE\.c$$' | grep -v '^sdk/ninja/njModelS\.c$$' | sort -u)
# njModelE.c (modelos "easy") exige o backend SH-4 do SH4ZAM: fase C5
# njModelS.c (modelos "simple") escreve cada vertice na store queue (R8 + PREF) e chama
# 12 emissores em asm SH-4 que aqui eram armadilhas: o que usava njSimpleDraw* saia
# invisivel (Zero, Twinkle Circuit). zlive.c define njSimpleDrawModel/Object no lugar.
# paralelismo dos make internos (o laptop esquenta com -j alto)
JOBS     ?= 8
REBASE_F := src/stage/stage.c src/event/play.c src/object/download.c \
            src/sound/stream.c src/progress/recorder.h src/progress/recorder.c \
            src/system/loader.h src/system/loader.c src/debug/host.c \
            modules/movie/movie.c modules/shooting/shooting.c \
            events/shared/sound.h events/ev0045/script.c
# gerados com endereços fixos (as câmeras de cada ato apontam para
# CAMERA_AREA_n); as fontes (font_*.inc) têm palavras parecidas e ficam fora
REBASE_GEN := src/camera/layouts/camera_layouts.inc
REBASE_C := $(filter %.c,$(REBASE_F))

INC := -I$(PT)/sdk/include -I$(RB)/gen/src -I$(RB)/src -I$(RB)/modules -I$(RB)/events \
       -I$(UPSTREAM)/sdk -I$(UPSTREAM)/src -I$(UPSTREAM)/modules -I$(UPSTREAM)/events \
       -I$(UPSTREAM)/build/gen/src -I$(UPSTREAM)/build/gen/modules \
       -I$(UPSTREAM)/build/gen/events -I$(UPSTREAM)/build/gen/sdk \
       -I$(UPSTREAM)/engine/include -I$(UPSTREAM)/engine/dc -I$(UPSTREAM)/engine/sa1 \
       -I$(UPSTREAM)/engine/native -I$(UPSTREAM)/engine/numeric -I$(SHZAM)/include \
       -I$(UPSTREAM) -I$(CURDIR) -idirafter $(UPSTREAM)/sdk/include

# OPT antes do CFLAGS: o CFLAGS := expande na hora, e o "OPT ?= -O3" que ficava mais abaixo chegava
# tarde (o codigo do jogo saia sem nenhum -O, ou seja, -O0). -O2: -O3 incha o .text sem ganho
OPT ?= -O2 -fno-tree-vectorize
CFLAGS := -std=gnu89 -march=armv6 -mcpu=arm1136j-s -mfloat-abi=soft $(OPT) \
          -ffunction-sections -fdata-sections -fcommon -ffreestanding -nostdinc \
          -isystem $(shell $(CC) -print-file-name=include) \
          -DNONMATCHING -DZEEBO_PORT -DSHZ_TLS_MODEL=0 -DINFINITY=__builtin_inff\(\) -fms-extensions -ffp-contract=off -fno-strict-aliasing \
          -fwrapv -Wimplicit-function-declaration -include zrebase.h \
          -include engine/native/prelude.h -include zmath_decl.h $(INC)
# dependencias de cabecalho: sem isto, editar um .h nao recompilava nada e o .mod
# saia com objetos velhos (medido: tocar sdk/include/ninja.h nao recompilou nem 1 objeto)
CFLAGS += -MMD -MP
# DEBUG_TITLE=N: o menu de debug do jogo no lugar do título, na linha N de
# title_debug_stages (src/progress/title.c; 3 = Emerald Coast ato 1)
ifneq ($(DEBUG_TITLE),)
$(OUT)/obj/src/progress/title.o $(OUT)/obj/src/main.o: CFLAGS += -DSA1_DEBUG_TITLE=$(DEBUG_TITLE)
endif
# sempre recompilam: a flag liga e desliga entre builds
$(OUT)/obj/src/progress/title.o $(OUT)/obj/src/main.o: FORCE
FORCE:

# fontes remendadas por tools/gen_patch.py (polygon.c, card.c, mesh.c, shinobi.h)
# os fontes que o gen_patch remenda: editar um deles refaz as copias (antes so mudar o proprio
# gen_patch.py refazia, e uma edicao em src/main.c ficava de fora do build sem aviso)
PATCHED_SRC := src/collision/polygon.c src/system/frame.c sdk/ninja/njSystem.c src/main.c \
               src/system/timer.c src/system/present.c sdk/ninja/njDebug.c src/draw/text.c \
               src/save/card.c sdk/include/shinobi.h engine/native/mesh.c
$(PT)/.done: $(CURDIR)/tools/gen_patch.py $(addprefix $(UPSTREAM)/,$(PATCHED_SRC))
	@mkdir -p $(PT)
	@python3 $(CURDIR)/tools/gen_patch.py $(UPSTREAM) $(PT) >/dev/null
	@touch $@
$(OUT)/obj/src/collision/polygon.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/src/collision/polygon.c 2>$(@:.o=.err) || { echo "FALHOU polygon.c"; rm -f $@; exit 1; }
# opção C: os arquivos de desenho no sabor Dreamcast (com as flags do applet,
# make dcflags), cópias de gen_patch.py com os rótulos __asm__ do SHC ajustados
DC_SET := sdk/ninja/njDraw.c sdk/ninja/njDrawLine2D.c sdk/ninja/njDrawLine3D.c \
          sdk/ninja/njDrawPolygon2D.c sdk/ninja/njDebug.c
DS := -D__declspec(x)=
DC_CFLAGS := $(subst $(DS),'$(DS)',$(shell $(MAKE) -s -C $(CURDIR) -f Makefile dcflags)) -I$(UPSTREAM)/build/gen/src -I$(UPSTREAM)/sdk -w
DC_OBJ := $(patsubst %.c,$(OUT)/obj/dc/%.o,$(DC_SET))
$(DC_OBJ): $(OUT)/obj/dc/%.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(DC_CFLAGS) -c -o $@ $(PT)/$*.c 2>$(@:.o=.err) || { echo "FALHOU dc/$*.c"; rm -f $@; exit 1; }
$(OUT)/obj/src/system/timer.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/src/system/timer.c 2>$(@:.o=.err) || { echo "FALHOU timer.c"; rm -f $@; exit 1; }
# a camada de modelos (model.c, no lugar de model.src) com o live.h de host
# (engine/sa1/live.h, out-of-line): zlive.c desenha cada modelo no sink
$(OUT)/obj/src/draw/model.o: $(UPSTREAM)/src/draw/model.c
	@mkdir -p $(@D)
	@$(CC) -I$(UPSTREAM)/engine $(CFLAGS) -include sa_dc.h -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU model.c"; cat $(@:.o=.err) | head -20; rm -f $@; exit 1; }
# os kernels do njModel.c em C (o asm SH-4 não compila em ARM)
$(OUT)/obj/njmodel_arm.o: CFLAGS += -std=gnu99
$(OUT)/obj/njmodel_arm.o: $(CURDIR)/ninja/njmodel_arm.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU njmodel_arm.c"; head -20 $(@:.o=.err); rm -f $@; exit 1; }
# as calculadoras de luz do njLtSrcs.c em C (o asm SH-4 não compila em ARM)
$(OUT)/obj/lt_arm.o: CFLAGS += -std=gnu99
$(OUT)/obj/lt_arm.o: $(CURDIR)/ninja/lt_arm.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU lt_arm.c"; head -20 $(@:.o=.err); rm -f $@; exit 1; }
# um XMTRX só: o do SH4ZAM do applet vira o banco X do asm (ver zxmtrx.c)
$(OUT)/obj/zxmtrx.o: $(CURDIR)/ninja/zxmtrx.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU zxmtrx.c"; head -20 $(@:.o=.err); rm -f $@; exit 1; }
$(OUT)/obj/src/main.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/src/main.c 2>$(@:.o=.err) || { echo "FALHOU main.c"; rm -f $@; exit 1; }
$(OUT)/obj/sdk/ninja/njSystem.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/sdk/ninja/njSystem.c 2>$(@:.o=.err) || { echo "FALHOU njSystem.c"; rm -f $@; exit 1; }
$(OUT)/obj/src/system/frame.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/src/system/frame.c 2>$(@:.o=.err) || { echo "FALHOU frame.c"; rm -f $@; exit 1; }
# present.c remendado (ganchos de perfil, vazios sem PROF=1)
PROF ?= 0
$(OUT)/obj/src/system/present.o: $(PT)/.done FORCE
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $(if $(filter 1,$(PROF)),-DZPROF) -c -o $@ $(PT)/src/system/present.c 2>$(@:.o=.err) || { echo "FALHOU present.c"; rm -f $@; exit 1; }
$(OUT)/obj/src/draw/text.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/src/draw/text.c 2>$(@:.o=.err) || { echo "FALHOU text.c"; rm -f $@; exit 1; }
$(OUT)/obj/src/save/card.o: CFLAGS += -fno-common
$(OUT)/obj/src/save/card.o: $(PT)/.done
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $(PT)/src/save/card.c 2>$(@:.o=.err) || { echo "FALHOU card.c"; rm -f $@; exit 1; }

OBJS := $(filter-out $(OUT)/obj/sdk/ninja/njSystem.o $(OUT)/obj/src/sound/stream.o $(patsubst %.c,$(OUT)/obj/%.o,$(DC_SET)),$(patsubst %.c,$(OUT)/obj/%.o,$(ALL_C))) $(OUT)/obj/content.o

# shims do slice nativo que já compilam em ARM, e o asm do SH-4 em C (asm2c.py)
SHIMS := engine/native/loads engine/native/names engine/native/vibration \
         engine/sa1/hooks
OBJS += $(DC_OBJ) $(OUT)/obj/src/sound/stream_z.o $(OUT)/obj/zninjasys.o $(OUT)/obj/sdk/ninja/njSystem_w.o $(OUT)/obj/zgame.o $(OUT)/obj/zstubs.o $(OUT)/obj/zshinobi.o $(OUT)/obj/zlive.o $(OUT)/obj/zmovie.o $(OUT)/obj/zplm.o $(OUT)/obj/engine/native/mesh.o $(patsubst %,$(OUT)/obj/%.o,$(SHIMS)) $(OUT)/obj/engine/numeric/math.o $(OUT)/obj/zfpu.o $(OUT)/obj/asm.o $(OUT)/obj/njmodel_arm.o $(OUT)/obj/lt_arm.o $(OUT)/obj/zshim.o

# programas linkados no .mod (fase D1): cada um compila, linka sozinho (ld -r)
# e só mantém global o que o 1ST_READ lê (a tabela de entrada); o resto fica
# local, como engine/tools/programs.py faz no slice nativo. O jogo lê o binário
# SH-4 do disco em MODULE_AREA e zgame.c põe a tabela ARM por cima.
PROGRAMS := advertise summary
PROGRAM_KEEP_advertise := advertise_entries
PROGRAM_KEEP_summary := summary_entries
# Arquivos de movimento são dados com ponteiros absolutos do DC. Integre-os
# como overlays ARM relocáveis: ler o PRS cru em zdc_ram não define os símbolos
# importados pelo 1ST_READ (sonic_clear_action ficava zerado após Chaos 0).
MOTION_PROGRAMS := a_mot b_mot e_mot k_mot s_mot m_sbmot s_sbmot
PROGRAMS += $(MOTION_PROGRAMS)
# os programas de stage não têm tabela: o 1ST_READ lê os dados deles por nome
# (terras, câmeras, objetos). Sem PROGRAM_KEEP, fica global o que o jogo
# importa (uu.txt de make integrate)
PROGRAMS += adv00 b_chaos0 stg00 stg01 stg02 stg03 stg04 stg05 stg06 stg07 stg08 stg09 stg10 stg12
PROGRAMS += adv0100 adv0130 adv01obj adv02 adv03 al_garden00 al_garden01 al_garden02 al_main al_race b_chaos2 b_chaos4 b_chaos6 b_chaos7 b_e101 b_e101_r b_egm1 b_egm2 b_egm3 b_robo minicart sboard shooting tikal_prog
# eventos: a cabeça (struct event_module) vai por cima do binário SH-4 que
# event_load lê em EVENT_MODULE (zevents.c, gerado)
EVENTS := $(sort $(notdir $(patsubst %/,%,$(wildcard $(UPSTREAM)/config/ev*/))))
PROGRAMS += $(EVENTS)
# programas cujo .data vai para overlay externo (<P>.ovl no disco, PRS): so a
# regiao fica no .mod. Ver tools/gen_ovl.py
comma := ,
space := $(subst ,, )
OVL_PROGRAMS := $(filter-out advertise summary,$(PROGRAMS))
# uma regiao por area do loader (src/system/loader.c): FIELD_AREA, EXTRA_AREA e
# TIKAL_AREA coexistem com o MODULE_AREA; dentro de cada area so um programa por vez
OVL_FIELD := adv0100 adv0130
OVL_EXTRA := al_garden00 al_garden01 al_garden02 al_race
OVL_TIKAL := tikal_prog
# MOTION_AREA (0x0cc00000) coexiste com MODULE/FIELD/EXTRA/TIKAL. Os arquivos
# *_mot se substituem entre si, mas precisam sobreviver à carga posterior do
# stage/chefe para a pose de vitória continuar válida.
OVL_MOTION := $(MOTION_PROGRAMS)
# cada evento e lido em EVENT_MODULE (zevents.c): regiao propria
OVL_EVENT := $(EVENTS)
OVL_MODULE := $(filter-out $(OVL_FIELD) $(OVL_EXTRA) $(OVL_TIKAL) $(OVL_MOTION) $(OVL_EVENT),$(OVL_PROGRAMS))
OVL_GROUPS := modulo=$(subst $(space),$(comma),$(OVL_MODULE)) field=$(subst $(space),$(comma),$(OVL_FIELD)) extra=$(subst $(space),$(comma),$(OVL_EXTRA)) tikal=$(OVL_TIKAL) motion=$(subst $(space),$(comma),$(OVL_MOTION)) evento=$(subst $(space),$(comma),$(OVL_EVENT))
$(foreach e,$(EVENTS),$(eval PROGRAM_KEEP_$(e) := $(e)_head))
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
	@{ printf '%s\n' $$(PROGRAM_KEEP_$(1)); \
	   arm-none-eabi-nm -g --defined-only $(OUT)/program-$(1).r | \
	       awk '{print $$$$3}' | grep -Fxf $(OUT)/uu.txt; true; } > $(OUT)/program-$(1).keep
	@arm-none-eabi-objcopy $$(addprefix --globalize-symbol=,$$(PROGRAM_KEEP_$(1))) $(OUT)/program-$(1).r $(OUT)/program-$(1).g
	@arm-none-eabi-objcopy --keep-global-symbols=$(OUT)/program-$(1).keep \
	    $$(foreach s,data_start data_end bss_start bss_end,--keep-global-symbol=$(1)_$$(s)) \
	    $(OUT)/program-$(1).g $$@
endef
$(foreach p,$(PROGRAMS),$(eval $(call program_rules,$(p))))
# os programas compilam do decomp; os arquivos do REBASE_F que um programa usa
# (shooting.c, movie.c, ev0045/script.c) tem de vir da copia reescrita (ZDC), senao
# o endereco fixo do DC (0x0cb80000 etc.) fica cru no codigo
define rebased_prog_rule
$(OUT)/obj/prog/$(1)/$(2:.c=.o): $(RB)/.done
	@mkdir -p $$(@D)
	@$$(CC) $$(CFLAGS) -c -o $$@ $(RB)/$(2) 2>$$(@:.o=.err) || { echo "FALHOU $(1): $(2)"; rm -f $$@; exit 1; }
endef
$(foreach p,$(PROGRAMS),$(foreach f,$(filter $(REBASE_C),$(call program_c,$(p))),$(eval $(call rebased_prog_rule,$(p),$(f)))))
# programas que definem o mesmo simbolo de dados (overlay no mesmo endereco do
# DC): tools/gen_dups.py renomeia para P__S e gera zdups.c (copia ao abrir)
$(OUT)/dups.stamp: $(PROGRAMS:%=$(OUT)/program-%.o) tools/gen_dups.py
	@python3 tools/gen_dups.py $(OUT) $(NM) arm-none-eabi-objcopy $(PROGRAMS)
	@touch $@
$(PROGRAMS:%=$(OUT)/dup/program-%.o): $(OUT)/dups.stamp
	@:
$(OUT)/gen/zdups.c: $(OUT)/dups.stamp
	@:
$(OUT)/obj/zdups.o: $(OUT)/gen/zdups.c
	@$(CC) $(CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU zdups.c"; head -20 $(@:.o=.err); rm -f $@; exit 1; }
$(OUT)/ovl/program-%.o: $(OUT)/dup/program-%.o
	@mkdir -p $(OUT)/ovl
	@arm-none-eabi-objcopy --rename-section .data=.ovl_$* $< $@
OBJS += $(addprefix $(OUT)/dup/program-,$(addsuffix .o,$(filter-out $(OVL_PROGRAMS),$(PROGRAMS)))) $(OVL_PROGRAMS:%=$(OUT)/ovl/program-%.o) $(OUT)/obj/zprogs.o $(OUT)/obj/zdups.o
ELF2MOD_X := $(VENDOR)/toolset/bin/elf2mod/src/gnu/elf2mod.x
$(OUT)/zeebo_a.x $(OUT)/zeebo_b.x: tools/gen_ovl.py game.mk $(ELF2MOD_X) $(OVL_PROGRAMS:%=$(OUT)/ovl/program-%.o)
	@python3 tools/gen_ovl.py script $(ELF2MOD_X) $(OUT)/zeebo_a.x $(OUT)/zeebo_b.x $(OVL_GROUPS)
$(OUT)/prscomp: tools/prscomp.c assets/prs.c
	@cc -O2 -I assets -o $@ $^
# os programas com código que o .mod não tem: zgdfs.c reinicia o jogo quando
# um é lido (os de movimento, *_mot, são só dados e funcionam do disco)
MISSING_PROGRAMS := $(filter-out 1st_read loader movie %_mot %_sbmot $(PROGRAMS),$(notdir $(patsubst %/,%,$(wildcard $(UPSTREAM)/config/*/))))
$(OUT)/gen/zprogs.c: game.mk
	@mkdir -p $(@D)
	@{ echo 'const char *const zmissing_programs[] = {'; \
	   for p in $(MISSING_PROGRAMS); do echo "    \"$$p\","; done; echo '    0,'; echo '};'; \
	   echo 'struct zevent_head { void *entry; void *textures; };'; \
	   for e in $(EVENTS); do echo "extern struct zevent_head $${e}_head;"; done; \
	   echo 'const struct { int number; const struct zevent_head *head; } zevents[] = {'; \
	   for e in $(EVENTS); do echo "    {0x$${e#ev}, &$${e}_head},"; done; echo '    {-1, 0},'; echo '};'; \
	   echo 'void zevent_head(int number, void *module)'; \
	   echo '{ int i; for (i = 0; zevents[i].head; i++) if (zevents[i].number == number) { *(struct zevent_head *)module = *zevents[i].head; return; } }'; \
	 } > $@
$(OUT)/obj/zprogs.o: $(OUT)/gen/zprogs.c
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OUT)/obj/asm.o: $(UPSTREAM)/build/engine/native/asm.c
	@mkdir -p $(@D)
	@$(CC) $(filter-out -std=gnu89 -nostdinc,$(CFLAGS)) -std=gnu99 -D_GNU_SOURCE -DNATIVE_DOMAIN=0 -O2 \
	    -include engine/native/prelude.h -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU asm.c"; rm -f $@; exit 1; }
	@# o desenho do asm (PVR) cede ao njDraw.c do sabor Dreamcast (DC_SET)
	@arm-none-eabi-objcopy $(addprefix --weaken-symbol=,njDrawPolygon njDrawTexture njDrawTextureMemList) --redefine-sym njCalcPoint=njCalcPoint_asm --redefine-sym njCalcVector=njCalcVector_asm $@

# os shims do host usam libc: headers do newlib (o SDK do jogo fica por último)
SHIM_CFLAGS := $(filter-out -std=gnu89 -nostdinc -isystem%,$(CFLAGS)) -std=gnu99 -D_GNU_SOURCE \
               -DNATIVE_DOMAIN=0 -include engine/native/prelude.h
$(patsubst %,$(OUT)/obj/%.o,$(SHIMS)): $(OUT)/obj/%.o: $(UPSTREAM)/%.c
	@mkdir -p $(@D)
	@$(CC) $(SHIM_CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU $*.c"; rm -f $@; exit 1; }

# a matematica do SH-4 (FTRV, FIPR, FSCA, FSRRA, FSQRT): fast.c da os mesmos bits do math.c de
# referencia em inteiros, sem a lentidao de uma referencia escrita para ser lida (era ~17% das
# instrucoes do 3D). A tabela de senos vem do proprio math.c, rodado no host
$(OUT)/gen/sine.h: $(UPSTREAM)/engine/numeric/sine.c $(UPSTREAM)/engine/numeric/math.c
	@mkdir -p $(@D)
	@cc -std=c17 -O2 -I$(UPSTREAM)/engine/numeric -o $@.bin $^ -lm && $@.bin > $@ && rm -f $@.bin
$(OUT)/obj/engine/numeric/math.o: $(UPSTREAM)/engine/numeric/fast.c $(OUT)/gen/sine.h
	@mkdir -p $(@D)
	@$(CC) $(SHIM_CFLAGS) -O2 -Dmemcpy=__builtin_memcpy -I$(UPSTREAM)/engine/numeric -I$(OUT)/gen -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU fast.c"; head -20 $(@:.o=.err); rm -f $@; exit 1; }
	@arm-none-eabi-objcopy --redefine-sym sa_ftrv=exact_ftrv --redefine-sym sa_fipr=exact_fipr --redefine-sym sa_fmac=exact_fmac --redefine-sym sa_fcmp_gt=exact_fcmp_gt --redefine-sym sa_fcmp_eq=exact_fcmp_eq --redefine-sym sa_fsca=exact_fsca --redefine-sym sa_fsqrt=exact_fsqrt --redefine-sym sa_fsrra=exact_fsrra $@

$(OUT)/obj/zfpu.o: $(CURDIR)/zfpu.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -O2 -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU zfpu.c"; head -20 $(@:.o=.err); rm -f $@; exit 1; }

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
# filmes: pl_mpeg (MIT, baixado no build) com o shim de libc de tests/sfdplay
$(OUT)/gen/pl_mpeg.orig.h:
	@mkdir -p $(@D)
	@curl -sSL -o $@ https://raw.githubusercontent.com/phoboslab/pl_mpeg/master/pl_mpeg.h
# tools/patch_plm.py: leitura de bits mais rápida, mesmo resultado
$(OUT)/gen/pl_mpeg.h: $(OUT)/gen/pl_mpeg.orig.h $(CURDIR)/tools/patch_plm.py
	@python3 $(CURDIR)/tools/patch_plm.py $< $@
# AUDIO=1 liga o som (Makefile): sem ele o filme nem extrai o ADX
AUDIO ?= 0
ifeq ($(AUDIO),0)
$(OUT)/obj/zmovie.o: CFLAGS += -DZAUDIO_OFF
endif
$(OUT)/obj/zmovie.o: $(CURDIR)/zmovie.c $(CURDIR)/zstrip.h FORCE
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU zmovie.c"; grep -m10 error $(@:.o=.err); rm -f $@; exit 1; }
$(OUT)/obj/zplm.o: $(CURDIR)/zplm.c $(OUT)/gen/pl_mpeg.h
	@mkdir -p $(@D)
	@$(CC) -std=gnu99 -march=armv6 -mcpu=arm1136j-s -mfloat-abi=soft -O2 -fno-builtin -ffunction-sections \
	    -mapcs-frame -mno-thumb-interwork -fno-common -ffreestanding -nostdinc -isystem $(shell $(CC) -print-file-name=include) \
	    -I$(CURDIR)/tests/sfdplay/inc -I$(OUT)/gen -c -o $@ $<
$(OUT)/obj/zlive.o: $(CURDIR)/zlive.c $(CURDIR)/zstrip.h
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $< 2>$(@:.o=.err) || { echo "FALHOU zlive.c"; head -20 $(@:.o=.err); rm -f $@; exit 1; }
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
	@python3 $(CURDIR)/tools/gen_rebase.py $(UPSTREAM)/build/gen $(RB)/gen $(REBASE_GEN) >/dev/null
	@python3 $(CURDIR)/tools/gen_rebase.py --event-hook $(RB)/src/event/play.c
	@python3 $(CURDIR)/tools/gen_rebase.py --shooting-hook $(RB)/modules/shooting/shooting.c
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
	    --redefine-sym ADXT_StartAfs=zADXT_StartAfs \
	    --redefine-sym ADXT_SetOutVol=zADXT_SetOutVol \
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
	@arm-none-eabi-objcopy $(addprefix --localize-symbol=,$(GAME_YIELD)) --weaken-symbol=shz_xmtrx_state_ $< $@

# game + applet: o applet (make) tem de estar construído. Gera game/integrated.elf
integrate: link $(APPLET_OBJS) $(OUT)/zeebo_a.x
	@python3 $(CURDIR)/tools/integrate.py $(NM) arm-none-eabi-objcopy $(OUT)/full.o $(OUT)/full_w.o $(APPLET_OBJS)
	@LG=$$($(CC) -mcpu=arm1136j-s -mfloat-abi=soft -print-libgcc-file-name); \
	 $(LD) --emit-relocs -T $(OUT)/zeebo_a.x -o $(OUT)/integrated.elf \
	   $(APPLET_OBJS) $(OUT)/full_w.o $$LG 2>$(OUT)/integrate.err; rc=$$?; \
	 echo "duplicatas: $$(grep -c 'multiple definition' $(OUT)/integrate.err)  indefinidos: $$(grep -c 'undefined reference' $(OUT)/integrate.err)"; exit 0

# .mod com jogo + applet. Símbolos ainda sem definição (programas, modelo 3D)
# ficam em 0 (--unresolved-symbols=ignore-all): rodar só serve para ver até onde o
# jogo chega no zeebx. Gera build/zeebo/sa1g.mod
mod: integrate $(OUT)/obj/zxmtrx.o $(OUT)/zeebo_b.x $(OUT)/prscomp
	@arm-none-eabi-objcopy --weaken $(OUT)/full_w.o $(OUT)/full_weak.o
	@python3 $(CURDIR)/tools/gen_traps.py $(NM) arm-none-eabi-readelf $(OUT)/ztraps.c $(APPLET_OBJS) $(OUT)/obj/njmodel_arm.o $(OUT)/obj/lt_arm.o $(OUT)/obj/zxmtrx.o $(OUT)/full_weak.o
	@$(CC) -mcpu=arm1136j-s -mfloat-abi=soft -marm -O1 -fno-common -w -c -o $(OUT)/ztraps.o $(OUT)/ztraps.c
	@$(LD) --emit-relocs --unresolved-symbols=ignore-all -T $(OUT)/zeebo_a.x \
	   -o $(ROOT)/build/zeebo/sa1g.elf $(APPLET_OBJS) $(OUT)/obj/njmodel_arm.o $(OUT)/obj/lt_arm.o $(OUT)/obj/zxmtrx.o $(OUT)/full_weak.o $(OUT)/ztraps.o $$($(CC) -mcpu=arm1136j-s -mfloat-abi=soft -print-libgcc-file-name) 2>$(OUT)/mod.err || { head $(OUT)/mod.err; exit 1; }
	@$(LD) --emit-relocs --unresolved-symbols=ignore-all -T $(OUT)/zeebo_b.x \
	   -o $(OUT)/ovl_extract.elf $(APPLET_OBJS) $(OUT)/obj/njmodel_arm.o $(OUT)/obj/lt_arm.o $(OUT)/obj/zxmtrx.o $(OUT)/full_weak.o $(OUT)/ztraps.o $$($(CC) -mcpu=arm1136j-s -mfloat-abi=soft -print-libgcc-file-name) 2>$(OUT)/mod_b.err || { head $(OUT)/mod.err; exit 1; }
	@python3 tools/gen_ovl.py extract $(OUT)/ovl_extract.elf $(ROOT)/build/zeebo $(OUT)/prscomp $(OVL_GROUPS)
	@rm -f $(ROOT)/build/zeebo/sa1g.mod
	@WINEDEBUG=-all wine $(VENDOR)/toolset/bin/elf2mod.exe -output $(ROOT)/build/zeebo/sa1g.mod $(ROOT)/build/zeebo/sa1g.elf 2>&1 | tail -2
	@[ -s $(ROOT)/build/zeebo/sa1g.mod ] || { echo 'ERRO: o elf2mod nao gerou o sa1g.mod'; exit 1; }
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

-include $(shell find $(OUT)/obj -name '*.d' 2>/dev/null)

.PHONY: all indef link integrate mod
.DELETE_ON_ERROR:
