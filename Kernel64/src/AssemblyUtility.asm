[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte, kLoadGDTR, kLoadTR, kLoadIDTR

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

; set GDT Table to GDTR register
; @param address of GDT Table Data Structure
kLoadGDTR:
	lgdt [ rdi ]
	ret

; set TSS Segment Descriptor to TR register
; @param TSS Segment descriptor offset
kLoadTR:
	ltr di
	ret

; set IDT Table to IDTR register
; @param address of IDT Table Data Structure
kLoadIDTR:
	lidt [ rdi ]
	ret

