# Set Configuration
NASM64 = nasm -f elf64
GCC64 = gcc -c -m64 -ffreestanding
LD64 = ld -melf_x86_64 -T ../elf_x86_64.x -nostdlib -e Main -Ttext 0x200000
OBJCOPY64 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

OBJECTDIRECTORY = tmp
SOURCEDIRECTORY = src

# build contents

all: prepare Kernel64.bin

prepare:
	mkdir -p $(OBJECTDIRECTORY)

$(OBJECTDIRECTORY)/EntryPoint.bin: $(SOURCEDIRECTORY)/EntryPoint.s
	$(NASM32) -o $@ $<

dep:
	@echo ===== Make Dependency File =====
	make -C $(OBJECTDIRECTORY) -f ../Makefile InternalDependency
	@echo ===== Dependency Search Complete =====

ExecuteInternalBuild: dep
	make -C $(OBJECTDIRECTORY) -f ../Makefile Kernel64.elf

Kernel64.bin: ExecuteInternalBuild
	$(OBJCOPY64) $(OBJECTDIRECTORY)/Kernel64.elf $@

clean:
	rm -f *.bin
	rm -f $(OBJECTDIRECTORY)/*.*

# Recall, based on tmp directory
ENTRYPOINTSOURCEFILE = ../$(SOURCEDIRECTORY)/EntryPoint.s
ENTRYPOINTOBJECTFILE = EntryPoint.o
CSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*c)
ASSEMBLYSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.asm)
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))
ASSEMBLYOBJECTFILES = $(notdir $(patsubst %.asm,%.o,$(ASSEMBLYSOURCEFILES)))

$(ENTRYPOINTOBJECTFILE): $(ENTRYPOINTSOURCEFILE)
	$(NASM64) -o $@ $<

%.o: ../$(SOURCEDIRECTORY)/%.c
	$(GCC64) -c $<

%.o: ../$(SOURCEDIRECTORY)/%.asm
	$(NASM64) -o $@ $<

InternalDependency:
	$(GCC64) -MM $(CSOURCEFILES) > Dependency.dep

Kernel64.elf: $(ENTRYPOINTOBJECTFILE) $(COBJECTFILES) $(ASSEMBLYOBJECTFILES)
	$(LD64) -o $@ $^

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
