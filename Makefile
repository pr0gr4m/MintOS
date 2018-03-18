# Makefile to build all subdirs and make disk image

all: bootloader kernel32 kernel64 Disk.img application

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

kernel64:
	@echo
	@echo ==================== Build 64bit Kernel ====================
	@echo

	make -C Kernel64

	@echo
	@echo ==================== Build Complete ====================
	@echo

Disk.img: BootLoader/BootLoader.bin Kernel32/Kernel32.bin Kernel64/Kernel64.bin
	@echo
	@echo ==================== Disk Image Build Start ====================
	@echo

	make -C Utility
	./Utility/ImageMaker.out $^

	@echo
	@echo ==================== All Build Complete ====================
	@echo

application:
	@echo
	@echo ==================== Build Application ====================
	@echo

	make -C Application

	@echo
	@echo ==================== Build Complete ====================
	@echo
	

clean:
	make -C BootLoader clean
	make -C Kernel32 clean
	make -C Kernel64 clean
	make -C Application clean
	make -C Utility clean
	rm -f Disk.img
