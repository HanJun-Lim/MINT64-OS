[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x1000:START		; Copy 0x1000 to Segment register then go to START

SECTORCOUNT:		dw 0x0000		; executing sector number
TOTALSECTORCOUNT	equ	1024		; total sector number of Virtual OS
									; Max : 1152 sector (0x90000 byte)


; --------------- Code Area ---------------

START:
	mov ax, cs
	mov ds, ax			; ds - 0x10000 - OS image loaded
	mov ax, 0xB800
	mov es, ax			; es - 0xB8000 - print messages
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Create sector for each sector
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	%assign i	0	; var i = 0 (sector number)
	
	%rep TOTALSECTORCOUNT	; repeat code below while TOTALSECTORCOUNT
		%assign i	i + 1		; i += 1
		
		mov ax, 2		; one word = 2 byte
		mul word [SECTORCOUNT]
		mov si, ax		; si = 0, 2, 4, 8, 10 ...
		
		mov byte [ es:si + (160 * 2) ], '0' + (i % 10)			; 160 -> 80 (width) * 2 (column)
																; i = 0~9

		add word [SECTORCOUNT], 1		; SECTORCOUNT += 1
	
	
		; check if sector number is final or not
		%if i == TOTALSECTORCOUNT		; case of final sector (1024)
			jmp $
		%else							; case of sector 1~1023
			jmp (0x1000 + i * 0x20):0x0000		; move to next sector
		%endif
		
	
		times (512 - ($ - $$) % 512)	db 0x00

	%endrep
