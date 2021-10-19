[ORG 0x00]
[BITS 16]

SECTION .text


; --------------- Code Area ---------------

START:
	mov ax, 0x1000
	mov ds, ax
	mov es, ax
	
	; **************************************************************
	; if Application Processor, move to Protected Mode Kernel
	; **************************************************************
	mov ax, 0x0000						; to verify AP Flag
	mov es, ax							; set ES Segment Start Address as 0
	
	cmp byte [es:0x7C09], 0x00			; if Flag 0, means Application Processor
	je .APPLICATIONPROCESSORSTARTPOINT
	
	
	; --------------- The part executed by Bootstrap Processor only ---------------
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; activate A20 Gate
	; if BIOS service failed, try to activate using System Control Port
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	; activate A20 Gate using BIOS Service
	
	mov ax, 0x2401
	int 0x15
	
	jc .A20GATEERROR
	jmp .A20GATESUCCESS
	
	
.A20GATEERROR:
	; if error occured, try again using System Control Port
	
	in al, 0x92			; 0x92 = System Control Port
	or al, 0x02			; 00000000 OR  00000010 = 00000010
	and al, 0xFE		; 00000010 AND 11111110 = 00000010
	out 0x92, al
	
	
	; --------------- The part executed by Bootstrap / Application Processor  ---------------
	
.A20GATESUCCESS:
.APPLICATIONPROCESSORSTARTPOINT:
	cli					; interrupt not occured
	lgdt [GDTR]			; set GDTR data structure to processor, load GDT Table
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Enter to Protect Mode
	; Disabled - Paging, Cache, FPU, Align Check
	; Protect mode is only enabled
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	mov eax, 0x4000003B			; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1
	mov cr0, eax
	
	; Kernel Code Segment based on 0x00
	; re-set EIP to 0x00
	; CS Segment Selector : EIP
	
	jmp dword 0x18: (PROTECTEDMODE - $$ + 0x10000)
	
	
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Enter to Protected mode
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[BITS 32]

PROTECTEDMODE:
	mov ax, 0x20		; store "Data Segment Descriptor for Protect Mode Kernel" to AX reg
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	; create stack at 0x00000000~0x0000FFFF (size : 64KB)
	mov ss, ax
	mov esp, 0xFFFE
	mov ebp, 0xFFFE
	
	
	; **************************************************************
	; if Application Processor, move to C Kernel Entry Point
	; **************************************************************
	cmp byte [0x7C09], 0x00			; if Flag 0, means Application Processor
	je .APPLICATIONPROCESSORSTARTPOINT
	
	
	; print message that switched to Protect mode
	push (SWITCHSUCCESSMESSAGE - $$ + 0x10000)		; address of message to print
	push 2											; push Y coord to stack
	push 0											; push X coord to stack
	call PRINTMESSAGE
	add esp, 12			; eliminate inserted paremeter  ( 12 = 4(dword) * 3(num of param) )
	

.APPLICATIONPROCESSORSTARTPOINT:
	; move to 0x10200 (C Kernel is here) by modifying CS to Kernel Code Descriptor (0x08) 
	; and execute C Kernel
	jmp dword 0x18: 0x10200			
	
	

; --------------- Function Area ---------------


; Message print function
; param : x coord, y coord, string

PRINTMESSAGE:
	push ebp
	mov ebp, esp		; set EBP to ESP's value
	push esi
	push edi
	push eax
	push ecx
	push edx
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; calculate Video Memory Addr using X,Y coord
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	; get line addr using Y coord
	
	mov eax, dword [ebp + 12]		; 12 -> 4 (4 byte) * 3 (param pos)
	mov esi, 160
	mul esi
	mov edi, eax
	
	; get final addr using X coord * 2
	
	mov eax, dword [ebp + 8]		; 8 -> 4 (4 byte) * 2 (param pos)
	mov esi, 2						
	mul esi
	add edi, eax					; Y address + X address = Video Memory Address
	
	; address of string to print
	
	mov esi, dword [ebp + 16]		; 16 -> 4 (4 byte) * 4 (param pos)
	

.MESSAGELOOP:						; loop to print message
	mov cl, byte [esi]
	
	cmp cl, 0
	je .MESSAGEEND
	
	mov byte [edi + 0xB8000], cl
	
	add esi, 1			; move to next string
	add edi, 2			; move to next word position ( 1 word -> word + attribute )
	
	jmp .MESSAGELOOP
	
.MESSAGEEND:
	pop edx
	pop ecx
	pop eax
	pop edi
	pop esi
	pop ebp			; restore base pointer register
	ret



; --------------- Data Area ---------------

; added to sort below data by 8-byte 
align 8, db 0


; added to sort end of GDTR by 8-byte
dw 0x0000


; define GDTR Data Structure
GDTR:
	dw GDTEND - GDT - 1				; size of GDT Table 
	dd (GDT - $$ + 0x10000)			; start address of GDT Table

	
; define GDT Table
GDT:
	; NULL descriptor, initialized by 0
	NULLDESCRIPTOR:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00
		
	; Code Segment descriptor for IA-32e Mode (0x08)
	IA_32eCODEDESCRIPTOR:
		dw 0xFFFF	; Segment Limit[15:0] - 0xFFFF
		dw 0x0000	; Base Address[15:0] - 0x0000
		db 0x00		; Base Address[23:16] - 0x00
		db 0x9A		; P (Present), 1, 1, R (Readable)
		db 0xAF		; G (Granularity), L (Long mode segment), Segment Limit[19:16] : 0xF
		db 0x00		; Base Address[31:24] - 0x00

	; Data Segment descriptor for IA-32e Mode (0x10)
	IA_32eDATADESCRIPTOR
		dw 0xFFFF	; Segment Limit[15:0] - 0xFFFF
		dw 0x0000	; Base Address[15:0] - 0x0000
		db 0x00		; Base Address[23:16] - 0x00
		db 0x92		; P (Present), 1, 0, R (Readable)
		db 0xAF		; G (Granularity), L (Long mode segment), Segment Limit[19:16] : 0xF
		db 0x00		; Base Address[31:24] - 0x00

	; Code Segment descriptor for Protect Mode (0x18)
	CODEDESCRIPTOR:
		dw 0xFFFF	; Segment Limit[15:0] - 0xFFFF
		dw 0x0000	; Base Address[15:0] - 0x0000
		db 0x00		; Base Address[23:16] - 0x00
		db 0x9A		; P (Present), 1, 1, R (Readable)
		db 0xCF		; G (Granularity), D (Default operand size), Segment Limit[19:16] : 0xF
		db 0x00		; Base Address[31:24] - 0x00
		
	; Data Segment descriptor for Protect Mode (0x20)
	DATADESCRIPTOR:
		dw 0xFFFF	; Segment Limit[15:0] - 0xFFFF
		dw 0x0000	; Base Address[15:0] - 0x0000
		db 0x00		; Base Address[23:16] - 0x00
		db 0x92		; P (Present), 1, 0, R (Readable)
		db 0xCF		; G (Granularity), D (Default operand size), Segment Limit[19:16] : 0xF
		db 0x00		; Base Address[31:24] - 0x00
GDTEND:


; Message that switched to Protect Mode
SWITCHSUCCESSMESSAGE: db 'Switching to Protected Mode success', 0

times 512 - ($ - $$) db 0x00		; added to fill 512 byte fully
