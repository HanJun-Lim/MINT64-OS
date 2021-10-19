#include "Loader.h"
#include "FileSystem.h"
#include "DynamicMemory.h"


/*
 * 		How to execute Application Program
 * 
 * 		1. analyze ELF64 File Header
 * 		2. analyze Section Header Table
 * 		3. calculate Memory size to load Application Program
 * 		4. allocate Memory refer to the result of Procedure 3
 * 		5. change the Start Address of section based on allocated address
 * 		6. copy the sections in pbFileBuffer to Application Memory
 * 		7. relocate the sections in Application Memory
 * 		8. create Task, free temporary Memory
 * 		9. execute Application Program
 *
 */


QWORD kExecuteProgram(const char* pcFileName, const char* pcArgumentString, BYTE bAffinity)
{
	DIR* pstDirectory;
	struct dirent* pstEntry;
	DWORD dwFileSize;
	BYTE* pbTempFileBuffer;
	FILE* pstFile;
	DWORD dwReadSize;
	QWORD qwEntryPointAddress;
	QWORD qwApplicationMemory;
	QWORD qwMemorySize;
	TCB* pstTask;


	// -------------------------------------------------
	// open root dir, retrieve target file
	// -------------------------------------------------
	
	pstDirectory = opendir("/");
	dwFileSize = 0;


	// retreive target file
	while(1)
	{
		// get one directory entry
		pstEntry = readdir(pstDirectory);

		if(pstEntry == NULL)
		{
			break;
		}

		// retrieve target file by checking File Name, File Name Length
		if( (kStrLen(pstEntry->d_name) == kStrLen(pcFileName)) && 
			(kMemCmp(pstEntry->d_name, pcFileName, kStrLen(pcFileName)) == 0) )
		{
			dwFileSize = pstEntry->dwFileSize;
			break;
		}
	}


	// close Directory (retrieving end)
	closedir(pstDirectory);


	if(dwFileSize == 0)
	{
		kPrintf("%s file doesn't exist or size is zero\n", pcFileName);

		return TASK_INVALIDID;
	}


	// -------------------------------------------------
	// allocate temporary buffer to store whole file
	// -------------------------------------------------
	
	pbTempFileBuffer = (BYTE*)kAllocateMemory(dwFileSize);

	if(pbTempFileBuffer == NULL)
	{
		kPrintf("Memory %d bytes allocate fail\n");

		return TASK_INVALIDID;
	}


	// read whole file, and store it to allocated memory
	pstFile = fopen(pcFileName, "r");

	if( (pstFile != NULL) && (fread(pbTempFileBuffer, 1, dwFileSize, pstFile) == dwFileSize) )
	{
		fclose(pstFile);
		kPrintf("%s file read success\n", pcFileName);
	}
	else
	{
		kPrintf("%s file read fail\n", pcFileName);
		kFreeMemory(pbTempFileBuffer);
		fclose(pstFile);

		return TASK_INVALIDID;
	}


	// --------------------------------------------------------------
	// analyze ELF file, load Section and relocate (Procedure 1-5)
	// --------------------------------------------------------------
	
	if(kLoadProgramAndRelocation(pbTempFileBuffer, &qwApplicationMemory, &qwMemorySize, &qwEntryPointAddress) == FALSE)
	{
		kPrintf("%s file is invalid application file or loading fail\n", pcFileName);
		kFreeMemory(pbTempFileBuffer);

		return TASK_INVALIDID;
	}


	// free temporary buffer
	kFreeMemory(pbTempFileBuffer);


	// -----------------------------------------------------------------
	// create User-level Task, store Argument String to Program Stack
	// -----------------------------------------------------------------
	
	// create User-level Task
	pstTask = kCreateTask(TASK_FLAGS_USERLEVEL | TASK_FLAGS_PROCESS, 
						  (void*)qwApplicationMemory, qwMemorySize, qwEntryPointAddress, bAffinity);

	if(pstTask == NULL)
	{
		kFreeMemory((void*)qwApplicationMemory);

		return TASK_INVALIDID;
	}


	// store Argument String
	kAddArgumentStringToTask(pstTask, pcArgumentString);


	return pstTask->stLink.qwID;
}


