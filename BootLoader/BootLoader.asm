[ORG 0x00]		; start address of code is 0x00
[BITS 16]		; following code is 16 bit code

SECTION .text

jmp 0x07C0:START			; set CS segment register with 0x07C0 and jmp to START label

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Environment values
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TOTALSECTORCOUNT:	dw 0x02		; size of Mint64 OS excluding boot loader

KERNEL32SECTORCOUNT:	dw 0x02
BOOTSTRAPPROCESSOR:		db 0x01	; check BSP and AP
STARTGRAPHICMODE:		db 0x01	; graphic mode

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Code Section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
START:
	mov ax, 0x07C0			; ds segment register set to
	mov ds, ax				; boot loader's start address (0x7C00)
	
	mov ax, 0xB800			; es segment register set to
	mov es, ax				; video memory start address (0xB8000)

	; create stack on 0x0000:0000 ~ 0x0000:FFFF
	mov ax, 0x0000			; set ss segment register with
	mov ss, ax				; start of stack segment address (0x0000)
	mov sp, 0xFFFE
	mov bp, 0xFFFE			; init sp and bp with 0xFFFE

	; clear screen and init attribute
	mov si, 0

.SCREENCLEARLOOP:
	mov byte [ es: si ], 0			; clear video memory
	mov byte [ es: si + 1 ], 0x07	; set video memory attribute (white)

	add si, 2
	cmp si, 80 * 25 * 2				; cmp with screen size
	jl .SCREENCLEARLOOP

	; print starting message on top of screen
	push MESSAGE1					; message to print
	push 0							; y location of screen
	push 0							; x location of screen
	call PRINTMESSAGE
	add sp, 6						; clear param from stack

	; print os loading message
	push IMAGELOADINGMESSAGE
	push 1
	push 0
	call PRINTMESSAGE
	add sp, 6

	; load OS image from disk

	; reset before read disk

RESETDISK:
	; call BIOS Reset Function
	; service number 0, drive number 0 = Floppy
	mov ax, 0
	mov dl, 0
	int 0x13
	; error
	jc HANDLEDISKERROR

	; read sector from disk
	; set the address(ES:BX) to copy the disk to 0x10000
	mov si, 0x1000
	mov es, si				; set es segment register with 0x1000
	mov bx, 0x0000			; set bx register with 0x0000

	mov di, word [ TOTALSECTORCOUNT ]

READDATA:
	cmp di, 0
	je READEND
	sub di, 0x1

	; call BIOS read function
	mov ah, 0x02			; BIOS service number 2
	mov al, 0x1				; number of sector to read is 1
	mov ch, byte [ TRACKNUMBER ]
	mov cl, byte [ SECTORNUMBER ]
	mov dh, byte [ HEADNUMBER ]
	mov dl, 0x00			; drive number (0 = Floppy) to read
	int 0x13
	jc HANDLEDISKERROR

	; calculate address, track, head, sector
	add si, 0x0020
	mov es, si				; add 512 byte to es segment register

	mov al, byte [ SECTORNUMBER ]
	add al, 0x01
	mov byte [ SECTORNUMBER ], al	; inc sector number
	cmp al, 19
	jl READDATA

	xor byte [ HEADNUMBER ], 0x01	; toggle head number
	mov byte [ SECTORNUMBER ], 0x01

	cmp byte [ HEADNUMBER ], 0x00
	jne READDATA

	add byte [ TRACKNUMBER ], 0x01
	jmp READDATA

READEND:

	; print complete message
	push LOADINGCOMPLETEMESSAGE
	push 1
	push 20
	call PRINTMESSAGE
	add sp, 6

	; call VBE Service
	mov ax, 0x4F01
	mov cx, 0x117			; 1024x768 16bit(5:6:5) mode info
	mov bx, 0x07E0
	mov es, bx
	mov di, 0x00
	int 0x10
	cmp ax, 0x004F
	jne VBEERROR

	; change to graphic mode
	cmp byte [ STARTGRAPHICMODE ], 0x00
	je JUMPTOPROTECTEDMODE
	
	mov ax, 0x4F02
	mov bx, 0x4117			; 1024x768 16bit(5:6:5) linear frame buffer mode
	int 0x10
	cmp ax, 0x004F
	jne VBEERROR

	jmp JUMPTOPROTECTEDMODE

VBEERROR:
	push CHANGEGRAPHICMODEFAIL
	push 2
	push 0
	call PRINTMESSAGE
	add sp, 6
	jmp $

JUMPTOPROTECTEDMODE:
	; execute OS image
	jmp 0x1000:0x0000

HANDLEDISKERROR:
	push DISKERRORMESSAGE
	push 1
	push 20
	call PRINTMESSAGE

	jmp $

PRINTMESSAGE:
	push bp
	mov bp, sp

	push es
	push si
	push di
	push ax
	push cx
	push dx

	mov ax, 0xB800			; set es segment register
	mov es, ax				; with video memory address 0xB8000

	mov ax, word [ bp + 6 ]	; y location
	mov si, 160
	mul si
	mov di, ax

	mov ax, word [ bp + 4 ]	; x location
	mov si, 2
	mul si
	add di, ax

	mov si, word [ bp + 8 ]

.MESSAGELOOP:
	mov cl, byte [ si ]				; set cl with message
	cmp cl, 0						; 0 is end of message
	je .MESSAGEEND

	mov byte [ es: di ], cl
	add si, 1						; next message address
	add di, 2						; move to next video memory address

	jmp .MESSAGELOOP

.MESSAGEEND:
	pop dx
	pop cx
	pop ax
	pop di
	pop si
	pop es
	pop bp
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Data Section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MESSAGE1:	db 'MINT64 OS Boot Loader Start', 0		; message to print

DISKERRORMESSAGE:	db 'DISK Error!!', 0
IMAGELOADINGMESSAGE:	db 'OS Image Loading...', 0
LOADINGCOMPLETEMESSAGE:	db 'Complete!', 0
CHANGEGRAPHICMODEFAIL:	db 'Change Graphic Mode Fail!', 0

SECTORNUMBER:			db 0x02
HEADNUMBER:				db 0x00
TRACKNUMBER:			db 0x00

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
