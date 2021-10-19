#ifndef __PAGE_H__
#define __PAGE_H__

#include "Types.h"


// Macro

#define PAGE_FLAGS_P			0x00000001			// Present
#define PAGE_FLAGS_RW			0x00000002			// Read/Write (Read-only when clear)
#define PAGE_FLAGS_US			0x00000004			// User/Supervisor (User level appiled when set)
#define PAGE_FLAGS_PWT			0x00000008			// Page-Level Write-Through (Write-Through appiled when set)
#define PAGE_FLAGS_PCD			0x00000010			// Page-Level Cache Disable (means TLB?)
#define PAGE_FLAGS_A			0x00000020			// Accessed
#define PAGE_FLAGS_D			0x00000040			// Dirty
#define PAGE_FLAGS_PS			0x00000080			// Page Size
#define PAGE_FLAGS_G			0x00000100			// Global
#define PAGE_FLAGS_PAT			0x00001000			// Page Attribute Table Index
#define PAGE_FLAGS_EXB			0x80000000			// Execute-Disable

#define PAGE_FLAGS_DEFAULT		(PAGE_FLAGS_P | PAGE_FLAGS_RW)

#define PAGE_TABLESIZE			0x1000				// use 4KB size Page Table
#define PAGE_MAXENTRYCOUNT		512
#define PAGE_DEFAULTSIZE		0x200000			// use 2MB size Page


// structure

#pragma pack(push, 1)


// data structure of page entry

typedef struct kPageTableEntryStruct
{
	// Entry Structure (lower 32 bit)
	//
	// ------- Page Map Level 4 Table, Page Directory Pointer Table -------
	//
	// bit   | 31-12         11-9	8-6		  5  4    3    2    1    0
	// field | Base Address  Avail  Reserved  A  PCD  PWT  U/S  R/W  P
	//
	//
	// ------- Page Directory Entry (2MB Page) -------
	//
	// bit   | 31-21		 20-13				12	 11-9   8  7			  6  5  4    3    2    1    0
	// field | Base Address  Reserved (with 0)  PAT  Avail  G  PS (always 1)  D  A  PCD  PWT  U/S  R/W  P
	//
	DWORD dwAttributeAndLowerBaseAddress;

	// Entry Structure (upper 32 bit)
	// 
	// bit   | 63   62-52  51-40			  39-32
	// field | EXB  Avail  Reserved (with 0)  Base Address
	//
	DWORD dwUpperBaseAddressAndEXB;

} PML4TENTRY, PDPTENTRY, PDENTRY, PTENTRY;

#pragma pack(pop)


// function

void kInitializePageTables(void);
void kSetPageEntryData(PTENTRY* pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, 
					   DWORD dwLowerFlags, DWORD dwUpperFlags);


#endif	/* __PAGE_H__ */
