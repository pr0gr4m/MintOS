# Makefile to build all subdirs and make disk image

all: bootloader kernel32 Disk.img

bootloader:
	@echo
	@echo ==================== Build Boot Loader ====================
	@echo

	make -C BootLoader

	@echo
	@echo ==================== Build Complete ====================
	@echo

kernel32:
	@echo
	@echo ==================== Build 32bit Kernel ====================
	@echo

	make -C Kernel32

	@echo
	@echo ==================== Build Complete ====================
	@echo

Disk.img: BootLoader/BootLoader.bin Kernel32/Kernel32.bin
	@echo
	@echo ==================== Disk Image Build Start ====================
	@echo

	make -C Utility/ImageMaker
	./Utility/ImageMaker/ImageMaker $^

	@echo
	@echo ==================== All Build Complete ====================
	@echo

clean:
	make -C BootLoader clean
	make -C Kernel32 clean
	rm -f Disk.img
