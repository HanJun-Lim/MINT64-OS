#ifndef __LOADER_H__
#define __LOADER_H__

#include "Types.h"
#include "Task.h"


/*
 *		Relocatable File
 *
 *	+------------------------+
 *	|		ELF Header		 |	- Elf64_Ehdr
 *	+------------------------+
 *	|  Program Header Table  |
 *	|		(optional)		 |
 *	+------------------------+
 *	|		Section 1		 |  - Elf64_Sym, Elf64_Rel, Elf64_Rela
 *	+------------------------+
 *	|		Section 2		 |
 *	+------------------------+
 *	|		  . . .			 |
 *	+------------------------+
 *	|		Section n		 |
 *	+------------------------+
 *	|  	 Section Header 1	 |	- Elf64_Shdr
 *	+------------------------+
 *	|  	 Section Header 2	 |
 *	+------------------------+
 *	|  Section Header Table	 |
 *	+------------------------+
 *	|    Section Header n    |
 *	+------------------------+
 *
 *	
 *  Compile / Link command, options 
 *  	ex) make Output.elf using Main.c
 *
 *  $ x86_64-pc-linux-gcc -c -m64 -ffreestanding Main.c
 *  $ x86_64-pc-linux-ld -melf_x86_64 -T elf_x86_64.x -nostdlib -e _START -Ttext 0x0000 -r -o Main.elf Main.o
 *  $ x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss Main.elf Output.elf
 *
 */


// --------------- Macro ---------------

// basic data types

#define Elf64_Addr			unsigned long		// 8 byte - unsigned program address
#define Elf64_Off			unsigned long		// 8 byte - unsigned file offset
#define Elf64_Half			unsigned short int	// 2 byte - unsigned medium integer
#define Elf64_Word			unsigned int		// 4 byte - unsigned integer
#define Elf64_Sword			int					// 4 byte - signed integer
#define Elf64_Xword			unsigned long		// 8 byte - unsigned long integer
#define Elf64_Sxword		long				// 8 byte - signed long integer

// ELF Header - index of e_ident[]

#define EI_MAG0				0					// File identification
#define EI_MAG1				1						
#define EI_MAG2				2
#define EI_MAG3				3
#define EI_CLASS			4					// File class
#define EI_DATA				5					// Data encoding
#define EI_VERSION			6					// File version
#define EI_OSABI			7					// OS/ABI identification
#define EI_ABIVERSION		8					// ABI version
#define EI_PAD				9					// Start of padding bytes
#define EI_NIDENT			16					// Size of e_ident[]

// ELF Header - e_ident[EI_MAGX]

#define ELFMAG0				0x7F				
#define ELFMAG1				'E'					
#define ELFMAG2				'L'					
#define ELFMAG3				'F'					

// ELF Header - e_ident[EI_CLASS]

#define ELFCLASSNONE		0					 
#define ELFCLASS32			1					// 32-bit objects
#define	ELFCLASS64			2					// 64-bit objects

// ELF Header - e_ident[EI_DATA]

#define ELFDATANONE			0					
#define ELFDATA2LSB			1					// Object file data structures are little-endian
#define ELFDATA2MSB			2					// Object file data structures are big-endian

// ELF Header - e_ident[OSABI]

#define ELFOSABI_NONE		0
#define ELFOSABI_HPUX		1
#define ELFOSABI_NETBSD		2
#define ELFOSABI_LINUX		3
#define ELFOSABI_SOLARIS	6
#define ELFOSABI_AIX		7
#define ELFOSABI_FREEBSD	9

// ELF Header - e_type (Object File Types)

#define ET_NONE				0					// No file type
#define ET_REL				1					// Relocatable object file
#define ET_EXEC				2					// Executable file
#define ET_DYN				3					// Shared object file
#define ET_CORE				4					// Core file
#define ET_LOOS				0xFE00				// Environment-specific use
#define ET_HIOS				0xFEFF				
#define ET_LOPROC			0xFF00				// Processor-specific use
#define ET_HIPROC			0xFFFF			

// ELF Header - e_machine

#define EM_NONE				0
#define EM_M32				1
#define EM_SPARC			2
#define EM_386				3
#define EM_PPC				20
#define EM_PPC64			21
#define EM_ARM				40
#define EM_IA_64			50
#define EM_X86_64			62
#define EM_AVR				83
#define EM_AVR32			185
#define EM_CUDA				190

// Special Section Index
// 		Section index 0, and indices in the range 0xFF00-0xFFFF
// 		are reserved for special purpose.

