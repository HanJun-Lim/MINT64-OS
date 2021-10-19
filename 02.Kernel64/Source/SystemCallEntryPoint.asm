[BITS 64]				; below code is 64 bit code

SECTION .text			; define text section(segment)


; import external function
extern kProcessSystemCall

; export internal function
global kSystemCallEntryPoint, kSystemCallTestTask


; System Call Entry Point, executed when SYSCALL on User Level
; 	PARAM : QWORD qwServiceNumber, PARAMETERTABLE* pstParameter
kSystemCallEntryPoint:

	; change DS, ES, FS, GS to Kernel Data Segment to allow Kernel-Level Task run Kernel code
	; 	according to x86-64 ABI, no need to restore RCX, R11.
	; 	but RCX, R11 are preserved to prevent the system from unexpected situation
	push rcx 					; store RIP, RFLAGS register that used to return by SYSRET
	push r11	
	mov cx, ds
	push cx
	mov cx, es
	push cx
	push fs
	push gs

	mov cx, 0x10
	mov ds, cx
	mov es, cx
	mov fs, cx
	mov gs, cx
	
	call kProcessSystemCall		; PARAM : QWORD qwServiceNumber, PARAMETERTABLE* pstParameter
	
	pop gs
	pop fs
	pop cx
	mov es, cx
	pop cx
	mov ds, cx
	pop r11
	pop rcx
	
	o64 sysret					; o64 offered by NASM sets REX prefix in front of sysret
								; REX prefix determine operand size, To set REX prefix means 64-bit operand
								; if W bit of REX prefix set, SYSRET enters 64-bit sub-mode user level, 
								; otherwise enters protected mode
	
	
kSystemCallTestTask:
	
	mov rdi, 0xFFFFFFFF		; PARAM 1 - qwServiceNumber : SYSCALL_TEST (0xFFFFFFFF)
	mov rsi, 0x00			; PARAM 2 - pstParameter : NULL (No parameter)
	syscall
	
	mov rdi, 0xFFFFFFFF		; PARAM 1 - qwServiceNumber : SYSCALL_TEST (0xFFFFFFFF)
	mov rsi, 0x00			; PARAM 2 - pstParameter : NULL (No parameter)
	syscall
	
	mov rdi, 0xFFFFFFFF		; PARAM 1 - qwServiceNumber : SYSCALL_TEST (0xFFFFFFFF)
	mov rsi, 0x00			; PARAM 2 - pstParameter : NULL (No parameter)
	syscall
	
	mov rdi, 24				; PARAM 1 - qwServiceNumber : SYSCALL_EXITTASK (24)
	mov rsi, 0x00			; PARAM 2 - pstParameter : NULL (No parameter)
	syscall
	
	jmp $
	
	
	