# Set Configuration
NASM64 = nasm -f elf64
GCC64 = gcc -c -m64 -ffreestanding
AR64 = ar rcs

OBJECTDIRECTORY = tmp
SOURCEDIRECTORY = src

# build contents

all: prepare libmint

prepare:
	mkdir -p $(OBJECTDIRECTORY)

dep:
	@echo ===== Make Dependency File =====
	make -C $(OBJECTDIRECTORY) -f ../Makefile InternalDependency
	@echo ===== Dependency Search Complete =====

libmint: dep
	make -C $(OBJECTDIRECTORY) -f ../Makefile libmint.a
	cp $(OBJECTDIRECTORY)/libmint.a ./

clean:
	rm -f *.a
	rm -f $(OBJECTDIRECTORY)/*.*

# Recall, based on tmp directory
CSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.c)
ASSEMBLYSOURCEFILES = $(wildcard ../$(SOURCEDIRECTORY)/*.asm)
COBJECTFILES = $(notdir $(patsubst %.c,%.o,$(CSOURCEFILES)))
ASSEMBLYOBJECTFILES = $(notdir $(patsubst %.asm,%.o,$(ASSEMBLYSOURCEFILES)))

%.o: ../$(SOURCEDIRECTORY)/%.c
	$(GCC64) -c $<

%.o: ../$(SOURCEDIRECTORY)/%.asm
	$(NASM64) -o $@ $<

InternalDependency:
	$(GCC64) -MM $(CSOURCEFILES) > Dependency.dep

libmint.a: $(ASSEMBLYOBJECTFILES) $(COBJECTFILES)
	$(AR64) $@ $^

ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif
