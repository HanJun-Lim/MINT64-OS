#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "Types.h"


//////////////////////////////////////////////////////
// Macro
//////////////////////////////////////////////////////

// ========================================
// GDT (Global Descriptor Table)
// ========================================


// used to combine

#define GDT_TYPE_CODE			0x0A
#define GDT_TYPE_DATA			0x02
#define GDT_TYPE_TSS			0x09	// Task Status Segment

#define GDT_FLAGS_LOWER_S		0x10	// System
#define	GDT_FLAGS_LOWER_DPL0	0x00	// Descriptor Privilege Level
#define	GDT_FLAGS_LOWER_DPL1	0x20
#define GDT_FLAGS_LOWER_DPL2	0x40
#define GDT_FLAGS_LOWER_DPL3	0x60
#define GDT_FLAGS_LOWER_P		0x80	// Present 

#define GDT_FLAGS_UPPER_L		0x20	// Long mode
#define	GDT_FLAGS_UPPER_DB		0x40	// Default Operation Bit
#define	GDT_FLAGS_UPPER_G		0x80	// Granularity

// --------------------------------------

// Lower Flags  -  Code/Data/TSS, DPL0/3, Present

#define GDT_FLAGS_LOWER_KERNELCODE	(GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNELDATA	(GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERCODE	(GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA	(GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS			(GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)

// Upper Flags  -  G (Granulaty), add 64-bit to Code/Data (L)

#define GDT_FLAGS_UPPER_CODE		(GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA		(GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS			(GDT_FLAGS_UPPER_G)

// Segment Descriptor Offset

#define GDT_KERNELCODESEGMENT	0x08
#define GDT_KERNELDATASEGMENT	0x10
#define GDT_USERDATASEGMENT		0x18
#define GDT_USERCODESEGMENT		0x20
#define GDT_TSSSEGMENT			0x28

// RPL (Request Privilege Level) for Segment Selector

#define SELECTOR_RPL_0			0x00
#define SELECTOR_RPL_3			0x03

// Other macros related to GDT

#define GDTR_STARTADDRESS		0x142000					// 1MB (0x100000) + 264KB - Page Table Area (0x042000)

#define GDT_MAXENTRY8COUNT		5							// NULL / Kernel Code / Kernel Data / User Data / User Code
#define GDT_MAXENTRY16COUNT		(MAXPROCESSORCOUNT)			// TSS created as many as Max Processor/Core Count

#define GDT_TABLESIZE		((sizeof(GDTENTRY8) * GDT_MAXENTRY8COUNT) + (sizeof(GDTENTRY16) * GDT_MAXENTRY16COUNT))
#define TSS_SEGMENTSIZE		(sizeof(TSSSEGMENT) * MAXPROCESSORCOUNT)



// ========================================
// IDT (Interrupt Descriptor Table)
// ========================================


// used to combine

#define IDT_TYPE_INTERRUPT		0x0E
#define IDT_TYPE_TRAP			0x0F
#define IDT_FLAGS_DPL0			0x00
#define IDT_FLAGS_DPL1			0x20
#define IDT_FLAGS_DPL2			0x40
#define IDT_FLAGS_DPL3			0x60
#define IDT_FLAGS_P				0x80
#define IDT_FLAGS_IST0			0
#define IDT_FLAGS_IST1			1

// --------------------------------------

#define IDT_FLAGS_KERNEL		(IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER			(IDT_FLAGS_DPL3 | IDT_FLGAS_P)

// Other macros related to IDT

#define IDT_MAXENTRYCOUNT		100

#define IDTR_STARTADDRESS		(GDTR_STARTADDRESS + sizeof(GDTR) + GDT_TABLESIZE + TSS_SEGMENTSIZE)
#define IDT_STARTADDRESS		(IDTR_STARTADDRESS + sizeof(IDTR))

#define IDT_TABLESIZE			(IDT_MAXENTRYCOUNT * sizeof(IDTENTRY))


#define IST_STARTADDRESS		0x700000

#define IST_SIZE				0x100000



//////////////////////////////////////////////////////
// Structures
//////////////////////////////////////////////////////

#pragma pack(push, 1)


// GDTR, IDTR Structure

typedef struct kGDTRStruct
{
	WORD	wLimit;
	QWORD	qwBaseAddress;
	
	// added to align 16-Byte address
	WORD	wPading;
	DWORD	dwPading;
} GDTR, IDTR;


// GDT Entry Struct  -  8 byte

typedef struct kGDTEntry8Struct
{
	WORD	wLowerLimit;
	WORD	wLowerBaseAddress;
	BYTE	bUpperBaseAddress1;
	BYTE	bTypeAndLowerFlag;			// Type (4 bit), S, DPL (2 bit), P
	BYTE	bUpperLimitAndUpperFlag;	// Segment Limit (4 bit), AVL, L, D/B, G
	BYTE	bUpperBaseAddress2;
} GDTENTRY8;


// GDT Entry Struct  -  16 byte

typedef struct kGDTEntry16Struct
{
	WORD	wLowerLimit;
	WORD	wLowerBaseAddress;
	BYTE	bMiddleBaseAddress1;
	BYTE	bTypeAndLowerFlag;			// Type (4 bit), 0, DPL (2 bit), P
	BYTE	bUpperLimitAndUpperFlag;	// Segment Limit (4 bit), AVL, 0, 0, G
	BYTE	bMiddleBaseAddress2;
	DWORD	dwUpperBaseAddress;
	DWORD	dwReserved;
} GDTENTRY16;


// TSS Data Struct

typedef struct kTSSDataStruct
{
	DWORD	dwReserved1;
	QWORD	qwRsp[3];
	QWORD	qwReserved2;
	QWORD	qwIST[7];
	QWORD	qwReserved3;
	WORD	wReserved;
	WORD	wIOMapBaseAddress;
} TSSSEGMENT;


// IDT Gate Descriptor Struct

typedef struct kIDTEntryStruct
{
	WORD	wLowerBaseAddress;
	WORD	wSegmentSelector;
	BYTE	bIST;					// IST (3 bit), 0 (5 bit)
	BYTE	bTypeAndFlags;			// Type (4 bit), 0, DPL (2 bit), P
	WORD	wMiddleBaseAddress;
	DWORD	dwUpperBaseAddress;
	DWORD	dwReserved;
} IDTENTRY;


#pragma pack (pop)



//////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////

void kInitializeGDTTableAndTSS(void);
void kSetGDTEntry8(GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kSetGDTEntry16(GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kInitializeTSSSegment(TSSSEGMENT* pstTSS);

void kInitializeIDTTables(void);
void kSetIDTEntry(IDTENTRY* pstEntry, void* pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType);
void kDummyHandler(void);


#endif
