#ifndef __MPCONFIGURATIONTABLE__
#define __MPCONFIGURATIONTABLE__

#include "Types.h"


// =====================================
// Macro
// =====================================

// MP Floating Pointer Feature Byte

#define MP_FLOATINGPOINTER_FEATUREBYTE1_USEMPTABLE	0x00
#define MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE		0x80

// Entry Type

#define MP_ENTRYTYPE_PROCESSOR						0
#define MP_ENTRYTYPE_BUS							1
#define MP_ENTRYTYPE_IOAPIC							2
#define MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT			3
#define MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT		4

// Processor CPU Flag

#define MP_PROCESSOR_CPUFLAGS_ENABLE				0x01
#define MP_PROCESSOR_CPUFLAGS_BSP					0x02

// Bus Type String

#define MP_BUS_TYPESTRING_ISA						"ISA"
#define MP_BUS_TYPESTRING_PCI						"PCI"
#define MP_BUS_TYPESTRING_PCMCIA					"PCMCIA"
#define MP_BUS_TYPESTRING_VESALOCALBUS				"VL"

// Interrupt Type

#define MP_INTERRUPTTYPE_INT						0	
#define MP_INTERRUPTTYPE_NMI						1
#define MP_INTERRUPTTYPE_SMI						2
#define MP_INTERRUPTTYPE_EXTINT						3

// Interrupt Flags

#define MP_INTERRUPT_FLAGS_CONFORMPOLARITY			0x00
#define MP_INTERRUPT_FLAGS_ACTIVEHIGH				0x01
#define MP_INTERRUPT_FLAGS_ACTIVELOW				0x03
#define MP_INTERRUPT_FLAGS_CONFORMTRIGGER			0x00
#define MP_INTERRUPT_FLAGS_EDGETRIGGERED			0x04
#define MP_INTERRUPT_FLAGS_LEVELTRIGGERED			0x0C



// =====================================
// Stucture
// =====================================

// align by 1 byte
#pragma pack(push, 1)


// MP Floating Pointer Data Structure
typedef struct kMPFloatingPointerStruct
{
	// The signature, _MP_
	char vcSignature[4];
	
	// The address which MP Configuration Table locates
	DWORD dwMPConfigurationTableAddress;

	// The length of MP Floating Pointer Data Structure, 16 bytes
	BYTE bLength;

	// MultiProcessor Specification Version
	BYTE bRevision;

	// Checksum
	BYTE bCheckSum;

	// MP Feature Bytes 1~5
	BYTE vbMPFeatureByte[5];

} MPFLOATINGPOINTER;


// MP Configuration Table Header
typedef struct kMPConfigurationTableHeaderStruct
{
	// The signature, PCMP
	char vcSignature[4];

	// Base Table length
	WORD wBaseTableLength;

	// MultiProcessor Specification Version
	BYTE bRevision;

	// Checksum
	BYTE bCheckSum;

	// OEM ID String
	char vcOEMIDString[8];

	// PRODUCT ID String
	char vcProductIDString[12];

	// The address of Table defined by OEM
	DWORD dwOEMTablePointerAddress;

	// The length of Table defined by OEM
	WORD wOEMTableSize;

	// The number of Basic MP Configuration Table Entry
	WORD wEntryCount;

	// Memory Mapped I/O Address of Local APIC
	DWORD dwMemoryMapIOAddressOfLocalAPIC;

	// The length of Extended Table
	WORD wExtendedTableLength;

	// The Checksum of Extended Table
	BYTE bExtendedTableChecksum;

	// Reserved
	BYTE bReserved;

} MPCONFIGURATIONTABLEHEADER;


// Processor Entry
typedef struct kProcessorEntryStruct
{
	// Entry Type, 0
	BYTE bEntryType;

	// Local APIC included in Processor
	BYTE bLocalAPICID;
	
	// Local APIC Version
	BYTE bLocalAPICVersion;

	// CPU Flags
	BYTE bCPUFlags;

	// CPU Signature
	BYTE vbCPUSignature[4];

	// Feature Flags
	DWORD dwFeatureFlags;

	// Reserved
	DWORD vdwReserved[2];
} PROCESSORENTRY;


// Bus Entry
typedef struct kBusEntryStruct
{
	// Entry Type, 1
	BYTE bEntryType;

	// Bus ID
	BYTE bBusID;

	// Bus Type String
	char vcBusTypeString[6];
} BUSENTRY;


// I/O APIC Entry
typedef struct kIOAPICEntryStruct
{
	// Entry Type, 2
	BYTE bEntryType;

	// I/O APIC ID
	BYTE bIOAPICID;

	// I/O APIC Version
	BYTE bIOAPICVersion;

	// I/O APIC Flags
	BYTE bIOAPICFlags;

	// Memory Mapped I/O Address
	DWORD dwMemoryMapAddress;
} IOAPICENTRY;


// I/O Interrupt Assignment Entry
typedef struct kIOInterruptAssignmentEntryStruct
{
	// Entry Type, 3
	BYTE bEntryType;

	// Interrupt Type
	BYTE bInterruptType;

	// I/O Interrupt Flags
	WORD wInterruptFlags;

	// Source Bus ID
	BYTE bSourceBusID;

	// Source Bus IRQ
	BYTE bSourceBusIRQ;

	// Destination I/O APIC ID
	BYTE bDestinationIOAPICID;

	// Destination I/O APIC INTIN
	BYTE bDestinationIOAPICINTIN;
} IOINTERRUPTASSIGNMENTENTRY;


// Local Interrupt Assignment Entry
typedef struct kLocalInterruptAssignmentEntryStruct
{
	// Entry Type, 4
	BYTE bEntryType;

	// Interrupt Type
	BYTE bInterruptType;

	// Local Interrupt Flags
	WORD wInterruptFlags;

	// Source Bus ID
	BYTE bSourceBusID;

	// Source Bus IRQ
	BYTE bSourceBusIRQ;

	// Destination Local APIC ID
	BYTE bDestinationLocalAPICID;

	// Destination Local APIC INTIN
	BYTE bDestinationLocalAPICINTIN;
} LOCALINTERRUPTASSIGNMENTENTRY;


#pragma pack(pop)


// MP Configuration Table Manager
typedef struct kMPConfigurationManagerStruct
{
	// MP Floating Table
	MPFLOATINGPOINTER* pstMPFloatingPointer;

	// MP Configuration Table Header
	MPCONFIGURATIONTABLEHEADER* pstMPConfigurationTableHeader;
	
	// The start address of Basic MP Configuration Table Entry
	QWORD qwBaseEntryStartAddress;
	
	// The number of Core of Processor
	int iProcessorCount;

	// PIC mode supported?
	BOOL bUsePICMode;
	
	// ISA Bus ID
	BYTE bISABusID;
} MPCONFIGURATIONMANAGER;



// =====================================
// Function
// =====================================

BOOL kFindMPFloatingPointerAddress(QWORD* pstAddress);
BOOL kAnalysisMPConfigurationTable(void);
MPCONFIGURATIONMANAGER* kGetMPConfigurationManager(void);
void kPrintMPConfigurationTable(void);
int  kGetProcessorCount(void);

IOAPICENTRY* kFindIOAPICEntryForISA(void);


#endif