// load Application Program Section, and relocate
static BOOL kLoadProgramAndRelocation(BYTE*  pbFileBuffer			 , QWORD* pqwApplicationMemoryAddress, 
									  QWORD* pqwApplicationMemorySize, QWORD* pqwEntryPointAddress)
{
	Elf64_Ehdr* pstELFHeader;
	Elf64_Shdr* pstSectionHeader;
	Elf64_Shdr* pstSectionNameTableHeader;
	Elf64_Xword qwLastSectionSize;
	Elf64_Addr	qwLastSectionAddress;
	int   i;
	QWORD qwMemorySize;
	QWORD qwStackAddress;
	BYTE* pbLoadedAddress;


	// -------------------------------------------------
	// print ELF Header info, store information for analysis
	// -------------------------------------------------
	
	pstELFHeader = (Elf64_Ehdr*)pbFileBuffer;
	pstSectionHeader = (Elf64_Shdr*)(pbFileBuffer + pstELFHeader->e_shoff);
	pstSectionNameTableHeader = pstSectionHeader + pstELFHeader->e_shstrndx;

	kPrintf("=============== ELF Header Info ===============\n");
	kPrintf("Magic Number [%c%c%c] Section Header Count [%d]\n", 
			pstELFHeader->e_ident[1], pstELFHeader->e_ident[2], pstELFHeader->e_ident[3], pstELFHeader->e_shnum);
	kPrintf("File Type [%d]\n", pstELFHeader->e_type);
	kPrintf("Section Header Offset [0x%X] Size [0x%X]\n", pstELFHeader->e_shoff, pstELFHeader->e_shentsize);
	kPrintf("Program Header Offset [0x%X] Size [0x%X]\n", pstELFHeader->e_phoff, pstELFHeader->e_phentsize);
	kPrintf("Section Name String Table Section Index [%d]\n", pstELFHeader->e_shstrndx);


	// check ELF ID, Class, Encoding, Type (in ELF Identification) to judge the correct Application Program
	if( (pstELFHeader->e_ident[EI_MAG0]  != ELFMAG0    ) ||		// 0x7F
		(pstELFHeader->e_ident[EI_MAG1]  != ELFMAG1    ) ||		// 'E'
		(pstELFHeader->e_ident[EI_MAG2]  != ELFMAG2    ) ||		// 'L'
		(pstELFHeader->e_ident[EI_MAG3]  != ELFMAG3	   ) ||		// 'F'
		(pstELFHeader->e_ident[EI_CLASS] != ELFCLASS64 ) ||		// 64-bit objects
		(pstELFHeader->e_ident[EI_DATA]  != ELFDATA2LSB) ||		// object file data structures are little-endian
		(pstELFHeader->e_type 			 != ET_REL	   ) )		// Relocatable object file
	{
		return FALSE;
	}


	// -------------------------------------------------
	// find the latest section by checking memory address to load of all Section Header
	// also print Section information
	// -------------------------------------------------
	
	qwLastSectionAddress = 0;
	qwLastSectionSize = 0;

	for(i=0; i<pstELFHeader->e_shnum; i++)
	{
		if( (pstSectionHeader[i].sh_flags & SHF_ALLOC) &&				// section is allocated in memory image of program
			(pstSectionHeader[i].sh_addr >= qwLastSectionAddress) )		// get the latest section address
		{
			qwLastSectionAddress = pstSectionHeader[i].sh_addr;
			qwLastSectionSize 	 = pstSectionHeader[i].sh_size;
		}
	}

	kPrintf("\n=============== Load & Relocation ===============\n");
	kPrintf("Last Section Address [0x%Q] Size [0x%Q]\n", qwLastSectionAddress, qwLastSectionSize);

	
	// calculate Max Memory size by Last Section address, align by 4Kbyte
	// 		why need to align by 4Kbyte? because of cluster size?
	qwMemorySize = (qwLastSectionAddress + qwLastSectionSize + 0x1000 - 1) & 0xFFFFFFFFFFFF000;
	kPrintf("Aligned Memory Size [0x%Q]\n", qwMemorySize);


	// allocate memory for Application Program
	pbLoadedAddress = (char*)kAllocateMemory(qwMemorySize);

	if(pbLoadedAddress == NULL)
	{
		kPrintf("Memory allocate fail\n");
		return FALSE;
	}
	else
	{
		kPrintf("Loaded Address [0x%Q]\n", pbLoadedAddress);
	}

	
	// -------------------------------------------------
	// copy file contents to memory (loading)
	// -------------------------------------------------
	
	// NOTE : the first section (section 0) is NULL
	for(i=1; i<pstELFHeader->e_shnum; i++)
	{
		// if the section has no SHF_ALLOC flag or the size 0, no need to copy
		if( !(pstSectionHeader[i].sh_flags & SHF_ALLOC) || (pstSectionHeader[i].sh_size == 0) )
		{
			continue;
		}

		
		// apply loaded address to Section Header
		pstSectionHeader[i].sh_addr += (Elf64_Addr)pbLoadedAddress;


		// the section with SHT_NOBITS type (like .bss section) initialized with 0 (no data in section)
		if(pstSectionHeader[i].sh_type == SHT_NOBITS)
		{
			kMemSet(pstSectionHeader[i].sh_addr, 0, pstSectionHeader[i].sh_size);
		}
		else
		{
			kMemCpy(pstSectionHeader[i].sh_addr, pbFileBuffer + pstSectionHeader[i].sh_offset, pstSectionHeader[i].sh_size);
		}

		kPrintf("Section [%X] Virtual Address [%Q] File Address [%Q] Size [%Q]\n",
				i, pstSectionHeader[i].sh_addr, pbFileBuffer + pstSectionHeader[i].sh_offset, pstSectionHeader[i].sh_size);
	}


	kPrintf("Program load success\n");
	

	// -------------------------------------------------
	// relocation
	// -------------------------------------------------
	
	if(kRelocation(pbFileBuffer) == FALSE)
	{
		kPrintf("Relocation fail\n");

		return FALSE;
	}
	else
	{
		kPrintf("Relocation success\n");
	}


	// return Application Program address, Entry Point address
	*pqwApplicationMemoryAddress = (QWORD)pbLoadedAddress;
	*pqwApplicationMemorySize	 = qwMemorySize;
	*pqwEntryPointAddress		 = pstELFHeader->e_entry + (QWORD)pbLoadedAddress;


	return TRUE;
}


