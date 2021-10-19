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


; read 1 byte from port
; 	PARAM : port_num
kInPortByte:
	push rdx
	
	mov rdx, rdi	; RDX = param 1 (port_num)
	mov rax, 0		; init RAX, used to return value
	in al, dx		; AL = *DX
	
	pop rdx
	ret

; write 1 byte to port
; 	PARAM : port_num, data
kOutPortByte:
	push rdx
	push rax
	
	mov rdx, rdi	; RDX = param 1 (port_num)
	mov rax, rsi	; RAX = param 2 (data)
	out dx, al		; *DX = AL
	
	pop rax
	pop rdx
	ret
	

; read 2 bytes from port
; 	PARAM : port_num
kInPortWord:
	push rdx
	
	mov rdx, rdi	; RDX = param 1 (port_num)
	mov rax, 0		; init RAX, used to return value
	in ax, dx		; AX = *DX
	
	pop rdx
	ret

; write 2 bytes to port
; 	PARAM : port_num, data
kOutPortWord:
	push rdx
	push rax
	
	mov rdx, rdi	; RDX = param 1 (port_num)
	mov rax, rsi	; RAX = param 2 (data)
	out dx, ax		; *DX = AX
	
	pop rax
	pop rdx
	ret
	
	
; set GDT (Global Descriptor Table) on GDTR Register
;	PARAM : address of data structure saving information of GDT Table
kLoadGDTR:
	lgdt [rdi]
	ret
	
; set TSS (Task Status Segment) Segment Descriptor on TR Register
;	PARAM : offset value of TSS Segment Descriptor
kLoadTR:
	ltr	di
	ret

; set IDT (Interrupt Descriptor Table) on IDTR Register
;	PARAM : address of data structure saving informationo of IDT Table
kLoadIDTR:
	lidt [rdi]
	ret
	
	
; Enable Interrupt
;	PARAM : no
kEnableInterrupt:
	sti
	ret

; Disable Interrupt
;	PARAM : no
kDisableInterrupt:
	cli
	ret
	
; read and return RFLAGS value
; 	PARAM : no
kReadRFLAGS:
	pushfq		; save RFLAGS val to Stack
	pop rax
	
	ret
	
	
; read and return Time Stamp Counter
; 	PARAM : no
kReadTSC:
	push rdx
	
	rdtsc		; read Time Stamp Counter and store in RDX:RAX
	
	shl rdx, 32
	or rax, rdx
	
	pop rdx
	ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Assembly Function for Task
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Macro to save Context, change Selector
%macro KSAVECONTEXT 0		;	PARAM : no
	
	; insert Register from RBP to GS into Stack
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
	
	mov ax, ds		; DS, ES cannot be inserted directly
	push rax
	mov ax, es
	push rax
	push fs
	push gs
	
%endmacro


; Macro to restore Context
%macro KLOADCONTEXT 0
	; restore Register from GS to RBP
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


; save Current Context in "Current Context", restore context from "Next Context"
;	PARAM : Current Context (rdi), Next Context (rsi)
kSwitchContext:
	push rbp			; function prologue
	mov rbp, rsp		; rbp = rsp
	
	; if Current Context is NULL, no need to save Context
	pushfq		; push RFLAGS register into Stack to prevent modification from result of "cmp"
	
	cmp rdi, 0		; if "Current Context" is empty..
	je .LoadContext
	
	popfq

	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; save Context of "Current Task"
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	push rax		; used as "Context Offset"
	
	; insert SS, RSP, RFLAGS, CS, RIP in order
	mov ax, ss
	mov qword[rdi + (23 * 8)], rax		; SSOFFSET - 23
	
	mov rax, rbp						; RSP is saved in RBP
	add rax, 16							; push rbp(8), Return Address(8)
	mov qword[rdi + (22 * 8)], rax		; RSPOFFSET - 22
	
	pushfq
	pop rax
	mov qword[rdi + (21 * 8)], rax		; RFLAGSOFFSET - 21
	
	mov ax, cs
	mov qword[rdi + (20 * 8)], rax
	
	mov rax, qword[rbp + 8]				; set RIP Register as "Return Address"
	mov qword[rdi + (19 * 8)], rax		; move to function called position when "next context" restored

	pop rax
	pop rbp
	
	; change stack pointer to save Context using "push"
	add rdi, (19 * 8)
	mov rsp, rdi
	sub rdi, (19 * 8)
	
	; save rest of Register into CONTEXT Structure
	KSAVECONTEXT
	
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; restore next Context
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.LoadContext
	mov rsp, rsi
	
	; restore Register from CONTEXT
	KLOADCONTEXT
	
	iretq		; restore SS, RSP, RFLAGS, CS, RIP
	
	