#define SHN_UNDEF			0					// used to mark an undefined or meaningless section reference
#define SHN_LORESERVE		0xFF00				
#define SHN_LOPROC			0xFF00				// Processor-specific use
#define SHN_HIPROC			0xFF1F				
#define SHN_LOOS			0xFF20				// Environment-specific use
#define SHN_HIOS			0xFF3F				
#define SHN_ABS				0xFFF1				// indicates that the corresponding reference is an absolute value
#define SHN_COMMON			0xFFF2				// indicates a symbol that has been declared as a common block
#define SHN_XINDEX			0xFFFF				
#define SHN_HIRESERVE		0xFFFF				

// Section Header - sh_type
//
// 		1. Use of the sh_link field
// 		
// 		Section Type				Associated Section
// 		----------------------------------------------------------------------------
// 		SHT_DYNAMIC					String table used by entries in this section
// 		SHT_HASH					Symbol table to which the hash table applies
//		SHT_REL, SHT_RELA			Symbol table referenced by relocations
//		SHT_SYMTAB, SHT_DYNSYM		String table used by entries in this section
//		Other						SHN_UNDEF
//
//		2. Use of the sh_info field
//
//		Section Type				sh_info
// 		----------------------------------------------------------------------------
// 		SHT_REL, SHT_RELA			Section index of section to which the relocations apply
//		SHT_SYMTAB, SHT_DYNSYM		Index of first non-local symbol (i.e., number of local symbols)
//		Other						0

#define SHT_NULL			0					// marks an unused section header 
#define SHT_PROGBITS		1					// contains information defined by the program (.text, .data, .rodata ..)
#define SHT_SYMTAB			2					// contains a linker symbol table (.symtab ..)
#define SHT_STRTAB			3					// contains a string table (.strtab, .shstrtab ..)
#define SHT_RELA			4					// contains "Rela" type relocation entries (section name with .rela prefix)
#define SHT_HASH			5					// contains a symbol hash table
#define SHT_DYNAMIC			6					// contains dynamic linking tables (.dynamic)
#define SHT_NOTE			7					// contains note information
#define SHT_NOBITS			8					// contains uninitialized space; does not occupy any space in the file (.bss ..)
#define SHT_REL				9					// contains "Rel" type relocation entries (section name with .rel prefix)
#define SHT_SHLIB			10					// reserved
#define SHT_DYNSYM			11					// contains a dynamic loader symbol table
#define SHT_LOOS			0x60000000			// Environment-specific use
#define SHT_HIOS			0x6FFFFFFF			
#define SHT_LOPROC			0x70000000			// Processor-specific use
#define SHT_HIPROC			0x7FFFFFFF			
#define SHT_LOUSER			0x80000000
#define SHT_HIUSER			0xFFFFFFFF

// Section Header - sh_flags

#define SHF_WRITE			1					// section contains writable data
#define SHF_ALLOC			2					// section is allocated in memory image of program
#define SHF_EXECINSTR		4					// section contains executable instructions
#define SHF_MASKOS			0x0F000000			// Environment-specific use
#define SHF_MASKPROC		0xF0000000			// Processor-specific use

// Special Section Index

#define SHN_UNDEF			0
#define SHN_LORESERVE		0xFF00
#define SHN_LOPROC			0xFF00
#define SHN_HIPROC			0xFF1F
#define SHN_ABS				0xFFF1
#define SHN_COMMON			0xFFF2
#define SHN_HIRESERVE		0xFFFF

// Section Entry - r_info of SHT_REL, SHT_RELA - Relocation Type (lower 32 bit of r_info field)
// 
// 		S : st_value of Elf64_Sym + loaded address of Symbol Section
// 		P : r_offset of Elf64_Rel, Elf64_Rela + loaded address of Relocation Section
// 		A : r_addend of Elf64_Rela
// 		B : loaded address of Relocation Section
// 		Z : st_size of Elf64_Sym
												// Size of field to relocate (bit)		Relocation type
												// --------------------------------------------------------------
#define R_X86_64_NONE		0					// none
#define R_X86_64_64			1					// word64								S + A
#define R_X86_64_PC32		2					// word32								S + A - P
#define R_X86_64_GOT32		3					// word32								G + A
#define R_X86_64_PLT32		4					// word32								L + A - P
#define R_X86_64_COPY		5					// none
#define R_X86_64_GLOB_DAT	6					// word64								S
#define R_X86_64_JUMP_SLOT	7					// word64								S
#define R_X86_64_RELATIVE	8					// word64								B + A
#define R_X86_64_GOTPCREL	9					// word32								G + GOT + A - P
#define R_X86_64_32			10					// word32								S + A
#define R_X86_64_32S		11					// word32								S + A
#define R_X86_64_16			12					// word16								S + A
#define R_X86_64_PC16		13					// word16								S + A - P
#define R_X86_64_8			14					// word8								S + A
#define R_X86_64_PC8		15					// word8								S + A - P
#define R_X86_64_DPTMOD64	16					// word64
#define R_X86_64_DTPOFF64	17					// word64
#define R_X86_64_TPOFF64	18					// word64
#define R_X86_64_TLSGD		19					// word32
#define R_X86_64_TLSLD		20					// word32
#define R_X86_64_DTPOFF32	21					// word32
#define R_X86_64_GOTTPOFF	22					// word32
#define R_X86_64_TPOFF32	23					// word32
#define R_X86_64_PC64		24					// word64								S + A - P
#define R_X86_64_GOTOFF64	25					// word64								S + A - GOT
#define R_X86_64_GOTPC32	26					// word32								GOT + A - P
#define R_X86_64_SIZE32		32					// word32								Z + A
#define R_X86_64_SIZE64		33					// word64								Z + A

