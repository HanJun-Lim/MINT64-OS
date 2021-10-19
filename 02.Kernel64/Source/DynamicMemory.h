#ifndef __DYNAMICMEMORY_H__
#define __DYNAMICMEMORY_H__

#include "Types.h"
#include "Synchronization.h"


// -------------------------------------------
// Macro
// -------------------------------------------

// aligned by 2MB (Page size)
// 		Kernel Stack area included into Dynamic Memory Management area
#define DYNAMICMEMORY_START_ADDRESS ( (TASK_STACKPOOLADDRESS + 0x1FFFFF) & 0xFFFFFFFFFFE00000 )	

#define DYNAMICMEMORY_MIN_SIZE		(1 * 1024)		// Minimum size of Buddy Block : 1 KB

#define DYNAMICMEMORY_EXIST			0x01			// Exist in Block List, not allocated
#define DYNAMICMEMORY_EMPTY			0x00			// Set empty by being allocated



// -------------------------------------------
// Structure
// -------------------------------------------

// for Bitmap Management
typedef struct kBitmapStruct
{
	BYTE* pbBitmap;
	QWORD qwExistBitCount;
} BITMAP;


// for Buddy Block Management
typedef struct kDynamicMemoryManagerStruct
{
	// SpinLock for Synchronization
	SPINLOCK stSpinLock;

	int iMaxLevelCount;					// number of Block List
	int iBlockCountOfSmallestBlock;		// number of Smallest Block
	QWORD qwUsedSize;					// Allocated memory size

	// Block Pool Address
	QWORD qwStartAddress;
	QWORD qwEndAddress;

	BYTE* 	pbAllocatedBlockListIndex;		// Block index which Allocated memory belongs
	BITMAP* pstBitmapOfLevel;				// Address of Bitmap Data Structure
} DYNAMICMEMORY;



// -------------------------------------------
// Function
// -------------------------------------------

void  kInitializeDynamicMemory(void);
void* kAllocateMemory(QWORD qwSize);
BOOL  kFreeMemory(void* pvAddress);
void  kGetDynamicMemoryInformation(QWORD* pqwDynamicMemoryStartAddress, QWORD* pqwDynamicMemoryTotalSize,
								   QWORD* pqwMetaDataSize,              QWORD* pqwUsedMemorySize);
DYNAMICMEMORY* kGetDynamicMemoryManager(void);


static QWORD kCalculateDynamicMemorySize(void);
static int   kCalculateMetaBlockCount(QWORD qwDynamicRAMSize);
static int   kAllocationBuddyBlock(QWORD qwAlignedSize);
static QWORD kGetBuddyBlockSize(QWORD qwSize);
static int   kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize);
static int   kFindFreeBlockInBitmap(int iBlockListIndex);
static void  kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag);
static BOOL  kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset);
static BYTE  kGetFlagInBitmap(int iBlockListIndex, int iOffset);



#endif
