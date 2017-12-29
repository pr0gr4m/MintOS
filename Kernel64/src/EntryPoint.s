[BITS 64]

SECTION .text

extern Main

; Code Section
START:
	mov ax, 0x10			; IA-32e Mode Kernel Data Segment Descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; Create stack on 0x600000 ~ 0x6FFFFF (1MB)
	mov ss, ax
	mov rsp, 0x6FFFF8
	mov rbp, 0x6FFFF8

	call Main

	jmp $
