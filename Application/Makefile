all: mintlib helloworld textviewer

mintlib:
	@echo
	@echo ==================== Build UserLibrary ====================
	@echo

	make -C UserLibrary

	@echo
	@echo ==================== Build Complete ====================
	@echo

helloworld:
	@echo
	@echo ==================== Build HelloWorld ====================
	@echo

	make -C HelloWorld

	@echo
	@echo ==================== Build Complete ====================
	@echo

textviewer:
	@echo
	@echo ==================== Build TextViewer ====================
	@echo

	make -C TextViewer

	@echo
	@echo ==================== Build Complete ====================
	@echo

clean:
	make -C UserLibrary clean
	make -C HelloWorld clean
	make -C TextViewer clean
