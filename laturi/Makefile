# Copyright (C) Teemu Suutari, 2008-2011

ASM_VERSION := yes

VPATH = onekpaq

#COMMONFLAGS = -fvisibility=hidden
COMMONFLAGS += -g
#COMMONFLAGS += -mmacosx-version-min=10.7 -DMAC_OS_X_VERSION_MIN_REQUIRED=1070 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk
CFLAGS	= $(COMMONFLAGS)
CFLAGS += -O3
CFLAGS += -Wall -Wswitch-default -Wswitch-enum -Wunused -Wextra -Wshadow
CFLAGS += -Wbad-function-cast -Wcast-align -Wshorten-64-to-32
# -Werror
#CFLAGS += -Wunreachable-code
AFLAGS	= -O2 -Ionekpaq/
CC	= gcc
CXX	= g++
CXXFLAGS = $(CFLAGS) -std=gnu++14 -Wno-shift-op-parentheses
NASM	= ../nasm/nasm

TRANS	= 0 1
OSXVER	= 11 12 13
HASH	= 2 3 4
COMPRES	= 0 1 2 3 4
LSTUBS32 = header32.o $(foreach compr,$(COMPRES),$(foreach os,$(OSXVER),decompressor32_X$(os)_C$(compr).o)) \
	$(foreach hash,$(HASH),$(foreach trans,$(TRANS),$(foreach os,$(OSXVER),loader32_X$(os)_T$(trans)_H$(hash).o)))
LSTUBS64 = header64.o $(foreach compr,$(COMPRES),$(foreach os,$(OSXVER),decompressor64_X$(os)_C$(compr).o)) \
	$(foreach hash,$(HASH),$(foreach trans,$(TRANS),$(foreach os,$(OSXVER),loader64_X$(os)_T$(trans)_H$(hash).o)))

LSTUBS	= $(LSTUBS32) $(LSTUBS64)

LATURI32 = laturi.32
LATURI64 = laturi.64
ifneq ($(ASM_VERSION),)
LATURI	= laturi_asm
CFLAGS += -DLATURI_ASM_ONLY
AFLAGS += -DLATURI_ASM_ONLY
LATURI_LIB = liblaturi_asm.a
else
LATURI	= laturi
LATURI_LIB = liblaturi_c.a
endif

LOBJS	= hashloader.o stub.o kompura.o main.o dyld_address.o onekpaq_integration.o \
	ArithEncoder.o ArithDecoder.o BlockCodec.o StreamCodec.o CacheFile.o
LOBJS32 = $(patsubst %,b32_%,$(LOBJS))
LOBJS64 = $(patsubst %,b64_%,$(LOBJS))
LDFLAGS	= $(COMMONFLAGS) -framework CoreServices
STRIP	= strip

all: $(LATURI) $(LATURI_LIB)

clean:
	rm -f *~ $(LSTUBS) c-symbols.inc _csym_solver.? solver $(LOBJS32) $(LOBJS64) $(LATURI) $(LATURI_LIB) $(LATURI32) $(LATURI64) $(addsuffix .inc,$(LSTUBS))

install: $(LATURI)
	cp $(LATURI) ~/bin/

define flags32l_tmp
loader32_$(1): loader32.asm solver.c
loader32_$(1): AFLAGS+= -DTRAN_$(2) -DOSX_$(3) -DHASH_LEN_$(4) -fmacho32
endef
define flags64l_tmp
loader64_$(1): loader64.asm solver.c
loader64_$(1): AFLAGS+= -DTRAN_$(2) -DOSX_$(3) -DHASH_LEN_$(4) -fmacho64
endef
$(foreach hash,$(HASH),$(foreach trans,$(TRANS),$(foreach os,$(OSXVER),$(eval $(call flags32l_tmp,X$(os)_T$(trans)_H$(hash).o,$(trans),$(os),$(hash))))))
$(foreach hash,$(HASH),$(foreach trans,$(TRANS),$(foreach os,$(OSXVER),$(eval $(call flags64l_tmp,X$(os)_T$(trans)_H$(hash).o,$(trans),$(os),$(hash))))))

define flags32d_tmp
decompressor32_$(1): decompressor32.asm solver.c
decompressor32_$(1): AFLAGS+= -DOSX_$(2) -DCOMPRESSION_$(3) -fmacho32
endef
define flags64d_tmp
decompressor64_$(1): decompressor64.asm solver.c
decompressor64_$(1): AFLAGS+= -DOSX_$(2) -DCOMPRESSION_$(3) -fmacho64
endef
$(foreach compr,$(COMPRES),$(foreach os,$(OSXVER),$(eval $(call flags32d_tmp,X$(os)_C$(compr).o,$(os),$(compr)))))
$(foreach compr,$(COMPRES),$(foreach os,$(OSXVER),$(eval $(call flags64d_tmp,X$(os)_C$(compr).o,$(os),$(compr)))))


header32.o: header32.asm solver.c
header32.o: AFLAGS+= -fmacho32
header64.o: header64.asm solver.c
header64.o: AFLAGS+= -fmacho64

$(LSTUBS):
	rm -f _csym_solver.? solver
	cp -f solver.c _csym_solver.c
	$(NASM) $(AFLAGS) -DSYMBOL_SEARCH -e $< | grep SOLVE_C | sed -E s/\[\^S\]*\(SOLVE_C\\\(\[a-zA-Z0-9_\]*\\\)\)\[\^S\]*\|S/\\1\;/g >> _csym_solver.c
	echo return 0\;\} >> _csym_solver.c
	$(CC) $(CFLAGS) -c _csym_solver.c
	$(CC) $(LDFLAGS) -o solver _csym_solver.o
	./solver > c-symbols.inc
	$(NASM) $(AFLAGS) -o $@ $<
#	$(STRIP) -x $@
	hexdump -ve 1/1\ \"%u,\" $@ > $@.inc
	rm -f _csym_solver.? solver


$(LATURI32): COMMONFLAGS += -m32
$(LATURI32): $(LSTUBS32) $(LOBJS32)
	$(CXX) $(LDFLAGS) -o $@ $(LOBJS32)
#	$(STRIP) -x $@

$(LATURI64): $(LSTUBS64) $(LOBJS64)
	$(CXX) $(LDFLAGS) -o $@ $(LOBJS64)
#	$(STRIP) -x $@

b32_%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

b32_%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

b64_%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

b64_%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(LATURI): $(LATURI32) $(LATURI64)
	lipo -create -arch x86_64 $(LATURI64) -arch i386 $(LATURI32) -output $@

$(LATURI_LIB): $(LOBJS32)
	ar -rv $(LATURI_LIB) $(filter-out b32_main.o,$(LOBJS32))
	ranlib $(LATURI_LIB)
