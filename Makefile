# Copyright (C) Teemu Suutari

CC	= gcc
CFLAGS	= -m32 -Wall -Werror
LDFLAGS = -m32

NASM	=  ./nasm/nasm
AFLAGS	= -O2
SCR_W	?= 1920
SCR_H	?= 1080
AFLAGS += -DSCREEN_WIDTH=$(SCR_W) -DSCREEN_HEIGHT=$(SCR_H)
# -DMODE_SWITCHING
#AFLAGS += -DCOMPATIBILITY_BUILD
#AFLAGS += -DSCREEN_WIDTH=800 -DSCREEN_HEIGHT=600

LATURI	= ./laturi/laturi_asm
LATURIFLAGS = -v -m 200000000 -H2 -W111
LATURIFLAGS += -f Cocoa
LATURIFLAGS += --tgt-noclean
LATURIFLAGS += -p4 -P65536
#LATURIFLAGS += --tgt-nopack
LDFLAGS_BIN = -m32 -framework Cocoa -framework Carbon -framework OpenGL -framework CoreAudio
#LDFLAGS_BIN += -framework AppKit

OBJS	= intro.o shader.o
#OBJS	= intro.o shader.o
DYN_OBJS = find_sym.dyn_o
OBJS_L	= $(patsubst %,%.laturi_o,$(basename $(OBJS)))
OBJS_D	= $(patsubst %,%.dyn_o,$(basename $(OBJS)))
OBJS_C	= $(patsubst %,%.compat_o,$(basename $(OBJS)))
BIN	= intro

SHADER	= data/main_shader_$(SCR_W)_$(SCR_H).fsh

all: $(BIN).laturi $(BIN).dl $(BIN).compat

clean:
	rm -f *~ */*~ c-symbols.inc _csym_solver.? solver $(OBJS_L) $(OBJS_D) $(OBJS_C) $(DYN_OBJS) $(BIN).laturi $(BIN).dl $(BIN).compat
	rm -rf data

$(SHADER): video/main_shader_generic.fsh
	mkdir -p data
	cpp -xc++ -DSCREEN_WIDTH=$(SCR_W) -DSCREEN_HEIGHT=$(SCR_H) video/main_shader_generic.fsh | \
	sed -E s/\(#.*\)//  | sed s/^[\ \	]*// | sed s/[\ \	]*\$$// | tr -ds '\n' ' ' | cat video/header.fsh - > $(SHADER)

#%.laturi_o: AFLAGS+=-DOLD_OSX
%.laturi_o: %.asm $(NASM)
	rm -f _csym_solver.? solver
	cp -f solver.c _csym_solver.c
	$(NASM) $(AFLAGS) -DSYMBOL_SEARCH -e $< | grep SOLVE_C | sed -E s/\[\^S\]*\(SOLVE_C\\\(\[a-zA-Z0-9_\]*\\\)\)\[\^S\]*\|S/\\1\;/g >> _csym_solver.c
	echo return 0\;\} >> _csym_solver.c
	$(CC) $(CFLAGS) -c _csym_solver.c
	$(CC) $(LDFLAGS) -o solver _csym_solver.o
	./solver > c-symbols.inc
	$(NASM) $(AFLAGS) -f macho -o $@ $<
	rm -f _csym_solver.? solver

%.dyn_o: AFLAGS+=-DDEBUG_BUILD -DNO_LATURI -g
%.dyn_o: %.asm $(NASM)
	rm -f _csym_solver.? solver
	cp -f solver.c _csym_solver.c
	$(NASM) $(AFLAGS) -DSYMBOL_SEARCH -e $< | grep SOLVE_C | sed -E s/\[\^S\]*\(SOLVE_C\\\(\[a-zA-Z0-9_\]*\\\)\)\[\^S\]*\|S/\\1\;/g >> _csym_solver.c
	echo return 0\;\} >> _csym_solver.c
	$(CC) $(CFLAGS) -c _csym_solver.c
	$(CC) $(LDFLAGS) -o solver _csym_solver.o
	./solver > c-symbols.inc
	$(NASM) $(AFLAGS) -f macho -o $@ $<
	rm -f _csym_solver.? solver
%.dyn_o: %.c
	gcc -m32 -o $@ -c $<

%.compat_o: AFLAGS+=-DNO_LATURI -DMODE_SWITCHING
%.compat_o: %.asm $(NASM)
	rm -f _csym_solver.? solver
	cp -f solver.c _csym_solver.c
	$(NASM) $(AFLAGS) -DSYMBOL_SEARCH -e $< | grep SOLVE_C | sed -E s/\[\^S\]*\(SOLVE_C\\\(\[a-zA-Z0-9_\]*\\\)\)\[\^S\]*\|S/\\1\;/g >> _csym_solver.c
	echo return 0\;\} >> _csym_solver.c
	$(CC) $(CFLAGS) -c _csym_solver.c
	$(CC) $(LDFLAGS) -o solver _csym_solver.o
	./solver > c-symbols.inc
	$(NASM) $(AFLAGS) -f macho -o $@ $<
	rm -f _csym_solver.? solver

$(BIN).laturi: $(SHADER) $(OBJS_L) $(LATURI)
	$(LATURI) $(LATURIFLAGS)  $(patsubst %,-i %,$(OBJS_L)) -o $@

$(BIN).dl: LDFLAGS_BIN += -framework GLUT
$(BIN).dl: $(SHADER) $(OBJS_D) $(DYN_OBJS)
	$(CC) $(LDFLAGS_BIN) -o $@ $(OBJS_D) $(DYN_OBJS)
#	strip $@
	
$(BIN).compat: $(SHADER) $(OBJS_C) $(DYN_OBJS)
	$(CC) $(LDFLAGS_BIN) -o $@ $(OBJS_C) $(DYN_OBJS)
	strip $@

$(LATURI):
	make -C laturi

$(NASM):
	make -C nasm
