# Makefile to build all subdirs and make disk image

all: BootLoader Disk.img

BootLoader/BootLoader.bin:
	@echo
	@echo ==================== Build Boot Loader ====================
	@echo

	make -C BootLoader

	@echo
	@echo ==================== Build Complete ====================
	@echo

Disk.img: BootLoader/BootLoader.bin
	@echo
	@echo ==================== Disk Image Build Start ====================
	@echo

	cp BootLoader/BootLoader.bin Disk.img

	@echo
	@echo ==================== All Build Complete ====================
	@echo

clean:
	make -C BootLoader clean
	rm -f Disk.img