; Halt Processor
; 	PARAM : No
kHlt:
	hlt			; change CPU statement to Halt
	hlt
	ret

	
	
; Handle Test and Set at once (CMPXCHG A, B)
; if Dst == Cmp, insert Src into Dst
; 	PARAM : Destination (rdi), Compare (rsi), Source (rdx)
kTestAndSet:
	mov rax, rsi		; rax = Compare (rsi) 
	
	lock cmpxchg byte[rdi], dl
	je .SUCCESS			; move .SUCCESS when ZF is 1

.NOTSAME:		; Dst != Cmp
	mov rax, 0x00
	ret

.SUCCESS		; Dst == Cmp
	mov rax, 0x01
	ret



; for FPU

; Initialize FPU
; 	PARAM : No
kInitializeFPU:
	finit		; initialize FPU
	ret


; Save FPU-related Register to Context Buffer
;	PARAM : Address of Context Buffer
kSaveFPUContext:
	fxsave [rdi]	; Param 1
	ret


; Load FPU-related Register from Context Buffer
;	PARAM : Address of Context Buffer
kLoadFPUContext:
	fxrstor [rdi]	; Param 1
	ret

	
; Set TS bit in CR0
;	PARAM : No
kSetTS:
	push rax
	
	mov rax, cr0
	or rax, 0x08		; TS Offset : 3
	mov cr0, rax
	
	pop rax				; restore RAX
	ret

	
; Clear TS bit in CR0
;	PARAM : No
kClearTS:
	clts
	ret
	
	
; set Enable Global Local APIC bit (bit 11) in IA32_APIC_BASE MSR to activate APIC 
; 	PARAM : No
kEnableGlobalLocalAPIC:
	push rax			; EAX (means lower 32 bit of MSR Register)
	push rcx			; ECX (means MSR Register Address)
	push rdx			; EDX (means upper 32 bit of MSR Register)
						; These Regs are used by rdmsr, wrmsr command

	mov rcx, 27			; IA32_APIC_BASE MSR located at Register Address 27
	rdmsr				; read value saved at EAX (lower 32 bit), EDX (upper 32 bit)
	
	
	or eax, 0x0800		; Enable/Disable Global APIC bit located at bit 11
	wrmsr				; overwrite value to MSR Register

	pop rdx
	pop rcx
	pop rax
	
	ret
	
	
; make CPU rest
; 	PARAM : No
kPause:
	pause				; make processor enter into pause state
	ret
	
	
; read value from the MSR Register
; 	PARAM : QWORD qwMSRAddress (rdi), QWORD* pqwRDX (rsi), QWORD* pqwRAX (rdx)
kReadMSR:
	push rdx
	push rax
	push rcx
	push rbx

	mov rbx, rdx		; store pqwRAX (3rd PARAM) to RBX temporarily
	
	mov rcx, rdi		; store qwMSRAddress (1st PARAM) to RCX
	rdmsr				; "rdmsr" command uses RCX for target address, RDX(upper 32), RAX(lower 32) for storing

	
	mov qword[rsi], rdx		; *pqwRDX = RDX (upper 32 bit);
	mov qword[rbx], rax		; *pqwRAX = RAX (lower 32 bit);
	
	pop rbx
	pop rcx
	pop rax
	pop rdx
	
	ret


; write value to the MSR Register
;	PARAM : QWORD qwMSRAddress (rdi), QWORD qwRDX (rsi), QWORD qwRAX (rdx)
kWriteMSR:
	push rdx
	push rax
	push rcx
	
	mov rcx, rdi		; store MSR Address (1st PARAM) to RCX
	mov rax, rdx		; store lower 32 bit (3rd PARAM) to RAX
	mov rdx, rsi		; store upper 32 bit (2nd PARAM) to RDX
	wrmsr				; "wrmsr" command uses RCX for target address, RDX(upper 32), RAX(lower 32) for writing
	
	pop rcx
	pop rax
	pop rdx
	
	ret
	
	
	