static BOOL kRelocation(BYTE* pbFileBuffer)
{
	Elf64_Ehdr* pstELFHeader;
	Elf64_Shdr* pstSectionHeader;
	int i;
	int j;
	int iSymbolTableIndex;
	int iSectionIndexInSymbol;
	int iSectionIndexToRelocation;
	Elf64_Addr   ulOffset;
	Elf64_Xword  ulInfo;
	Elf64_Sxword lAddend;
	Elf64_Sxword lResult;
	int iNumberOfBytes;
	Elf64_Rel*  pstRel;
	Elf64_Rela* pstRela;
	Elf64_Sym*  pstSymbolTable;

	
	// get ELF header, first Section Header
	pstELFHeader = (Elf64_Ehdr*)pbFileBuffer;
	pstSectionHeader = (Elf64_Shdr*)(pbFileBuffer + pstELFHeader->e_shoff);

	
	// -----------------------------------------------------------------
	// find Section Header with SHT_REL / SHT_RELA to relocate them
	// -----------------------------------------------------------------
	
	// NOTICE : The first symbol table entry is reserved and must be all zeroes.
	// 			The symbolic constant STN_UNDEF is used to refer to this entry
	for(i=1; i<pstELFHeader->e_shnum; i++)
	{
		if( (pstSectionHeader[i].sh_type != SHT_RELA) && (pstSectionHeader[i].sh_type != SHT_REL) )
		{
			continue;
		}


		// Index of Section Header to relocate stored at sh_info
		iSectionIndexToRelocation = pstSectionHeader[i].sh_info;

		// Index of referred Symbol Table Section Header stored at sh_link
		iSymbolTableIndex = pstSectionHeader[i].sh_link;

		// get first entry of Symbol Table section
		pstSymbolTable = (Elf64_Sym*)(pbFileBuffer + pstSectionHeader[iSymbolTableIndex].sh_offset);


		// -------------------------------------------------
		// relocate by searching Relocation Section entry
		// -------------------------------------------------
		
		for(j=0; j<pstSectionHeader[i].sh_size; )
		{
			// SHT_REL Type
			if(pstSectionHeader[i].sh_type == SHT_REL)
			{
				pstRel 	 = (Elf64_Rel*)(pbFileBuffer + pstSectionHeader[i].sh_offset + j);
				ulOffset = pstRel->r_offset;
				ulInfo 	 = pstRel->r_info;
				lAddend  = 0;		// SHT_REL type has no Addend value


				j += sizeof(Elf64_Rel);
			}

			// SHT_RELA Type
			else
			{
				pstRela = (Elf64_Rela*)(pbFileBuffer + pstSectionHeader[i].sh_offset + j);
				ulOffset = pstRela->r_offset;
				ulInfo = pstRela->r_info;
				lAddend = pstRela->r_addend;


				j += sizeof(Elf64_Rela);
			}
			

			// SHT_REL, SHT_RELA type refers Symbol Table, upper 32 bit of r_info field indicates referred Symbol entry Index
			// 		if Absolute address type, No relocation needed
			if(pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx == SHN_ABS)
			{
				continue;
			}


			// Common type Symbol not supported
			else if(pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx == SHN_COMMON)
			{
				kPrintf("Common symbol is not supported\n");

				return FALSE;
			}



			// ----------------------------------------------------------
			// calculate Relocation value by referring Relocation type
			// ----------------------------------------------------------

			// lower 32 bit of r_info field indicates relocation type
			switch(RELOCATION_LOWER32(ulInfo))
			{
				// S(st_value) + A(r_addend) type
				case R_X86_64_64:
				case R_X86_64_32:
				case R_X86_64_32S:
				case R_X86_64_16:
				case R_X86_64_8:

					// index of Section Header containing Symbol
					iSectionIndexInSymbol = pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx;

					lResult = (pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_value +
							   pstSectionHeader[iSectionIndexInSymbol].sh_addr) + 
							   lAddend;
					break;

				// S(st_value) + A(r_addend) - P(r_offset) type	
				case R_X86_64_PC32:
				case R_X86_64_PC16:
				case R_X86_64_PC8:
				case R_X86_64_PC64:
					
					// index of Section Header containing Symbol
					iSectionIndexInSymbol = pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx;

					lResult = (pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_value +
							   pstSectionHeader[iSectionIndexInSymbol].sh_addr) +
							   lAddend -
							   (ulOffset + pstSectionHeader[iSectionIndexToRelocation].sh_addr);
					break;

				// B(sh_addr) + A(r_addend) type	
				case R_X86_64_RELATIVE:
					
					lResult = pstSectionHeader[i].sh_addr +
							  lAddend;
					break;

				// Z(st_size) + A(r_addend) type
				case R_X86_64_SIZE32:
				case R_X86_64_SIZE64:
					
					lResult = pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_size +
							  lAddend;
					break;

				// Other cases are not supported
				default:
					kPrintf("Unsupported relocation type [%X]\n", RELOCATION_LOWER32(ulInfo));

					return FALSE;
					break;
			}



			// -------------------------------------------------
			// calculate range to apply by Relocation type
			// -------------------------------------------------

			switch(RELOCATION_LOWER32(ulInfo))
			{
				// 64 bit
				case R_X86_64_64:
				case R_X86_64_PC64:
				case R_X86_64_SIZE64:
					
					iNumberOfBytes = 8;
					
					break;

				// 32 bit
				case R_X86_64_PC32:
				case R_X86_64_32:
				case R_X86_64_32S:
				case R_X86_64_SIZE32:

					iNumberOfBytes = 4;

					break;

				// 16 bit
				case R_X86_64_16:
				case R_X86_64_PC16:

					iNumberOfBytes = 2;

					break;
				
				// 8 bit
				case R_X86_64_8:
				case R_X86_64_PC8:

					iNumberOfBytes = 1;

					break;

				// Other type prints error and exit
				default:
					
					kPrintf("Unsupported relocation type [%X]\n", RELOCATION_LOWER32(ulInfo));

					return FALSE;
					break;
			}



			// ---------------------------------------------------------------
			// apply calculated result to target Section by range to apply
			// ---------------------------------------------------------------

			// loaded address plused by kLoadProgramAndRelocation(), sh_addr indicates loaded section address
			switch(iNumberOfBytes)
			{
				case 8:

					*((Elf64_Sxword*)
					  (pstSectionHeader[iSectionIndexToRelocation].sh_addr + ulOffset)) += lResult;

					break;

				case 4:
					
					*((int*)
					  (pstSectionHeader[iSectionIndexToRelocation].sh_addr + ulOffset)) += (int)lResult;

					break;

				case 2:

					*((short*)
					  (pstSectionHeader[iSectionIndexToRelocation].sh_addr + ulOffset)) += (short)lResult;

					break;

				case 1:

					*((char*)
					  (pstSectionHeader[iSectionIndexToRelocation].sh_addr + ulOffset)) += (char)lResult;

					break;
				
				// Other size not supported
				default:

					kPrintf("Relocation error. Relocation byte size is [%d]byte\n", iNumberOfBytes);

					return FALSE;
					break;
			}

		} // relocation table entry loop end 

	} // SHT_REL, SHT_RELA type section header search loop end


	return TRUE;
}


