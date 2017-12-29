
all: imagemaker

imagemaker:
	@echo
	@echo ==================== Build ImageMaker ====================
	@echo

	make -C ImageMaker
	mv ImageMaker/ImageMaker ./ImageMaker.out

	@echo
	@echo ==================== Build ImageMaker Complete ====================
	@echo

clean:
	make -C ImageMaker clean
	rm -f ImageMaker.out
