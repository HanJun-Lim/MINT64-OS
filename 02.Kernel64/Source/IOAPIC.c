#include "IOAPIC.h"
#include "MPConfigurationTable.h"
#include "PIC.h"


static IOAPICMANAGER gs_stIOAPICManager;


// Return I/O APIC Base Address of ISA Bus
QWORD kGetIOAPICBaseAddressOfISA(void)
{
	MPCONFIGURATIONMANAGER* pstMPManager;
	IOAPICENTRY* pstIOAPICEntry;


	// if No I/O APIC Address in Manager, search Entry and store
	if(gs_stIOAPICManager.qwIOAPICBaseAddressOfISA == NULL)
	{
		pstIOAPICEntry = kFindIOAPICEntryForISA();

		if(pstIOAPICEntry != NULL)
		{
			gs_stIOAPICManager.qwIOAPICBaseAddressOfISA = pstIOAPICEntry->dwMemoryMapAddress & 0xFFFFFFFF;
		}
	}


	return gs_stIOAPICManager.qwIOAPICBaseAddressOfISA;
}


// Set value to I/O Redirection Table
void kSetIOAPICRedirectionEntry(IOREDIRECTIONTABLE* pstEntry, BYTE bAPICID,
		BYTE bInterruptMask, BYTE bFlagsAndDeliveryMode, BYTE bVector)
{
	kMemSet(pstEntry, 0, sizeof(IOREDIRECTIONTABLE));

	pstEntry->bDestination 			= bAPICID;
	pstEntry->bFlagsAndDeliveryMode = bFlagsAndDeliveryMode;
	pstEntry->bInterruptMask 		= bInterruptMask;
	pstEntry->bVector				= bVector;
}


// Read I/O Redirection Table [iINTIN]
void kReadIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE* pstEntry)
{
	QWORD* pqwData;
	QWORD  qwIOAPICBaseAddress;


	qwIOAPICBaseAddress = kGetIOAPICBaseAddressOfISA();


	// I/O Redirection Table is 8 bytes (64 bits)
	pqwData = (QWORD*)pstEntry;

	
	// ====================================================================
	// read upper 4 bytes of I/O Redirection Table
	// 		- I/O Redirection Table is made by couple of DWORD
	// 		  so, double INTIN to calculate I/O Redirection Table Index
	// ====================================================================
	
	// send Index of upper I/O Redirection Table Register to I/O Register Selector (0xFEC00000)
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) = 
		IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN * 2;

	// read upper I/O Redirection Table Register from I/O Window Register
	*pqwData = *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW);
	*pqwData = *pqwData << 32;


	// ====================================================================
	// read lower 4 bytes of I/O Redirection Table
	// 		- I/O Redirection Table is made by couple of DWORD
	// 		  so, double INTIN to calculate I/O Redirection Table Index
	// ====================================================================
	
	// send Index of lower I/O Redirection Table Register to I/O Register Selector (0xFEC00000)
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) =
		IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + iINTIN * 2;

	// read lower I/O Redirection Table Register from I/O Window Register
	*pqwData |= *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW);
}


// Write I/O Redirection Table [iINTIN]
void kWriteIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE* pstEntry)
{
	QWORD* pqwData;
	QWORD  qwIOAPICBaseAddress;

	
	qwIOAPICBaseAddress = kGetIOAPICBaseAddressOfISA();


	// I/O Redirection Table is 8 bytes (64 bits)
	pqwData = (QWORD*)pstEntry;


	// ====================================================================
	// write upper 4 bytes of I/O Redirection Table
	// 		- I/O Redirection Table is made by couple of DWORD
	// 		  so, double INTIN to calculate I/O Redirection Table Index
	// ====================================================================
	
	// select I/O Redirection Table (High) [iINTIN]
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) =
		IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN * 2;

	// write upper I/O Redirection Table Register
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW) = *pqwData >> 32;

	// ====================================================================
	// write lower 4 bytes of I/O Redirection Table
	// 		- I/O Redirection Table is made by couple of DWORD
	// 		  so, double INTIN to calculate I/O Redirection Table Index
	// ====================================================================
	
	// select I/O Redirection Table (Low) [iINTIN]
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) = 
		IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + iINTIN * 2;

	// write lower I/O Redirection Table Register
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW) = *pqwData;
}


// Mask all interrupt pin connected with I/O APIC for no interrupt reception
void kMaskAllInterruptInIOAPIC(void)
{
	IOREDIRECTIONTABLE stEntry;
	int i;


	// Disable all Interrupt
	for(i=0; i<IOAPIC_MAXIOREDIRECTIONTABLECOUNT; i++)
	{
		kReadIOAPICRedirectionTable(i, &stEntry);
		
		stEntry.bInterruptMask = IOAPIC_INTERRUPT_MASK;

		kWriteIOAPICRedirectionTable(i, &stEntry);
	}
}


