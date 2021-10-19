#ifndef __IOAPIC_H__
#define __IOAPIC_H__

#include "Types.h"


// ----------------------------------------
// Macro
// ----------------------------------------

// I/O APIC Register Offset

#define IOAPIC_REGISTER_IOREGISTERSELECTOR				0x00
#define IOAPIC_REGISTER_IOWINDOW						0x10

// Register Index to access

#define IOAPIC_REGISTERINDEX_IOAPICID					0x00	
#define IOAPIC_REGISTERINDEX_IOAPICVERSTION				0x01
#define IOAPIC_REGISTERINDEX_IOAPICARBID				0x02
#define IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE  	0x10
#define IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE 	0x11

// Max Count of I/O Redirection Table

#define IOAPIC_MAXIOREDIRECTIONTABLECOUNT				24

// ***** Below Macros for I/O Redirection Table Register *****

// for Interrupt Mask

#define IOAPIC_INTERRUPT_MASK							0x01

// for Trigger Mode

#define IOAPIC_TRIGGERMODE_LEVEL						0x80
#define IOAPIC_TRIGGERMODE_EDGE							0x00

// for Remote IRR (Interrupt Request Register)

#define IOAPIC_REMOTEIRR_EOI							0x40
#define IOAPIC_REMOTEIRR_ACCEPT							0x00

// for Interrupt Input Pin Polarity

#define IOAPIC_POLARITY_ACTIVELOW						0x20
#define IOAPIC_POLARITY_ACTIVEHIGH						0x00

// for Delivery Status

#define IOAPIC_DELIVERYSTATUS_SENDPENDING				0x10
#define IOAPIC_DELIVERYSTATUS_IDLE						0x00

// for Destination Mode

#define IOAPIC_DESTINATIONMODE_LOGICALMODE				0x08
#define IOAPIC_DESTINATIONMODE_PHYSICALMODE				0x00

// for Delivery Mode

#define IOAPIC_DELIVERYMODE_FIXED						0x00
#define IOAPIC_DELIVERYMODE_LOWESTPRIORITY				0x01
#define IOAPIC_DELIVERYMODE_SMI							0x02
#define IOAPIC_DELIVERYMODE_NMI							0x04
#define IOAPIC_DELIVERYMODE_INIT						0x05
#define IOAPIC_DELIVERYMODE_EXTINT						0x07

// Max Size of IRQ to INTIN Mapping Table

#define IOAPIC_MAXIRQTOINTINMAPCOUNT					16



// ----------------------------------------
// Structure
// ----------------------------------------

#pragma pack(push, 1)

typedef struct kIORedirectionTableStruct
{
	// Interrupt Vector
	BYTE bVector;

	// Trigger Mode, Remote IRR, Interrupt Input Pin Polarity,
	// Delivery Status, Destination Mode, Delivery Mode
	BYTE bFlagsAndDeliveryMode;

	// Interrupt Mask
	BYTE bInterruptMask;

	// Reserved
	BYTE vbReserved[4];

	// Destination APIC ID to send Interrupt
	BYTE bDestination;

} IOREDIRECTIONTABLE;

#pragma pack(pop)


typedef struct kIOAPICManagerStruct
{
	// I/O APIC Memory Map Address which ISA Bus connected
	QWORD qwIOAPICBaseAddressOfISA;

	// The Table that saves Relationship between IRQ and INTIN
	BYTE vbIRQToINTINMap[IOAPIC_MAXIRQTOINTINMAPCOUNT];

} IOAPICMANAGER;



// ----------------------------------------
// Function
// ----------------------------------------

QWORD kGetIOAPICBaseAddressOfISA(void);
void  kSetIOAPICRedirectionEntry(IOREDIRECTIONTABLE* pstEntry, BYTE bAPICID, 
		BYTE bInterruptMask, BYTE bFlagsAndDeliveryMode, BYTE bVector);
void  kReadIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE* pstEntry);
void  kWriteIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE* pstEntry);
void  kMaskAllInterruptInIOAPIC(void);
void  kInitializeIORedirectionTable(void);
void  kPrintIRQToINTINMap(void);



#endif
