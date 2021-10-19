/*
	C Entry Point of Protected Mode Kernel
	
	1. Print message that C Kernel is started
	2. Memory size check
	3. IA-32e Kernel Area Initialization with 0x00 (0x100000 (1MB) ~ 0x600000 (6MB))
	4. Create Page Table for IA-32e Mode
	5. Read Process Manufacturer information
	6. Check if the processor supports 64-bit or not
	7. move IA-32e Mode Kernel to 0x200000 (2MB)
	8. Switch to IA-32e Mode
*/


#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"


// Function declaration
void kPrintString(int iX, int iY, const char* pcString);
BOOL kInitializeKernel64Area(void);
BOOL kIsMemoryEnough(void);
void kCopyKernel64ImageTo2MByte(void);

// Bootstrap Processor Flag Address Macro, located at front part of Boot Loader
#define BOOTSTRAPPROCESSOR_FLAGADDRESS		0x7C09



// ********************************************
// Main Function
// ********************************************

void Main(void)
{
	DWORD i;
	DWORD dwEAX, dwEBX, dwECX, dwEDX;
	char vcVendorString[13] = {0, };
	

	// if Application Processor, switch to 64 bit mode
	if(*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0)
	{
		kSwitchAndExecute64bitKernel();

		while(1);
	}


	// --------------- Below code executed by Bootstrap Processor only ---------------


	kPrintString(0, 3, "Protected Mode C Language Kernel Start......[Pass]");
	
	// check if memory size is enough
	kPrintString(0, 4, "Mininum Memory Size Check...................[    ]");
	
	if(kIsMemoryEnough() == FALSE)
	{
		kPrintString(45, 4, "Fail");
		kPrintString(0, 5, "Not Enough Memory. MINT64 OS Requires Over 64 Byte Memory");

		while(1);
	}
	else
		kPrintString(45, 4, "Pass");

	
	// initialize kernel area of IA-32 mode
	kInitializeKernel64Area();
	kPrintString(0, 5, "IA-32e Kernel Area Initialize...............[    ]");

	if(kInitializeKernel64Area() == FALSE)
	{
		kPrintString(45, 5, "Fail");
		kPrintString(0, 6, "Kernel Area Initialization Failed");
		while(1);
	}	
	
	kPrintString(45, 5, "Pass");

	
	// Create Page Table for IA-32e mode Kernel
	kPrintString(0, 6, "IA-32e Page Tables Initialize...............[    ]");
	kInitializePageTables();
	kPrintString(45, 6, "Pass");


	// read Process Manufacturer Information
	kReadCPUID(0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	
	*(DWORD*)vcVendorString = dwEBX;
	*((DWORD*)vcVendorString + 1) = dwEDX;
	*((DWORD*)vcVendorString + 2) = dwECX;

	kPrintString(0, 7, "Process Vendor String.......................[            ]");
	kPrintString(45, 7, vcVendorString);


	// check if process supports 64-bit
	kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	
	kPrintString(0, 8, "64-bit Mode Support Check...................[    ]");

	if(dwEDX & (1 << 29))
		kPrintString(45, 8, "Pass");
	else
	{
		kPrintString(45, 8, "Fail");
		kPrintString(0, 9, "This processor does not support 64-bit mode");
		while(1);
	}


	// move IA-32e Mode Kernel to 0x200000 (2MB)
	kPrintString(0, 9, "Copy IA-32e Kernel to 2MB Address...........[    ]");
	kCopyKernel64ImageTo2MByte();
	kPrintString(45, 9, "Pass");


	// Switch to IA-32e Mode
	kPrintString(0, 10, "Switch to IA-32e Mode");
	kSwitchAndExecute64bitKernel();


	while(1);
}

//*********************************************


// Print String Function

void kPrintString(int iX, int iY, const char* pcString)
{
	CHARACTER* pstScreen = (CHARACTER*)0xB8000;
	int i;

	pstScreen += (iY * 80) + iX;	// one line contains 80 words
	
	for(i = 0; pcString[i] != 0; i++)
	{
		pstScreen[i].bCharactor = pcString[i];
	}
}


// initialize kernel area of IA-32e mode

BOOL kInitializeKernel64Area(void)
{
	DWORD* pdwCurrentAddress;


	// initialize from address 0x100000 (1MB) - 64 bit Kernel Started from here
	pdwCurrentAddress = (DWORD*)0x100000;


	// to address 0x600000 (6MB) - filled with 0 by 4 Byte
	while((DWORD)pdwCurrentAddress < 0x600000)
	{
		*pdwCurrentAddress = 0x00;
		

		// if *pdwCurrentAddress is not 0 after filled with 0, 
		// there's a problem using that address
		if(*pdwCurrentAddress != 0)
			return FALSE;

		pdwCurrentAddress++;
	}

	return TRUE;
}


// check if memory is enough to execute MINT64 OS

BOOL kIsMemoryEnough(void)
{
	DWORD* pdwCurrentAddress;

	// Start checking from 0x100000 (1MB)
	pdwCurrentAddress = (DWORD*)0x100000;

	// ... to 0x4000000 (64MB)
	while((DWORD)pdwCurrentAddress < 0x4000000)
	{
		*pdwCurrentAddress = 0x12345678;

		if(*pdwCurrentAddress != 0x12345678)
			return FALSE;

		// move 1MB ???
		pdwCurrentAddress += (0x100000 / 4);
	}

	return TRUE;
}


// copy IA-32e Mode Kernel to 0x200000 (2MB)

void kCopyKernel64ImageTo2MByte(void)
{
	WORD wKernel32SectorCount, wTotalKernelSectorCount;
	DWORD* pdwSourceAddress, *pdwDestinationAddress;
	int i;

	
	// TOTALSECTORCOUNT at 0x7C05
	// KERNEL32SECTORCOUNT at 0x7C07
	wTotalKernelSectorCount = *((WORD*)0x7C05);
	wKernel32SectorCount	= *((WORD*)0x7C07);

	pdwSourceAddress 		= (DWORD*)(0x10000 + (wKernel32SectorCount * 512));	
							// Protected Mode Kernel location + KERNEL32SECTORCOUNT * 512  
	pdwDestinationAddress	= (DWORD*)0x200000;
	

	for(i=0; i<512 * (wTotalKernelSectorCount - wKernel32SectorCount) / 4; i++)		// 512 * num of Kernel64 sector
	{
		*pdwDestinationAddress = *pdwSourceAddress;
		pdwDestinationAddress++;
		pdwSourceAddress++;
	}
}


