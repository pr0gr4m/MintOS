[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte, kInPortWord, kOutPortWord
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kReadTSC
global kSwitchContext, kHlt, kTestAndSet, kPause
global kInitializeFPU, kSaveFPUContext, kLoadFPUContext, kSetTS, kClearTS
global kEnableGlobalLocalAPIC
global kReadMSR, kWriteMSR

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

; read 2 bytes from port
; @param port number
kInPortWord:
	push rdx

	mov rdx, rdi
	mov rax, 0
	
	in ax, dx

	pop rdx
	ret

; write 2 bytes to port
; @param port number, data
kOutPortWord:
	push rdx
	push rax

	mov rdx, rdi
	mov rax, rsi

	out dx, ax

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

; Enable Interrupt
; @param
kEnableInterrupt:
	sti
	ret

; Disable Interrupt
; @param
kDisableInterrupt:
	cli
	ret

; Read RFLAGS
; @param
kReadRFLAGS:
	pushfq
	pop rax
	ret

; return time stamp counter
; @param
kReadTSC:
	push rdx

	rdtsc

	shl rdx, 32
	or rax, rdx

	pop rdx
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function for Task
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; macro that saves context and changes selector
%macro KSAVECONTEXT 0
	push rbp
	push rax
	push rbx
	push rcx
	push rdx
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov ax, ds
	push rax
	mov ax, es
	push rax
	push fs
	push gs
%endmacro

%macro KLOADCONTEXT 0
	pop gs
	pop fs
	pop rax
	mov es, ax
	pop rax
	mov ds, ax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	pop rbp
%endmacro

; Save context to Current Context
; and load context from Next Context
; @param Current Context, Next Context
kSwitchContext:
	push rbp
	mov rbp, rsp

	pushfq		; save RFLAGS to not change by cmp
	cmp rdi, 0
	je .LoadContext
	popfq		; load RFLAGS

	push rax

	mov ax, ss
	mov qword[rdi + (23 * 8)], rax

	mov rax, rbp		; save RSP except rbp, return address
	add rax, 16
	mov qword[rdi + (22 * 8)], rax

	pushfq				; save RFLAGS
	pop rax
	mov qword[rdi + (21 * 8)], rax

	mov ax, cs
	mov qword[rdi + (20 * 8)], rax

	mov rax, qword[rbp + 8]		; save return address to RIP register
	mov qword[rdi + (19 * 8)], rax

	pop rax
	pop rbp

	add rdi, (19 * 8)
	mov rsp, rdi
	sub rdi, (19 * 8)

	KSAVECONTEXT

.LoadContext:
	mov rsp, rsi

	KLOADCONTEXT
	iretq

; Halt Processor
; @param
kHlt:
	hlt
	hlt
	ret

; Pause Processor
; @param
kPause:
	pause
	ret

; Atomic Test and Set
; Compare Destination with Compare and set Source to Destination when same
; @param Destination, Compare, Source
kTestAndSet:
	mov rax, rsi

	lock cmpxchg byte[rdi], dl
	je .SUCCESS

.NOTSAME:
	mov rax, 0x00
	ret

.SUCCESS:
	mov rax, 0x01
	ret

;;;;;;;;;;;;;;;;;;;;
; FPU Functions
;;;;;;;;;;;;;;;;;;;;

; Init FPU
; @param
kInitializeFPU:
	finit
	ret

; Save FPU Context to Buffer
; @param Buffer
kSaveFPUContext:
	fxsave [rdi]
	ret

; Load FPU Context from Buffer
; @param Buffer
kLoadFPUContext:
	fxrstor [rdi]
	ret

; Set TS bit of CR0 to 1
; @param
kSetTS:
	push rax

	mov rax, cr0
	or rax, 0x08
	mov cr0, rax

	pop rax
	ret

; Set TS bit of CR0 to 0
; @param
kClearTS:
	clts
	ret

; Enable APIC to set IA32_APIC_BASE MSR register bit 11 to 1
; @param
kEnableGlobalLocalAPIC:
	push rax
	push rcx
	push rdx

	mov rcx, 27		; register address 27
	rdmsr			; MSR use edx(upper 32) and eax(lower 32)

	or eax, 0x0800
	wrmsr

	pop rdx
	pop rcx
	pop rax
	ret

; Read from MSR register
; @param QWORD qwMSRAddress, QWORD* pqwRDX, QWORD* pqwRAX
kReadMSR:
	push rdx
	push rax
	push rcx
	push rbx

	mov rbx, rdx
	mov rcx, rdi

	rdmsr

	mov qword [ rsi ], rdx
	mov qword [ rbx ], rax

	pop rbx
	pop rcx
	pop rax
	pop rdx
	ret

; Write to MSR register
; @param QWORD qwMSRAddress, QWORD pqwRDX, QWORD pqwRAX
kWriteMSR:
	push rdx
	push rax
	push rcx

	mov rcx, rdi
	mov rax, rdx
	mov rdx, rsi

	wrmsr

	pop rcx
	pop rax
	pop rdx
	ret
