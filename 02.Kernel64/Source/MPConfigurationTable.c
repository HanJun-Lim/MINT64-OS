#include "MPConfigurationTable.h"
#include "Console.h"


static MPCONFIGURATIONMANAGER gs_stMPConfigurationManager = {0, };


// Find MP Floating Pointer Header in BIOS Area and return address of it 
BOOL kFindMPFloatingPointerAddress(QWORD* pstAddress)
{
	char* pcMPFloatingPointer;
	QWORD qwEBDAAddress;
	QWORD qwSystemBaseMemory;


	// print Extended BIOS Data Area, System Base Memory
	//
	// 		- EBDA Start Address can be found at 0x040E (WORD size, Segment Address stored)
	// 		  you have to multiply 16 to convert Segment Address to Physical Address
	//
	// 		- System Base Memory can be found at 0x0413 (WORD size, stored as KB size)
	// 		  you have to multiply 1024 to get real size (byte size)
	//kPrintf("Extended BIOS Data Area = [0x%X] \n", (DWORD)(*(WORD*)0x040E) * 16);
	//kPrintf("System Base Address = [0x%X] \n", (DWORD)(*(WORD*)0x0413) * 1024);


	/*
	   IMPORTANT!

	   MP Floating Pointer stored in at least one of the following memory locations..
	 		1. the first Kilobyte of Extended BIOS Data Area
	 		2. within the last kilobyte of System Base Memory
			3. in the BIOS ROM address space between 0x0F0000 and 0x0FFFFF
	*/
	

	// --------------------------------------------------------------------------------------------
	// 1. find MP Floating Pointer by searching EBDA Area
	// 		- EBDA Start Address can be found at 0x040E (WORD size, Segment Address stored)
	qwEBDAAddress = *(WORD*)(0x040E);
	qwEBDAAddress *= 16;		// convert to Physical Address


	// search first KB of Extended BIOS Data Area
	for(pcMPFloatingPointer = (char*)qwEBDAAddress; 
		(QWORD)pcMPFloatingPointer <= (qwEBDAAddress + 1024); 
		pcMPFloatingPointer++)
	{
		if(kMemCmp(pcMPFloatingPointer, "_MP_", 4) == 0)
		{
			//kPrintf("MP Floating Pointer is in EBDA, [0x%X] Address\n", (QWORD)pcMPFloatingPointer);
			*pstAddress = (QWORD)pcMPFloatingPointer;

			return TRUE;
		}
	}


	// --------------------------------------------------------------------------------------------
	// 2. find MP Floating Pointer by searching System Base Memory
	//		- System Base Memory can be found at 0x0413 (WORD size, stored as KB size)
	qwSystemBaseMemory = *(WORD*)0x0413;
	qwSystemBaseMemory *= 1024;		// convert to Byte unit
	
	
	for(pcMPFloatingPointer = (char*)(qwSystemBaseMemory - 1024);
		(QWORD)pcMPFloatingPointer <= qwSystemBaseMemory;
		pcMPFloatingPointer++)
	{
		if(kMemCmp(pcMPFloatingPointer, "_MP_", 4) == 0)
		{
			//kPrintf("MP Floating Pointer is in System Base Memory, [0x%X] Address\n", (QWORD)pcMPFloatingPointer);
			*pstAddress = (QWORD)pcMPFloatingPointer;

			return TRUE;
		}
	}

	
	// --------------------------------------------------------------------------------------------
	// 3. find MP Floating Pointer by searching BIOS ROM Area (0x0F0000 ~ 0x0FFFFF)
	
	for(pcMPFloatingPointer = (char*)0x0F0000;
		(QWORD)pcMPFloatingPointer < 0x0FFFFF;
		pcMPFloatingPointer++)
	{
		if(kMemCmp(pcMPFloatingPointer, "_MP_", 4) == 0)
		{
			//kPrintf("MP Floating Pointer is in ROM, [0x%X] Address\n", pcMPFloatingPointer);
			*pstAddress = (QWORD)pcMPFloatingPointer;

			return TRUE;
		}
	}


	return FALSE;
}