void kInitializeIORedirectionTable(void)
{
	MPCONFIGURATIONMANAGER* pstMPManager;
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;
	IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
	IOREDIRECTIONTABLE stIORedirectionEntry;
	QWORD qwEntryAddress;
	BYTE  bEntryType;
	BYTE  bDestination;
	int   i;

	
	// =========================================================
	// initialize I/O APIC Manager
	// =========================================================
	
	kMemSet(&gs_stIOAPICManager, 0, sizeof(gs_stIOAPICManager));


	// store I/O APIC Memory Map I/O Address
	kGetIOAPICBaseAddressOfISA();

	
	// initialize IRQ->INTIN Mapping Table
	for(i=0; i<IOAPIC_MAXIRQTOINTINMAPCOUNT; i++)
	{
		gs_stIOAPICManager.vbIRQToINTINMap[i] = 0xFF;
	}


	// =========================================================
	// mask I/O APIC for no interrupt occurence,
	// initialize I/O Redirection Table
	// =========================================================
	
	// mask I/O APIC first
	kMaskAllInterruptInIOAPIC();

	
	// select I/O Interrupt Assignment Entry related to ISA Bus, 
	// and set it to I/O Redirection Table.
	// store Start Address of MP Configuration Table Header, Start Address of Entry
	pstMPManager   = kGetMPConfigurationManager();

	pstMPHeader    = pstMPManager->pstMPConfigurationTableHeader;
	qwEntryAddress = pstMPManager->qwBaseEntryStartAddress;


	// search for I/O Interrupt Assignment Entry related to ISA Bus
	for(i=0; i<pstMPHeader->wEntryCount; i++)
	{
		bEntryType = *(BYTE*)qwEntryAddress;

		switch(bEntryType)
		{
			case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
				pstIOAssignmentEntry = (IOINTERRUPTASSIGNMENTENTRY*)qwEntryAddress;

				// handle only "INT" type
				if( (pstIOAssignmentEntry->bSourceBusID == pstMPManager->bISABusID) &&
					(pstIOAssignmentEntry->bInterruptType == MP_INTERRUPTTYPE_INT) )
				{
					// set all Destination field to 0x00 for delivering interrupt to only Bootstrap Processor
					// IRQ 0 used to schedule, so, set 0xFF for delivering interrupt to all Processor
					if(pstIOAssignmentEntry->bSourceBusIRQ == 0)
					{
						bDestination = 0xFF;
					}
					else
					{
						bDestination = 0x00;
					}


					// ISA Bus uses Edge-Trigger, Active High
					// Destination Mode is Physical Mode, Delivery Mode is Fixed
					// set Interrupt Vector to 0x20 + IRQ as same as PIC Controller Vector
					kSetIOAPICRedirectionEntry(&stIORedirectionEntry, bDestination, 
							0x00, IOAPIC_TRIGGERMODE_EDGE | IOAPIC_POLARITY_ACTIVEHIGH | 
							IOAPIC_DESTINATIONMODE_PHYSICALMODE | IOAPIC_DELIVERYMODE_FIXED,
							PIC_IRQSTARTVECTOR + pstIOAssignmentEntry->bSourceBusIRQ);


					// IRQ which received from ISA Bus located at I/O APIC INTIN Pin
					// so, handle it using INTIN value
					kWriteIOAPICRedirectionTable(pstIOAssignmentEntry->bDestinationIOAPICINTIN, &stIORedirectionEntry);


					// configure IRQ->INTIN Mapping Table
					gs_stIOAPICManager.vbIRQToINTINMap[pstIOAssignmentEntry->bSourceBusIRQ] =
						pstIOAssignmentEntry->bDestinationIOAPICINTIN;
				}

				qwEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);
				break;

			// ignore other Entry
			case MP_ENTRYTYPE_PROCESSOR:
				qwEntryAddress += sizeof(PROCESSORENTRY);

				break;

			case MP_ENTRYTYPE_BUS:
			case MP_ENTRYTYPE_IOAPIC:
			case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
				qwEntryAddress += 8;			// 8 bytes

				break;
		}
	}	// searching all Entry loop end

}


// Print the relationship between IRQ and I/O APIC Interrupt Pin
void kPrintIRQToINTINMap(void)
{
	int i;

	kPrintf("========== IRQ To I/O APIC INTIN Mapping Table ==========\n");

	for(i=0; i<IOAPIC_MAXIRQTOINTINMAPCOUNT; i++)
	{
		kPrintf("IRQ [%d] -> INTIN [%d]\n", i, gs_stIOAPICManager.vbIRQToINTINMap[i]);
	}
}



// Make IRQ be sent to Local APIC
void kRoutingIRQToAPICID(int iIRQ, BYTE bAPICID)
{
	int i;
	IOREDIRECTIONTABLE stEntry;


	// iIRQ valid?
	if(iIRQ > IOAPIC_MAXIRQTOINTINMAPCOUNT)
	{
		return;
	}
	

	// modify Destination field in I/O Redirection Table
	kReadIOAPICRedirectionTable(gs_stIOAPICManager.vbIRQToINTINMap[iIRQ], &stEntry);
	stEntry.bDestination = bAPICID;
	kWriteIOAPICRedirectionTable(gs_stIOAPICManager.vbIRQToINTINMap[iIRQ], &stEntry);
}



