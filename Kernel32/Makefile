all: Kernel32.bin

Kernel32.bin: src/EntryPoint.s
	nasm -o Kernel32.bin $<

clean:
	rm -f Kernel32.bin
