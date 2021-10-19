[BITS 64]

SECTION .text

; import extern function - Main
extern Main

; Address of APID ID Register, Count of AP waken up
extern g_qwAPICIDAddress, g_iWakeUpApplicationProcessorCount


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Code Area
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

START:
	mov ax, 0x10		; 0x10 = Data Descriptor for IA-32e Kernel
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	; create stack at 0x600000 ~ 0x6FFFFF (1MB)
	mov ss, ax			; set this to SS Segment selector
	mov rsp, 0x6FFFF8
	mov rbp, 0x6FFFF8
	
	
	; *****************************************************
	; verify Bootstrap Processor Flag in Boot Loader.
	; if Bootstrap one, move to Main function
	; *****************************************************
	
	cmp byte [0x7C09], 0x01
	je .BOOTSTRAPPROCESSORSTARTPOINT
	
	; --------------- below code executed by Application Processor only ---------------
	
	; move below 0x700000 (Top of Core's stack area) using APIC ID
	; Max supported Core is 16, so move down 0x10000 (64 KB) per one core
	; get ID from APIC ID Register of APIC ID Register, ID located at Bit 24~31
	
	mov rax, 0										; init RAX
	mov rbx, qword [g_qwAPICIDAddress]				; read APIC ID Address
	mov eax, dword [rbx]							; read APIC ID from APIC ID Register
	shr rax, 24										; move APIC ID existing at bit 24~31 to bit 0~7
	
	; calculate address of Top of stack to move
	mov rbx, 0x10000				; APIC ID in rbx
	mul rbx
	
	sub rsp, rax					; subtract RAX (distance used to move Top of Stack) from RSP, RBP 
	sub rbp, rax					; to allocate stack area for each Core
	
	; increase waken AP count
	; use lock instruction for exclusive access
	lock inc dword [g_iWakeUpApplicationProcessorCount]
	
	
.BOOTSTRAPPROCESSORSTARTPOINT
	call Main			; call C Entry Point function
	
	jmp $
	
	
	