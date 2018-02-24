
all: imagemaker serialtransfer

imagemaker:
	@echo
	@echo ==================== Build ImageMaker ====================
	@echo

	make -C ImageMaker
	mv ImageMaker/ImageMaker ./ImageMaker.out

	@echo
	@echo ==================== Build ImageMaker Complete ====================
	@echo

serialtransfer:
	@echo
	@echo ==================== Build SerialTransfer ====================
	@echo

	make -C SerialTransfer

	@echo
	@echo ==================== Build SerialTransfer Complete ====================
	@echo

clean:
	make -C ImageMaker clean
	rm -f ImageMaker.out
