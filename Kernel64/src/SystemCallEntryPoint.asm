[BITS 64]

SECTION .text

extern kProcessSystemCall

global kSystemCallEntryPoint, kSystemCallTestTask

; system call entry point
; @param QWORD qwServiceNumber, PARAMETERTABLE* pstParameter
kSystemCallEntryPoint:
	push rcx	; save RIP, RFLAGS
	push r11

	mov cx, ds	; save segment selector
	push cx
	mov cx, es
	push cx
	push fs
	push gs

	mov cx, 0x10	; change to kernel data segment index
	mov ds, cx
	mov es, cx
	mov fs, cx
	mov gs, cx

	call kProcessSystemCall

	pop gs
	pop fs
	pop cx
	mov es, cx
	pop cx
	mov ds, cx
	pop r11
	pop rcx

	o64 sysret		; IA-32e SYSRET

; user level task
; @param
kSystemCallTestTask:
	mov rdi, 0xFFFFFFFF
	mov rsi, 0x00
	syscall

	mov rdi, 0xFFFFFFFF
	mov rsi, 0x00
	syscall

	mov rdi, 0xFFFFFFFF
	mov rsi, 0x00
	syscall

	mov rdi, 24
	mov rsi, 0x00
	syscall

	jmp $
