[ORG 0x00]		; start address of code is 0x00
[BITS 16]		; following code is 16 bit code

SECTION .text

jmp 0x07C0:START			; set CS segment register with 0x07C0 and jmp to START label

START:
	mov ax, 0x07C0			; ds segment register set to
	mov ds, ax				; boot loader's start address (0x7C00)
	
	mov ax, 0xB800			; es segment register set to
	mov es, ax				; video memory start address (0xB8000)

	mov si, 0

.SCREENCLEARLOOP:
	mov byte [ es: si ], 0			; clear video memory
	mov byte [ es: si + 1 ], 0x0A	; set video memory attribute (light green)

	add si, 2
	cmp si, 80 * 25 * 2				; cmp with screen size
	jl .SCREENCLEARLOOP

	
	mov si, 0
	mov di, 0

.MESSAGELOOP:
	mov cl, byte [ si + MESSAGE1 ]	; set cl with message
	cmp cl, 0						; 0 is end of message
	je .MESSAGEEND

	mov byte [ es: di ], cl
	add si, 1						; next message address
	add di, 2						; move to next video memory address

	jmp .MESSAGELOOP

.MESSAGEEND:

	jmp $			; loop at this location

MESSAGE1:	db 'MINT64 OS Boot Loader Start', 0		; message to print

times 510 - ( $ - $$ )	db 0x00		; $ : current line address
									; $$ : start of current section(.text) address
									; $ - $$ : offset from current section
									; 510 - ( $ - $$ ) : from current to address 510
									; db 0x00 : define 1 byte and init with 0x00
									; time : iteration
									; init with 0x00 from current to 510

db 0x55			; define 1 byte and init with 0x55
db 0xAA			; define 1 byte and init with 0xAA
				; 0x55, 0xAA is symbol of boot loader
