[BITS 64]

SECTION .text

global _START, ExecuteSystemCall

extern Main, exit

; entry point for user application
_START:
	call Main

	mov rdi, rax
	call exit

	jmp $

	ret

; execute system call
; @param QWORD qwServiceNumber, PARAMETERTABLE* pstParameter
ExecuteSystemCall:
	push rcx
	push r11

	syscall

	pop r11
	pop rcx
	ret
