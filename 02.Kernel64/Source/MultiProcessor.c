#include "MultiProcessor.h"
#include "MPConfigurationTable.h"
#include "AssemblyUtility.h"
#include "LocalAPIC.h"
#include "PIT.h"



// The number of acivated AP
volatile int g_iWakeUpApplicationProcessorCount = 0;

// The Address of APIC ID Register
volatile QWORD g_qwAPICIDAddress = 0;



// Activate Local APIC and AP
BOOL kStartUpApplicationProcessor(void)
{
	// analyze MP Configuration Table
	if(kAnalysisMPConfigurationTable() == FALSE)
	{
		return FALSE;
	}
	

	// activate all Local APIC in all Processor (in AssemblyUtility.h)
	kEnableGlobalLocalAPIC();


	// activate Local APIC of BootStrap Processor (in LocalAPIC.h)
	kEnableSoftwareLocalAPIC();
	

	// wake up AP
	if(kWakeUpApplicationProcessor() == FALSE)
	{
		return FALSE;
	}


	return TRUE;
}


// Activate AP (Application Processor)
static BOOL kWakeUpApplicationProcessor(void)
{
	MPCONFIGURATIONMANAGER* 	pstMPManager;
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;
	QWORD qwLocalAPICBaseAddress;
	BOOL  bInterruptFlag;
	int   i;


	///////////////////////////////
	// Disable Interrupt
	bInterruptFlag = kSetInterruptFlag(FALSE);
	///////////////////////////////
	

	// use Memory mapped I/O Address stored in MP Configuration Header
	pstMPManager = kGetMPConfigurationManager();

	pstMPHeader = pstMPManager->pstMPConfigurationTableHeader;
	qwLocalAPICBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;

	
	// store Address of APIC ID Register (0xFEE00020) to make AP read itself ID
	g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;


	// ===============================================================================
	// send INIT IPI, STARTUP IPI to Lower Interrupt Command Register (0xFEE00300)
	// to wake up AP
	// ===============================================================================
	
	// 1. Send INIT IPI to APs by using Lower Interrupt Command Register(0xFEE00300)
	// 		- AP(Application Processor) starts from 0x10000 (Protected Mode Kernel located)
	// 		- All Excluding Self, Edge Trigger, Assert, Physical Destination, INIT
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) = 
		APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE |
		APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_INIT;
	
	
	// wait for 10 ms (direct PIT control)
	kWaitUsingDirectPIT(MSTOCOUNT(10));


	// INIT success?
	if(*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING)
	{
		// set Timer Interrupt occurence as 1000 per 1 sec
		kInitializePIT(MSTOCOUNT(1), TRUE);

		// restore Interrupt Flag
		kSetInterruptFlag(bInterruptFlag);

		return FALSE;
	}
	

	// 2. Send STARTUP IPI to APs by using Lower Interrupt Command Register(0xFEE00300)
	// 		- AP(Application Processor) starts from 0x10000 (Protected Mode Kernel located)
	// 		  So, set Interrupt Vector as 0x10 (0x10000 / 4 KB)
	// 		- All Excluding Self, Edge Trigger, Assert, Physical Destination, STARTUP
	
	for(i=0; i<2; i++)
	{
		*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) =
			APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE |
			APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_STARTUP | 0x10;


		// wait for 200 us (direct PIT control)
		kWaitUsingDirectPIT(USTOCOUNT(200));


		// STARTUP sucess?
		if(*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING)
		{
			// set Timer Interrupt occurence as 1000 per 1 sec
			kInitializePIT(MSTOCOUNT(1), TRUE);

			// restore Interrupt Flag
			kSetInterruptFlag(bInterruptFlag);

			return FALSE;
		}
	}


	// set Timer Interrupt occurence as 1000 per 1 sec
	kInitializePIT(MSTOCOUNT(1), TRUE);


	// restore Interrupt Flag
	kSetInterruptFlag(bInterruptFlag);


	// wait for waking up all Application Processor
	while( g_iWakeUpApplicationProcessorCount < (pstMPManager->iProcessorCount - 1) )
	{
		kSleep(50);
	}

	
	return TRUE;
}


// return APIC ID from APIC ID Register
BYTE kGetAPICID(void)
{
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;
	QWORD qwLocalAPICBaseAddress;


	// if APIC ID Address value not set, read MP Configuration Table
	if(g_qwAPICIDAddress == 0)
	{
		// use Memory Mapped I/O Address of Local APIC stored in MP Configuration Table
		pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;

		if(pstMPHeader == NULL)
		{
			return 0;
		}


		// store APIC ID Register Address (0xFEE00020) to make AP read itself ID
		qwLocalAPICBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
		g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;
	}


	// return Bit 24-31 of APIC ID Register
	return *((DWORD*)g_qwAPICIDAddress) >> 24;
}



