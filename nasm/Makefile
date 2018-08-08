
NASM	= https://www.nasm.us/pub/nasm/releasebuilds/2.13.03/macosx/nasm-2.13.03-macosx.zip
NASM_EXE = nasm-2.13.03/nasm

all: nasm

nasm.zip:
	curl $(NASM) > nasm.zip

$(NASM_EXE): nasm.zip
	unzip nasm.zip
	touch $(NASM_EXE)

nasm: $(NASM_EXE)
	ln -s $(NASM_EXE) nasm