// Analysis MP Configuration Table and store requirements
BOOL kAnalysisMPConfigurationTable(void)
{
	QWORD qwMPFloatingPointerAddress;
	MPFLOATINGPOINTER* pstMPFloatingPointer;
	MPCONFIGURATIONTABLEHEADER* pstMPConfigurationHeader;
	BYTE  bEntryType;
	WORD  i;
	QWORD qwEntryAddress;
	PROCESSORENTRY* pstProcessorEntry;
	BUSENTRY* pstBusEntry;
	
	
	// init Data Structure
	kMemSet(&gs_stMPConfigurationManager, 0, sizeof(MPCONFIGURATIONMANAGER));
	gs_stMPConfigurationManager.bISABusID = 0xFF; 


	// get MP Floating Pointer Address
	if(kFindMPFloatingPointerAddress(&qwMPFloatingPointerAddress) == FALSE)
	{
		return FALSE;
	}


	// set MP Floating Pointer
	pstMPFloatingPointer = (MPFLOATINGPOINTER*)qwMPFloatingPointerAddress;
	gs_stMPConfigurationManager.pstMPFloatingPointer = pstMPFloatingPointer;

	pstMPConfigurationHeader = 
		(MPCONFIGURATIONTABLEHEADER*)((QWORD)pstMPFloatingPointer->dwMPConfigurationTableAddress & 0xFFFFFFFF);
	

	// PIC mode supported?
	if(pstMPFloatingPointer->vbMPFeatureByte[1] & MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE)
	{
		gs_stMPConfigurationManager.bUsePICMode = TRUE;
	}


	// set MP Configuration Table Header, MP Configuration Table Entry
	gs_stMPConfigurationManager.pstMPConfigurationTableHeader = pstMPConfigurationHeader;
	gs_stMPConfigurationManager.qwBaseEntryStartAddress = 
		pstMPFloatingPointer->dwMPConfigurationTableAddress + sizeof(MPCONFIGURATIONTABLEHEADER);


	// calculate Core count, store ISA Bus ID by looping all entries
	qwEntryAddress = gs_stMPConfigurationManager.qwBaseEntryStartAddress;

	for(i=0; i<pstMPConfigurationHeader->wEntryCount; i++)
	{
		bEntryType = *(BYTE*)qwEntryAddress;		// get value in Entry Type Field
		
		switch(bEntryType)
		{
			// if Processor Entry, increase Processor Count
			case MP_ENTRYTYPE_PROCESSOR:
				pstProcessorEntry = (PROCESSORENTRY*)qwEntryAddress;

				if(pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_ENABLE)
				{
					gs_stMPConfigurationManager.iProcessorCount++;
				}

				qwEntryAddress += sizeof(PROCESSORENTRY);

				break;

			// if Bus Entry, Check if ISA Bus Type and save
			case MP_ENTRYTYPE_BUS:
				pstBusEntry = (BUSENTRY*)qwEntryAddress;

				if(kMemCmp(pstBusEntry->vcBusTypeString, MP_BUS_TYPESTRING_ISA, kStrLen(MP_BUS_TYPESTRING_ISA)) == 0)
				{
					gs_stMPConfigurationManager.bISABusID = pstBusEntry->bBusID;
				}

				qwEntryAddress += sizeof(BUSENTRY);

				break;

			// ignore other entry types
			case MP_ENTRYTYPE_IOAPIC:
			case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
			case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
			default:
				qwEntryAddress += 8;
				break;
		}	// bEntryType switch end
	}	// Entry count loop end


	return TRUE;
}


// Get MP Configuration Manager Data Structure
MPCONFIGURATIONMANAGER* kGetMPConfigurationManager(void)
{
	return &gs_stMPConfigurationManager;
}