// store Argument String to the Task
static void kAddArgumentStringToTask(TCB* pstTask, const char* pcArgumentString)
{
	int iLength;
	int iAlignedLength;
	QWORD qwNewRSPAddress;


	// get Argument String length
	if(pcArgumentString == NULL)
	{
		iLength = 0;
	}
	else
	{
		iLength = kStrLen(pcArgumentString);

		// max string size : 1 KB
		if(iLength > 1023)
		{
			iLength = 1023;
		}
	}


	// align Argument String length by 8 byte
	iAlignedLength = (iLength + 7) & 0xFFFFFFF8;


	// calculate new RSP Register value, copy Argument list to Stack (make a room for Argument String)
	qwNewRSPAddress = pstTask->stContext.vqRegister[TASK_RSPOFFSET] - (QWORD)iAlignedLength;
	
	kMemCpy((void*)qwNewRSPAddress, pcArgumentString, iLength);
	*((BYTE*)qwNewRSPAddress + iLength) = '\0';


	// update RSP, RBP Register
	pstTask->stContext.vqRegister[TASK_RSPOFFSET] = qwNewRSPAddress;
	pstTask->stContext.vqRegister[TASK_RBPOFFSET] = qwNewRSPAddress;


	// set RDI Register (Parameter 1)
	pstTask->stContext.vqRegister[TASK_RDIOFFSET] = qwNewRSPAddress;
}


// create Thread running on Application Program
QWORD kCreateThread(QWORD qwEntryPoint, QWORD qwArgument, BYTE bAffinity, QWORD qwExitFunction)
{
	TCB* pstTask;


	// create User-level Application Program Task
	pstTask = kCreateTask(TASK_FLAGS_USERLEVEL | TASK_FLAGS_THREAD, NULL, 0, qwEntryPoint, bAffinity);

	if(pstTask == NULL)
	{
		return TASK_INVALIDID;
	}

	
	// replace kEndTask() function address to qwExitFunction
	// 		RSP value indicates the function called when exit
	*((QWORD*)pstTask->stContext.vqRegister[TASK_RSPOFFSET]) = qwExitFunction;


	// insert Argument into RDI Register (Parameter 1)
	pstTask->stContext.vqRegister[TASK_RDIOFFSET] = qwArgument;


	return pstTask->stLink.qwID;
}



