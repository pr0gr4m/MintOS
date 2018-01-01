[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte

; read from port
; @param port number
kInPortByte:
	push rdx

	mov rdx, rdi
	mov rax, 0

	in al, dx

	pop rdx
	ret

; write to port
; @param port number, data
kOutPortByte:
	push rdx
	push rax

	mov rdx, rdi
	mov rax, rsi

	out dx, al

	pop rax
	pop rdx
	ret
