# Set Configuration
NASM64 = nasm -f elf64
GCC64 = gcc -c -m64 -ffreestanding -mcmodel=large -fno-common
LD64 = ld -melf_x86_64 -T ../elf_x86_64.x -nostdlib -e _START -Ttext 0x0000
LDFLAGS = -lmint -L ../../UserLibrary
CFLAGS = -I ../../UserLibrary/src

OBJCOPY64 = objcopy -j .text -j .data -j .rodata -j .bss

OBJECTDIRECTORY = tmp
SOURCEDIRECTORY = src

# build contents

all: prepare Output.elf

prepare:
	mkdir -p $(OBJECTDIRECTORY)

dep:
	@echo ===== Make Dependency File =====
	make -C $(OBJECTDIRECTORY) -f ../Makefile InternalDependency
	@echo ===== Dependency Search Complete =====

ExecuteInternalBuild: dep
	make -C $(OBJECTDIRECTORY) -f ../Makefile Main.elf

Output.elf: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJECTDIRECTORY)/Main.elf $@

clean:
	rm -f *.elf
	rm -f $(OBJECTDIRECTORY)/*.*

# Recall, based on tmp directory
CSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.c)
ASSEMBLYSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.asm)
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))
ASSEMBLYOBJECTFILES = $(notdir $(patsubst %.asm,%.o,$(ASSEMBLYSOURCEFILES)))

%.o: ../$(SOURCEDIRECTORY)/%.c
	$(GCC64) -c $< $(CFLAGS)

%.o: ../$(SOURCEDIRECTORY)/%.asm
	$(NASM64) -o $@ $<

InternalDependency:
	$(GCC64) -MM $(CSOURCEFILES) > Dependency.dep $(CFLAGS)

Main.elf: $(COBJECTFILES) $(ASSEMBLYOBJECTFILES)
	$(LD64) -r -o $@ $^ $(LDFLAGS)

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
