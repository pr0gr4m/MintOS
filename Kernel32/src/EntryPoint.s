[ORG 0x00]
[BITS 16]

SECTION .text

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Code Section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
START:
	mov ax, 0x1000		; set segment register with
	mov ds, ax			; start address of protection mode
	mov es, ax			; entry point (0x10000)

	cli					; block interrupt
	lgdt [ GDTR ]		; set GDTR data structure

	; Protected Mode Attribute
	; Disable Paging, Disable Cache, Internal FPU, Disable Align Check,
	; Enable ProectedMode
	mov eax, 0x4000003B	; PG = 0, NW = 0, AM = 0, WP = 0, NE = 1, ET = 1,
	mov cr0, eax		; TS = 1, EM = 0, MP = 1, PE = 1

	; set the criteria with kernel code segment
	; cs segment selector : EIP
	jmp dword 0x08: ( PROTECTEDMODE - $$ + 0x10000 )


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Protected Mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
[BITS 32]
PROTECTEDMODE:
	mov ax, 0x10		; protected mode kernel data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; create stack on 0x00000000 ~ 0x0000FFFF
	mov ss, ax
	mov esp, 0xFFFE
	mov ebp, 0xFFFE

	push ( SWITCHSUCCESSMESSAGE - $$ + 0x10000 )
	push 2
	push 0
	call PRINTMESSAGE
	add esp, 12

	jmp dword 0x08: 0x10200		; jmp to C Kernel

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; function code section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; functino to print message
PRINTMESSAGE:
	push ebp
	mov ebp, esp
	push esi
	push edi
	push eax
	push ecx
	push edx

	mov eax, dword [ ebp + 12 ]
	mov esi, 160
	mul esi
	mov edi, eax

	mov eax, dword [ ebp + 8 ]
	mov esi, 2
	mul esi
	add edi, eax

	mov esi, dword [ ebp + 16 ]

.MESSAGELOOP:
	mov cl, byte [ esi ]
	cmp cl, 0
	je .MESSAGEEND

	mov byte [ edi + 0xB8000 ], cl
	add esi, 1
	add edi, 2
	jmp .MESSAGELOOP

.MESSAGEEND:
	pop edx
	pop ecx
	pop eax
	pop edi
	pop esi
	pop ebp
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Data Section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; align data
align 8, db 0

; align end of GDTR by 8 byte
dw 0x0000
; define GDTR
GDTR:
	dw GDTEND - GDT - 1				; size of GDT Table
	dd ( GDT - $$ + 0x10000 )		; start address of GDT Table

GDT:
	; NULL Descriptor
	NULLDescriptor:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00

	; Protected Mode Kernel Code Segment Descriptor
	CODEDESCRIPTOR:
		dw 0xFFFF					; Limit [15:0]
		dw 0x0000					; Base [15:0]
		db 0x00						; Base [23:16]
		db 0x9A						; P = 1, DPL = 0, Code Segment, Execute/Read
		db 0xCF						; G = 1, D = 1, L = 0, Limit [19:16]
		db 0x00						; Base [31:24]

	; Protected Mode Kernel Data Segment Descriptor
	DATADESCRIPTOR:
		dw 0xFFFF					; Limit [15:0]
		dw 0x0000					; Base [15:0]
		db 0x00						; Base [23:16]
		db 0x92						; P = 1, DPL = 0, Data Segment, Read/Write
		db 0xCF						; G = 1, D = 1, L = 0, Limit [19:16]
		db 0x00						; Base [31:24]
GDTEND:

SWITCHSUCCESSMESSAGE:	db 'Switch To Protected Mode Success!', 0

times 512 - ( $ - $$ ) db 0x00