// Print information of MP Configuration Table
void kPrintMPConfigurationTable(void)
{
	MPCONFIGURATIONMANAGER* 		pstMPConfigurationManager;
	QWORD 							qwMPFloatingPointerAddress;
	MPFLOATINGPOINTER* 				pstMPFloatingPointer;
	MPCONFIGURATIONTABLEHEADER* 	pstMPTableHeader;
	PROCESSORENTRY* 				pstProcessorEntry;
	BUSENTRY* 						pstBusEntry;
	IOAPICENTRY*					pstIOAPICEntry;
	IOINTERRUPTASSIGNMENTENTRY* 	pstIOAssignmentEntry;
	LOCALINTERRUPTASSIGNMENTENTRY*  pstLocalAssignmentEntry;
	QWORD							qwBaseEntryAddress;
	char							vcStringBuffer[20];
	WORD							i;
	BYTE							bEntryType;

	
	// string to print
	char* vpcInterruptType[4]	 = {"INT", "NMI", "SMI", "ExtINT"};
	char* vpcInterruptFlagsPO[4] = {"Conform", "Active High", "Reserved", "Active Low"};
	char* vpcInterruptFlagsEL[4] = {"Conform", "Edge-Trigger", "Reserved", "Level-Trigger"};


	// ------------------------------------------------------------------------------------------
	// Store information used for system handling by calling kAnalysisMPConfigurationTable()
	// ------------------------------------------------------------------------------------------
	
	kPrintf("=============== MP Configuration Table Summary ===============\n");

	pstMPConfigurationManager = kGetMPConfigurationManager();

	if( (pstMPConfigurationManager->qwBaseEntryStartAddress == 0) && 
		(kAnalysisMPConfigurationTable() == FALSE) )
	{
		kPrintf("MP Configuration Table Analysis Fail\n");
		return;
	}

	kPrintf("MP Configuration Table Analysis Success\n");

	kPrintf("MP Floating Pointer Address : 0x%Q\n", pstMPConfigurationManager->pstMPFloatingPointer);
	kPrintf("PIC Mode Support : %d\n", pstMPConfigurationManager->bUsePICMode);
	kPrintf("MP Configuration Table Header Address : 0x%Q\n", pstMPConfigurationManager->pstMPConfigurationTableHeader);
	kPrintf("Base MP Configuration Table Entry Start Address : 0x%Q\n", pstMPConfigurationManager->qwBaseEntryStartAddress);
	kPrintf("Processor Count : %d\n", pstMPConfigurationManager->iProcessorCount);
	kPrintf("ISA Bus ID : %d\n", pstMPConfigurationManager->bISABusID);

	kPrintf("Press any key to continue...('q' to exit) : ");
	if(kGetCh() == 'q')
	{
		kPrintf("\n");
		return;
	}


	// ------------------------------------------------------------------------------------------
	// Print MP Floating Pointer Information
	// ------------------------------------------------------------------------------------------
	
	kPrintf("\n=============== MP Floating Pointer ===============\n");

	pstMPFloatingPointer = pstMPConfigurationManager->pstMPFloatingPointer;

	kMemCpy(vcStringBuffer, pstMPFloatingPointer->vcSignature, 4);
	vcStringBuffer[4] = '\0';
	kPrintf("Signature : %s\n", vcStringBuffer);
	kPrintf("MP Configuration Header Address : 0x%Q\n", pstMPFloatingPointer->dwMPConfigurationTableAddress);
	kPrintf("Length : %d * 16 Byte\n", pstMPFloatingPointer->bLength);
	kPrintf("Version : %d\n", pstMPFloatingPointer->bRevision);
	kPrintf("CheckSum : 0x%X\n", pstMPFloatingPointer->bCheckSum);

	// check if MP Configuration Table used or not
	kPrintf("Feature Byte 1 : 0x%X", pstMPFloatingPointer->vbMPFeatureByte[0]);

	if(pstMPFloatingPointer->vbMPFeatureByte[0] == MP_FLOATINGPOINTER_FEATUREBYTE1_USEMPTABLE)
	{
		kPrintf("(Use MP Configuration Table)\n");
	}
	else
	{
		kPrintf("(Use Default Configuration)\n");
	}
	
	// check if PIC mode supported or not
	kPrintf("Feature Byte 2 : 0x%X", pstMPFloatingPointer->vbMPFeatureByte[1]);

	if(pstMPFloatingPointer->vbMPFeatureByte[1] & MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE)
	{
		kPrintf("(PIC Mode Support)\n");
	}
	else
	{
		kPrintf("(Virtual Wire Mode Support)\n");
	}

	
	// ------------------------------------------------------------------------------------------
	// Print MP Configuration Table Header Information
	// ------------------------------------------------------------------------------------------
	
	kPrintf("\n=============== MP Configuration Table Header ===============\n");
	
	pstMPTableHeader = pstMPConfigurationManager->pstMPConfigurationTableHeader;

	kMemCpy(vcStringBuffer, pstMPTableHeader->vcSignature, 4);
	vcStringBuffer[4] = '\0';
	kPrintf("Signature : %s\n", vcStringBuffer);
	kPrintf("Length : %d Byte\n", pstMPTableHeader->wBaseTableLength);
	kPrintf("Version : %d\n", pstMPTableHeader->bRevision);
	kPrintf("CheckSum : 0x%X\n", pstMPTableHeader->bCheckSum);
	kMemCpy(vcStringBuffer, pstMPTableHeader->vcOEMIDString, 8);
	vcStringBuffer[8] = '\0';
	kPrintf("OEM ID String : %s\n", vcStringBuffer);
	kMemCpy(vcStringBuffer, pstMPTableHeader->vcProductIDString, 12);
	vcStringBuffer[12] = '\0';
	kPrintf("Product ID String : %s\n", vcStringBuffer);
	kPrintf("OEM Table Pointer : 0x%X\n", pstMPTableHeader->dwOEMTablePointerAddress);
	kPrintf("QEM Table Size : %d Byte\n", pstMPTableHeader->wOEMTableSize);
	kPrintf("Entry Count : %d\n", pstMPTableHeader->wEntryCount);
	kPrintf("Memory Mapped I/O Address Of Local APIC : 0x%X\n", pstMPTableHeader->dwMemoryMapIOAddressOfLocalAPIC);
	kPrintf("Extended Table Length : %d Byte\n", pstMPTableHeader->wExtendedTableLength);
	kPrintf("Extended Table Checksum : 0x%X\n", pstMPTableHeader->bExtendedTableChecksum);

	kPrintf("Press any key to continue...('q' is exit) : ");
	if(kGetCh() == 'q')
	{
		kPrintf("\n");
		return;
	}
	kPrintf("\n");


	// ------------------------------------------------------------------------------------------
	// Print Base MP Configuration Table Entry Information
	// ------------------------------------------------------------------------------------------
	
	kPrintf("\n=============== Base MP Configuration Table Entry ===============\n");
	
	qwBaseEntryAddress = pstMPFloatingPointer->dwMPConfigurationTableAddress + sizeof(MPCONFIGURATIONTABLEHEADER);

	for(i=0; i<pstMPTableHeader->wEntryCount; i++)
	{
		bEntryType = *(BYTE*)qwBaseEntryAddress;

		switch(bEntryType)
		{
			case MP_ENTRYTYPE_PROCESSOR:
				pstProcessorEntry = (PROCESSORENTRY*)qwBaseEntryAddress;

				kPrintf("Entry Type : Processor\n");
				kPrintf("Local APIC ID : %d\n", pstProcessorEntry->bLocalAPICID);
				kPrintf("Local APIC Version : %d\n", pstProcessorEntry->bLocalAPICVersion);
				kPrintf("CPU Flags : 0x%X ", pstProcessorEntry->bCPUFlags);
				// print Enable/Disable
				if(pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_ENABLE)
				{
					kPrintf("(Enable, ");
				}
				else
				{
					kPrintf("(Disable, ");
				}
				// print BSP/AP
				if(pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_BSP)
				{
					kPrintf("BSP)\n");
				}
				else
				{
					kPrintf("AP)\n");
				}
				kPrintf("CPU Signature : 0x%X\n", pstProcessorEntry->vbCPUSignature);
				kPrintf("Feature Flags : 0x%X\n\n", pstProcessorEntry->dwFeatureFlags);


				// move to next entry
				qwBaseEntryAddress += sizeof(PROCESSORENTRY);

				break;

			case MP_ENTRYTYPE_BUS:
				pstBusEntry = (BUSENTRY*)qwBaseEntryAddress;

				kPrintf("Entry Type : Bus\n");
				kPrintf("Bus ID : %d\n", pstBusEntry->bBusID);
				kMemCpy(vcStringBuffer, pstBusEntry->vcBusTypeString, 6);
				vcStringBuffer[6] = '\0';
				kPrintf("Bus Type String : %s\n\n", vcStringBuffer);

				// move to next entry
				qwBaseEntryAddress += sizeof(BUSENTRY);

				break;

			case MP_ENTRYTYPE_IOAPIC:
				pstIOAPICEntry = (IOAPICENTRY*)qwBaseEntryAddress;

				kPrintf("Entry Type : I/O APIC\n");
				kPrintf("I/O APIC ID : %d\n", pstIOAPICEntry->bIOAPICID);
				kPrintf("I/O APIC Version : 0x%X\n", pstIOAPICEntry->bIOAPICVersion);
				kPrintf("I/O APIC Flags : 0x%X ", pstIOAPICEntry->bIOAPICFlags);
				// print Enable/Disable
				if(pstIOAPICEntry->bIOAPICFlags == 1)
				{
					kPrintf("(Enable)\n");
				}
				else
				{
					kPrintf("(Disable)\n");
				}
				kPrintf("Memory Mapped I/O Address : 0x%X\n\n", pstIOAPICEntry->dwMemoryMapAddress);

				// move to next entry
				qwBaseEntryAddress += sizeof(IOAPICENTRY);

				break;

			case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
				pstIOAssignmentEntry = (IOINTERRUPTASSIGNMENTENTRY*)qwBaseEntryAddress;
				
				kPrintf("Entry Type : I/O Interrupt Assignment\n");
				kPrintf("Interrupt Type : 0x%X ", pstIOAssignmentEntry->bInterruptType);
				// print Interrupt Type
				kPrintf("(%s)\n", vpcInterruptType[pstIOAssignmentEntry->bInterruptType]);
				kPrintf("I/O Interrupt Flags : 0x%X ", pstIOAssignmentEntry->wInterruptFlags);
				// print Polarity/Trigger mode
				kPrintf("(%s, %s)\n", 
						vpcInterruptFlagsPO[pstIOAssignmentEntry->wInterruptFlags & 0x03],
						vpcInterruptFlagsEL[(pstIOAssignmentEntry->wInterruptFlags >> 2) & 0x03]);
				kPrintf("Source Bus ID : %d\n", pstIOAssignmentEntry->bSourceBusID);
				kPrintf("Source Bus IRQ : %d\n", pstIOAssignmentEntry->bSourceBusIRQ);
				kPrintf("Destination I/O APIC ID : %d\n", pstIOAssignmentEntry->bDestinationIOAPICID);
				kPrintf("Destination I/O APIC INTIN : %d\n\n", pstIOAssignmentEntry->bDestinationIOAPICINTIN);

				// move to next entry
				qwBaseEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);

				break;

			case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
				pstLocalAssignmentEntry = (LOCALINTERRUPTASSIGNMENTENTRY*)qwBaseEntryAddress;

				kPrintf("Entry Type : Local Interrupt Assignment\n");
				kPrintf("Interrupt Type : 0x%X ", pstLocalAssignmentEntry->bInterruptType);
				// print Interrupt Type
				kPrintf("(%s)\n", vpcInterruptType[pstLocalAssignmentEntry->bInterruptType]);
				kPrintf("I/O Interrupt Flags : 0x%X ", pstLocalAssignmentEntry->wInterruptFlags);
				// print Polarity/Trigger mode
				kPrintf("(%s, %s)\n",
						vpcInterruptFlagsPO[pstLocalAssignmentEntry->wInterruptFlags & 0x03],
						vpcInterruptFlagsEL[(pstLocalAssignmentEntry->wInterruptFlags >> 2) & 0x03]);
				kPrintf("Source Bus ID : %d\n", pstLocalAssignmentEntry->bSourceBusID);
				kPrintf("Source Bus IRQ : %d\n", pstLocalAssignmentEntry->bSourceBusIRQ);
				kPrintf("Destination Local APIC ID : %d\n", pstLocalAssignmentEntry->bDestinationLocalAPICID);
				kPrintf("Destination Local APIC INTIN : %d\n\n", pstLocalAssignmentEntry->bDestinationLocalAPICINTIN);

				// move to next entry
				qwBaseEntryAddress += sizeof(LOCALINTERRUPTASSIGNMENTENTRY);

				break;

			default:
				kPrintf("Unknown Entry Type, %d\n", bEntryType);

				break;
		}

		if( (i != 0) && (((i+1) % 3) == 0) )
		{
			kPrintf("Press any key to continue...('q' is exit) : ");

			if(kGetCh() == 'q')
			{
				kPrintf("\n");
				break;
			}

			kPrintf("\n");
		}
	}	// Entry Count loop end

}


