[BITS 64]

SECTION .text

global ExecuteSystemCall

; execute system call
; @param QWORD qwServiceNumber, PARAMETERTABLE* pstParameter
ExecuteSystemCall:
	push rcx
	push r11

	syscall

	pop r11
	pop rcx
	ret
