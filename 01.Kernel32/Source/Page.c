#include "Page.h"
#include "../../02.Kernel64/Source/Task.h"


// get Dynamic Memory Start Address (align by Page size - 2MB)
#define DYNAMICMEMORY_START_ADDRESS		( (TASK_STACKPOOLADDRESS + 0x1FFFFF) & 0xFFE00000 )


// Create Page Table for IA-32e mode Kernel
// initialize with 1 PML4T, 1 PDPT, 64 PDs - 64 GB available
void kInitializePageTables()
{
	PML4TENTRY* pstPML4TEntry;
	PDPTENTRY*  pstPDPTEntry;
	PDENTRY*	pstPDEntry;
	
	DWORD dwMappingAddress;
	DWORD dwPageEntryFlags;
	int i;

	
	// Create Page Map Level 4 Table
	// 		all entry initialized by 0 except first one
	pstPML4TEntry = (PML4TENTRY*)0x100000;

	// set as User level
	kSetPageEntryData(&(pstPML4TEntry[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);

	for(i=1; i<PAGE_MAXENTRYCOUNT; i++)
	{
		kSetPageEntryData(&(pstPML4TEntry[i]), 0, 0, 0, 0);
	}


	// Create Page Directory Pointer Table
	// 		one PDPT can map 512 GB max
	// 		set 64 entries for 64 GB mapping
	pstPDPTEntry = (PDPTENTRY*)0x101000;

	for(i=0; i<64; i++)
	{
		// set as User level
		kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0x102000 + (i*PAGE_TABLESIZE), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
	}

	for(i=64; i<PAGE_MAXENTRYCOUNT; i++)
	{
		kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0, 0, 0);
	}
	

	// Create Page Directory Table
	// 		one PDT can map 1 GB max
	// 		create 64 page directories to support 64 GB
	pstPDEntry = (PDENTRY*)0x102000;
	dwMappingAddress = 0;

	for(i=0; i<PAGE_MAXENTRYCOUNT*64; i++)
	{
		// set Page entry attribute by Section
		// Kernel area is the area below Dynamic Memory area, User area is above of it
		
		if(i < ((DWORD)DYNAMICMEMORY_START_ADDRESS / PAGE_DEFAULTSIZE))
		{
			dwPageEntryFlags = PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS;
		}
		else
		{
			dwPageEntryFlags = PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS | PAGE_FLAGS_US;
		}


		// 32 bit cannot represent upper address (0xFFFFFFFF max)
		// 		calculate Address over 32-bit by dividing into MB and then dividing result into 4KB
		kSetPageEntryData(&(pstPDEntry[i]), (i * (PAGE_DEFAULTSIZE >> 20)) >> 12, dwMappingAddress, dwPageEntryFlags, 0);


		dwMappingAddress += PAGE_DEFAULTSIZE;
	}
}


// set Base Address, Attribute Flags to Page Entry

void kSetPageEntryData(PTENTRY* pstEntry, DWORD dwUpperBaseAddress,
					   DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags)
{
	pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
	pstEntry->dwUpperBaseAddressAndEXB		 = (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}



