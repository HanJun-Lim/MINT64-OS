[BITS 32]

; export name for being called by C
global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text		; define text section(segment)


; return CPUID
; Paremeter : DWORD dwEAX, DWORD* pdwEAX, pdwEBX, pdwECX, pdwEDX

kReadCPUID:
	push ebp
	mov ebp, esp
	push eax
	push ebx
	push ecx
	push edx
	push esi
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; exec CPUID instruction using value of EAX
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	mov eax, dword [ebp + 8]	; store Param 1 (dwEAX) to EAX reg
	cpuid
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; store return value to parameter
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	; *pdwEAX
	mov esi, dword [ebp + 12]	; store Param 2 (pdwEAX) to ESI reg
	mov dword [esi], eax		
	
	; *pdwEBX
	mov esi, dword [ebp + 16]	; store Param 3 (pdwEBX) to ESI reg
	mov dword [esi], ebx
	
	; *pdwECX
	mov esi, dword [ebp + 20]	; store Param 4 (pdwECX) to ESI reg
	mov dword [esi], ecx
	
	; *pdwEDX
	mov esi, dword [ebp + 24]	; store Param 5 (pdwEDX) to ESI reg
	mov dword [esi], edx
	
	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	pop ebp
	ret
	
	
; convert to IA-32e mode and exec 64-bit kernel
; 	PARAM : void

kSwitchAndExecute64bitKernel:

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; set PAE, OSXMMEXCPT, OSFXSR bit on CR4
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	mov eax, cr4
	or eax, 0x620		; PAE Offset : 5, OSFXSR Offset : 9, OSXMMEXCPT Offset : 10
	mov cr4, eax
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; activate cache and address of PML4 on CR3
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	mov eax, 0x100000		; PML4 is located on 0x100000 (1MB)
	mov cr3, eax
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; set IA32-EFER.LME, IA32-EFER.SCE 
	; to activate IA-32e mode and to enable SYSCALL, SYSRET
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	mov ecx, 0xC0000080		; need to access 0xC0000080 for IA32-EFER
	rdmsr					; read MSR reg
	
	or eax, 0x0101			; set Long Mode Enable (bit 8), System Call Extensions (bit 0)
	wrmsr					; write MSR reg
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; To activate cache and Paging,
	; clear NW, CD on CR0 and set PG for cache, paging
	; set TS, EM, MP for FPU
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	mov eax, cr0
	or eax, 0xE000000E		; NW = 1, CD = 1, PG = 1, TS = 1, EM = 1, MP = 1
	xor eax, 0x60000004		; clear NW, CD, EM
	mov cr0, eax
	
	
	jmp 0x08:0x200000		; CS segment selector -> CSD for IA-32e
							; move to 0x200000 (2MB)
							
	jmp $