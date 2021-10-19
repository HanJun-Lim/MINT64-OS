[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x07C0:START



; --------------------   MINT64 OS Configuration value	 --------------------									

TOTALSECTORCOUNT:		dw 0x02		; total sector number of MINT64 OS Image (except boot loader)
									; Maximum : 1152 sector (0x90000 byte)
									; value is updated by ImageMaker.c

KERNEL32SECTORCOUNT: 	dw 0x02		; total sector number of Protected Mode Kernel

BOOTSTRAPPROCESSOR: 	db 0x01		; check if Bootstrap Processor

STARTGRAPHICMODE: 		db 0x01		; check if OS starts with Graphic Mode

; --------------------   Code Area	 --------------------									

START:
	mov ax, 0x07C0		; Boot Loader starts from 0x7C00, store this segment at DS
	mov ds, ax
	mov ax, 0xB800		; Video Memory Address is 0xB8000, store this segment at ES
	mov es, ax

	; Create stack in area 0x0000:0000 ~ 0x0000:FFFF with 64KB size
	mov ax, 0x0000
	mov ss, ax			; Stack Segment : 0x0000
	mov sp, 0xFFFE		; Stack Pointer : 0xFFFE
	mov bp, 0xFFFE		; Base Pointer  : 0xFFFE
	
	mov byte [BOOTDRIVE], dl 		; store boot drive number into memory
									; DL register contains device number that boot loader loaded


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Clear screen and set attribute as Green		
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	mov si, 0



.SCREENCLEARLOOP:
	mov byte [es:si], 0			; (CHARACTER*)0xB8000->bCharacter = 0 (no character)
	mov byte [es:si+1], 0x0A		; (CHARACTER*)0xB8000->bAttribute = 0x0A (Green)
	add si, 2						; go to next character
	
	cmp si, 80*25*2					; 80*25 (number of screen characters) * character attributes

	jl .SCREENCLEARLOOP
	

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; print booting start message					
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	push MESSAGE1		; 
	push 0				; Y coord
	push 0				; X coord
	call PRINTMESSAGE	; call function
	add	 sp, 6			; 6 = 2 byte * 3 arguments (cdecl)


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; print OS Image loading message				
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	push IMAGELOADINGMESSAGE
	push 1
	push 0
	call PRINTMESSAGE
	add	 sp, 6


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; load OS Image in Disk							
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Reset before reading Disk						
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



RESETDISK:			; Disk Reset code

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Call BIOS Reset Function						
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	mov ax, 0					; BIOS service number 0 : Reset Disk System
	mov dl, byte [BOOTDRIVE]	; Drive Number : decided by BOOTDRIVE (0 = Floppy)
	int 0x13					; BIOS function call : int 13h for disk I/O function

	jc HANDLEDISKERROR			; if error occured, jump to error handling
	

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Read Disk Parameter							
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	mov ah, 0x08				; BIOS service number 8 (Read Disk Parameter)
	mov dl, byte [BOOTDRIVE]	; Drive Number : USB Flash Drive
	int 0x13					; BIOS function call : int 13h for disk I/O function
	
	jc HANDLEDISKERROR			; if error occured, jump to error handling
	
	mov byte [LASTHEAD], dh	; store head info into memory (DH : logical last index of heads)
	mov al, cl					; sector, track information into AL register
	and al, 0x3F				; CX bit [15:6] : logical last index of track or cylinder
								; CX bit [5:0] : logical last index of sectors per track
	
	mov byte [LASTSECTOR], al	; store sector info into memory
	mov byte [LASTTRACK], ch	; store upper 8 bits of track info into memory


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Read sector in Disk							
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	; set ES:BX to 0x10000
	mov si, 0x1000			; OS Image copied at 0x10000
	mov es, si
	mov bx, 0x0000			; set address to copy to 0x1000:0000
	
	;mov di, word [TOTALSECTORCOUNT]	; total sector number of OS image into di register
	mov di, 1146						; set DI register to 1146 sector (573 KB)
										; 	..to load package file behind of OS image



READDATA:			; Disk Read code
	
	; check all sector is read
	cmp di, 0					; is OS image copy end?
	je  READEND	
	sub di, 0x1	
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Call BIOS Read Function						
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	mov ah, 0x02					; BIOS service number 2 (Read Sector)
	mov al, 0x1						; read 1 sector (0x1)
	mov ch, byte [TRACKNUMBER]		; set track number to read
	mov cl, byte [SECTORNUMBER]	; set sector number to read
	mov dh, byte [HEADNUMBER]		; set head number to read
	mov dl, byte [BOOTDRIVE]		; set drive number to read (0 = Floppy)

	int 0x13						; BIOS function call : int 13h for disk I/O function
	jc HANDLEDISKERROR


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Calculate address to copy and head, sector address		
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	add si, 0x0020			; 512 bytes (1 sector) add (read)
	mov es, si				; apply to ES register
	
	
	; CHS (Cylinder - Head - Sector)
	;
	; one sector is read
	; 		increase sector number, check if last sector(18) is read
	;		if not the last sector, move to READDATA (continue sector reading)
	mov al, byte [SECTORNUMBER]	; AL : sector number
	add al, 0x01					; increase sector number 
	mov byte [SECTORNUMBER], al	; set increased sector number into SECTORNUMBER
	
	cmp al, byte [LASTSECTOR]		; is increased sector number equals to last sector number?
	jbe READDATA
	
	
	; if last sector is read, increase head number, reset sector number 1
	add byte [HEADNUMBER], 0x01	
	mov byte [SECTORNUMBER], 0x01


	; if all head is read, increase track number
	mov al, byte [LASTHEAD]		; AL : last head number
	cmp byte [HEADNUMBER], al		; is all head is read?
	ja .ADDTRACK					; if yes, increase track number
	
	jmp READDATA
	
.ADDTRACK:
	
	; increase track number, jump to READDATA (read sector) again
	mov byte [HEADNUMBER], 0x00		; reset head number
	add byte [TRACKNUMBER], 0x01	; increase track number
	
	jmp READDATA
	
	
READEND:

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; print message that OS Image Loading completed	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	push LOADINGCOMPLETEMESSAGE
	push 1
	push 20
	call PRINTMESSAGE
	add  sp, 6

	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; call VBE Function No.0x4F01 
	; 	to get Mode Information Block for Graphic Mode
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	mov ax, 0x4F01			; 0x4F01 - returns VBE Mode information	
	mov cx, 0x117			; Resolution : 1024 * 768, Color Mode : 16 bit [R(5):G(6):B(5)]
	mov bx, 0x07E0
	mov es, bx
	mov di, 0x00			; Mode Information Block saved in 0x07E0:0000 (0x7E00)
	int 0x10				; occur interrupt (0x10 for video display functions)
	cmp ax, 0x004F			; 0x004F means success
	jne VBEERROR
	
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; call VBE Function No.0x4F02
	; 	to go to Graphic Mode
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	; check if STARTGRAPHICMODE Flag set
	; 	if set, go to Graphic Mode
	cmp byte [STARTGRAPHICMODE], 0x00
	je  JUMPTOPROTECTEDMODE					; if not set, jump to Protected Mode
	
	mov ax, 0x4F02			; 0x4F02 - VBE Mode setting
	mov bx, 0x4117 			; Mode Number (bit 0~8) - 0x117 (1024 * 768, 16 bit color [R(5):G(6):B(5)]
							; Buffer Mode (bit 14)  - Linear Frame Buffer
	int 0x10				; occur interrupt (0x10 for video display functions)
	cmp ax, 0x004F			; 0x004F means success
	jne VBEERROR
	
	; if changed to Graphic Mode, jump to Protected Kernel
	jmp JUMPTOPROTECTEDMODE
	

VBEERROR:
	; ===============================================
	; Exception Handling
	; ===============================================
	
	; print message that changing to Graphic Mode failed
	push CHANGEGRAPHICMODEFAIL
	push 2
	push 0
	call PRINTMESSAGE
	add  sp, 6
	
	jmp $
	
	
JUMPTOPROTECTEDMODE:
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Execute virtual OS Image							
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	jmp 0x1000:0x0000



; --------------------   Function code Area	  --------------------									

HANDLEDISKERROR:
	
	; Disk error handling function
	
	push DISKERRORMESSAGE
	push 1
	push 20
	call PRINTMESSAGE

	jmp $



PRINTMESSAGE:
	
	; Message print
	; Parameter : x coord, y coord, string
	
	push bp
	mov  bp, sp

	push es
	push si
	push di
	push ax
	push cx
	push dx

	mov ax, 0xB800
	mov es, ax


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Calculate Video Memory addr using X,Y coord	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	; get line address using Y coord
	
	mov ax, word [bp + 6]
	mov si, 160
	mul si
	mov di, ax

	; get final address using X coord after multiplying 2
	
	mov ax, word [bp + 4]
	mov si, 2
	mul si
	add di, ax

	; address of string
	
	mov si, word [bp + 8]



.MESSAGELOOP:
	mov cl, byte [si]

	cmp cl,0
	je .MESSAGEEND
	
	mov byte [es:di], cl

	add si, 1
	add di, 2

	jmp .MESSAGELOOP



.MESSAGEEND:
	pop dx
	pop cx
	pop ax
	pop di
	pop si
	pop es
	pop bp
	ret



; --------------------   Data Area	  --------------------									


; Boot Loader start message

MESSAGE1: db 0	
DISKERRORMESSAGE:		db 'Disk error', 0
IMAGELOADINGMESSAGE:	db 0
LOADINGCOMPLETEMESSAGE:	db 0
CHANGEGRAPHICMODEFAIL:  db 0

;MESSAGE1: db 'MINT64 OS Boot Loader Start..', 0	
;DISKERRORMESSAGE:		db 'Disk error occured', 0
;IMAGELOADINGMESSAGE:	db 'OS Image Loading..', 0
;LOADINGCOMPLETEMESSAGE:	db 'Complete', 			 0
;CHANGEGRAPHICMODEFAIL:  db 'Change Graphic Mode failed', 0


; for Disk Read

SECTORNUMBER:	db 0x02		; sector number that OS image starts from
HEADNUMBER:		db 0x00		; head number that OS image starts from 
TRACKNUMBER:	db 0x00		; track number that OS image starts from

; for Disk Parameter

BOOTDRIVE:		db 0x00		; boot drive number		
LASTSECTOR:	 	db 0x00		; last sector number - 1 of drive
LASTHEAD:		db 0x00		; last head number of drive
LASTTRACK:		db 0x00		; last track number of drive


times 510 - ($ - $$)	db 0x00

db 0x55
db 0xAA

