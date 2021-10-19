[BITS 64]

SECTION .text


; export internal function
global _START, ExecuteSystemCall

; import extern function
extern Main, exit



; Entry Point executed when Application Program loaded
;		underbar(_) attatched ahead to prevent link problem by overlapping function name
_START:
	call Main			; call C Entry Point function (Main)
	
	mov rdi, rax		; store return value of Main function to parameter(rdi)
	call exit			; call exit()
	
	jmp $				; not executed by exit()
	
	ret



; execute System Call
;	PARAM : QWORD qwServiceNumber, PARAMETERTABLE* pstParameter
ExecuteSystemCall:
	
	; SYSCALL instruction uses.. 
	;		RCX register to store RIP Register
	;		R11 register to store RFLAGS Register
	; previous RCX, R11 register value should be preserved
	push rcx		
	push r11
	
	syscall
	
	pop r11
	pop rcx
	
	ret
	
	
