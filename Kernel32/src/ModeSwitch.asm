[BITS 32]

global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text

; return CPUID
; @param DWORD dwEAX, DWORD* pdwEAX,* pdwEBX,* pdwECX,* pdwEDX
kReadCPUID:
	push ebp
	mov ebp, esp
	push eax
	push ebx
	push ecx
	push edx
	push esi

	; execute CPUID inst
	mov eax, dword [ ebp + 8 ]		; mov param 1 to eax
	cpuid

	; store return values
	; *pdwEAX
	mov esi, dword [ ebp + 12 ]
	mov dword [ esi ], eax

	; *pdwEBX
	mov esi, dword [ ebp + 16 ]
	mov dword [ esi ], ebx

	; *pdwECX
	mov esi, dword [ ebp + 20 ]
	mov dword [ esi ], ecx

	; *pdwEDX
	mov esi, dword [ ebp + 24 ]
	mov dword [ esi ], edx

	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	pop ebp
	ret

; switch to IA-32e mode
; @param
kSwitchAndExecute64bitKernel:
	; set PAE bit of CR4 control register to 1
	mov eax, cr4
	or eax, 0x620		; PAE bit (bit 5)
	mov cr4, eax

	; set PML4 table address and activate cache at CR3
	mov eax, 0x100000		; PML4 table address (1MB)
	mov cr3, eax

	; set IA32_EFER.LME and IA32_EFER.SCE to 1
	mov ecx, 0xC0000080		; store IA32_EFER MSR register address
	rdmsr
							; rdmsr return lower value to eax 
	or eax, 0x0101			; set LME, SCE bit 1

	wrmsr

	; set CR0 register to NW bit 0, CD bit 0, PG bit 1
	; activate cache and paging
	mov eax, cr0
	or eax, 0xE000000E			; set NW (bit 29), CD (bit 30), PG (bit 31) 
								; TS (bit 3), EM (bit 2), MP(bit 1) to 1
	xor eax, 0x60000004			; set NW (bit 29), CD (bit 30), EM (bit 2) to 0
	mov cr0, eax

	jmp 0x08:0x200000			; set cs segment selector to IA-32e mode code descriptor
								; move to 2MB address

	; couldn't reach
	jmp $