// Return the number of Core or Processor
int kGetProcessorCount(void)
{
	// MP Configuration Table may be able not to exist
	// 		so, if iProcessorCount is 0, return 1
	if(gs_stMPConfigurationManager.iProcessorCount == 0)
	{
		return 1;
	}

	return gs_stMPConfigurationManager.iProcessorCount;
}



// Search for I/O APIC Entry connected with ISA Bus
// 		WARNING! kAnalysisMPConfigurationTable() must be called first
IOAPICENTRY* kFindIOAPICEntryForISA(void)
{
	MPCONFIGURATIONMANAGER* pstMPManager;
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;
	IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
	IOAPICENTRY* pstIOAPICEntry;
	QWORD qwEntryAddress;
	BYTE  bEntryType;
	BOOL  bFind = FALSE;
	int   i;


	// store Start Address of MP Configuration Table Header, Start Address of Entry
	pstMPHeader = gs_stMPConfigurationManager.pstMPConfigurationTableHeader;
	qwEntryAddress = gs_stMPConfigurationManager.qwBaseEntryStartAddress;

	
	// ====================================================================
	// 1. Search for I/O Interrupt Assignment Entry related to ISA Bus
	// ====================================================================
	
	for(i=0; (i<pstMPHeader->wEntryCount) && (bFind == FALSE); i++)
	{
		bEntryType = *(BYTE*)qwEntryAddress;

		switch(bEntryType)
		{
			// ignore other Entry
			case MP_ENTRYTYPE_PROCESSOR:
				qwEntryAddress += sizeof(PROCESSORENTRY);

				break;
			
			case MP_ENTRYTYPE_BUS:
			case MP_ENTRYTYPE_IOAPIC:
			case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
				qwEntryAddress += 8;
				
				break;
			
			// if I/O Interrupt Assignment type, check if it related to ISA Bus
			case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
				pstIOAssignmentEntry = (IOINTERRUPTASSIGNMENTENTRY*)qwEntryAddress;

				// compare with ISA Bus ID stored in MP Configuration Manager
				if(pstIOAssignmentEntry->bSourceBusID == gs_stMPConfigurationManager.bISABusID)
				{
					bFind = TRUE;
				}

				qwEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);

				break;
		}
	}	// search all Entry loop end


	if(bFind == FALSE)
	{
		return NULL;
	}


	// ====================================================================
	// 2. Search for I/O APIC related to ISA Bus 
	// ====================================================================
	
	qwEntryAddress = gs_stMPConfigurationManager.qwBaseEntryStartAddress;

	for(i=0; i<pstMPHeader->wEntryCount; i++)
	{
		bEntryType = *(BYTE*)qwEntryAddress;

		switch(bEntryType)
		{
			// ignore other Entry
			case MP_ENTRYTYPE_PROCESSOR:
				qwEntryAddress += sizeof(PROCESSORENTRY);

				break;
			
			case MP_ENTRYTYPE_BUS:
			case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
			case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
				qwEntryAddress += 8;

				break;

			// if I/O APIC Entry, check if I/O APIC connected with ISA Bus
			case MP_ENTRYTYPE_IOAPIC:
				pstIOAPICEntry = (IOAPICENTRY*)qwEntryAddress;

				if(pstIOAPICEntry->bIOAPICID == pstIOAssignmentEntry->bDestinationIOAPICID)
				{
					return pstIOAPICEntry;
				}

				qwEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);

				break;
		}
	}	// search all Entry loop end


	return NULL;
}