// get upper 32 / lower 32 bits

#define RELOCATION_UPPER32(x)		( (x) >> 32 )
#define RELOCATION_LOWER32(x)		( (x) & 0xFFFFFFFF )



// --------------- Structure ---------------

#pragma pack(push, 1)


// ELF Header of ELF file format
typedef struct
{
	unsigned char	e_ident[16];		// ELF Identification
	Elf64_Half		e_type;				// Object file type					
	Elf64_Half		e_machine;			// Machine type
	Elf64_Word		e_version;			// Object file version
	Elf64_Addr		e_entry;			// Entry Point address (address of _START that set in compile)
	Elf64_Off		e_phoff;			// Program Header Table offset
	Elf64_Off		e_shoff;			// Section Header Table offset
	Elf64_Word		e_flags;			// Processor-specific flags
	Elf64_Half		e_ehsize;			// ELF Header size
	Elf64_Half		e_phentsize;		// one Program Header entry size
	Elf64_Half		e_phnum;			// Number of Program Header entry
	Elf64_Half		e_shentsize;		// one Section Header entry size
	Elf64_Half		e_shnum;			// Number of Section Header entry
	Elf64_Half		e_shstrndx;			// Section Name String Table index
} Elf64_Ehdr;


// ELF64 Section Header
typedef struct
{
	Elf64_Word		sh_name;			// Section Header Name offset (Section name)
	Elf64_Word		sh_type;			// Section type
	Elf64_Xword		sh_flags;			// Section flags (attributes)
	Elf64_Addr		sh_addr;			// Memory address to load (Virtual address in memory)
	Elf64_Off		sh_offset;			// Section byte offset (offset in file)
	Elf64_Xword		sh_size;			// Section size
	Elf64_Word		sh_link;			// Link to other section (Associated section index)
	Elf64_Word		sh_info;			// Miscellaneous information
	Elf64_Xword		sh_addralign;		// Address alignment value (Address alignment boundary)
	Elf64_Xword		sh_entsize;			// Data Entry size (if section has table)
} Elf64_Shdr;


// ELF64 Symbol Table Entry
typedef struct
{
	Elf64_Word		st_name;			// Symbol Name offset
	unsigned char	st_info;			// Symbol type / binding attribute
	unsigned char	st_other;			// Reserved
	Elf64_Half		st_shndx;			// Section Header index of the section which the symbol defined (need to relocate)
	Elf64_Addr		st_value;			// Symbol value (if relocatable.. the offset of the defined symbol based on st_shndx)
	Elf64_Xword		st_size;			// Symbol size (if variable.. means variable size, if function.. means function code size)
} Elf64_Sym;


// ELF64 Relocation Entry (SHT_REL Type)
typedef struct
{
	Elf64_Addr		r_offset;			// location at which to apply the relocation action (meaning byte offset for relocatable)
	Elf64_Xword		r_info;				// upper 32 bit : referred Symbol entry index / lower 32 bit : relocation type
} Elf64_Rel;


// ELF64 Relocation Entry (SHT_RELA Type)
typedef struct
{
	Elf64_Addr		r_offset;			// location at which to apply the relocation action (meaning byte offset)
	Elf64_Xword		r_info;				// upper 32 bit : referred Symbol entry index / lower 32 bit : relocation type
	Elf64_Sxword	r_addend;			// number to add (constant part of expression)
} Elf64_Rela;


#pragma pack(pop)



// --------------- Function ---------------

QWORD kExecuteProgram(const char* pcFileName, const char* pcArgumentString, BYTE bAffinity);
static BOOL kLoadProgramAndRelocation(BYTE*  pbFileBuffer			 , QWORD* pqwApplicationMemoryAddress, 
									  QWORD* pqwApplicationMemorySize, QWORD* pqwEntryPointAddress);
static BOOL kRelocation(BYTE* pbFileBuffer);
static void kAddArgumentStringToTask(TCB* pstTask, const char* pcArgumentString);
QWORD kCreateThread(QWORD qwEntryPoint, QWORD qwArgument, BYTE bAffinity, QWORD qwExitFunction);



#endif
