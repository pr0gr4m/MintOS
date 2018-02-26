[BITS 64]

SECTION .text

extern Main
extern g_qwAPICIDAddress, g_iWakeUpApplicationProcessorCount

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

	; BSP Jump to Main
	cmp byte [ 0x7C09 ], 0x01
	je .BOOTSTRAPPROCESSORSTARTPOINT

	; Code for AP

	; Create 64KB Stack
	mov rax, 0
	mov rbx, qword [ g_qwAPICIDAddress ]
	mov eax, dword [ rbx ]
	shr rax, 24		; get APIC ID

	mov rbx, 0x10000
	mul rbx			; mul 64K

	sub rsp, rax
	sub rbp, rax

	lock inc dword [ g_iWakeUpApplicationProcessorCount ]

.BOOTSTRAPPROCESSORSTARTPOINT:
	call Main

	jmp $
