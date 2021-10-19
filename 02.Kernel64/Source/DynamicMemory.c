#include "DynamicMemory.h"
#include "Utility.h"
#include "Task.h"



static DYNAMICMEMORY gs_stDynamicMemory;



void kInitializeDynamicMemory(void)
{
	QWORD qwDynamicMemorySize;
	int i, j;
	BYTE* pbCurrentBitmapPosition;
	int iBlockCountOfLevel, iMetaBlockCount;


	// initialize DYNAMICMEMORY.iBlockCountOfSmallestBlock
	qwDynamicMemorySize = kCalculateDynamicMemorySize();
	iMetaBlockCount = kCalculateMetaBlockCount(qwDynamicMemorySize);

	gs_stDynamicMemory.iBlockCountOfSmallestBlock = (qwDynamicMemorySize / DYNAMICMEMORY_MIN_SIZE) - iMetaBlockCount;
	
	
	// initialize DYNAMICMEMORY.iMaxLevelCount
	for(i=0; (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) > 0; i++)
	{
		;
	}

	gs_stDynamicMemory.iMaxLevelCount = i;

	
	// initialize DYNAMICMEMORY.pbAllocatedBlockListIndex
	gs_stDynamicMemory.pbAllocatedBlockListIndex = (BYTE*)DYNAMICMEMORY_START_ADDRESS;
	
	for(i=0; i<gs_stDynamicMemory.iBlockCountOfSmallestBlock; i++)
	{
		gs_stDynamicMemory.pbAllocatedBlockListIndex[i] = 0xFF;
	}

	
	// initialize DYNAMICMEMORY.pstBitmapOfLevel
	gs_stDynamicMemory.pstBitmapOfLevel = 
		(BITMAP*)( DYNAMICMEMORY_START_ADDRESS + (sizeof(BYTE) * gs_stDynamicMemory.iBlockCountOfSmallestBlock) );
	

	// Initialize BITMAP Data Structure, Real Bitmap Address
	pbCurrentBitmapPosition = 
		((BYTE*)gs_stDynamicMemory.pstBitmapOfLevel) + (sizeof(BITMAP) * gs_stDynamicMemory.iMaxLevelCount);


	for(j=0; j<gs_stDynamicMemory.iMaxLevelCount; j++)		// loop while Block List Count (smallest -> biggest)
	{
		gs_stDynamicMemory.pstBitmapOfLevel[j].pbBitmap 	   = pbCurrentBitmapPosition;
		gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 0;

		iBlockCountOfLevel = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> j;		// divided to half

		// block which divisible by 8 can be combined with parent block
		for(i=0; i<iBlockCountOfLevel / 8; i++)
		{
			*pbCurrentBitmapPosition = 0x00;
			pbCurrentBitmapPosition++;		// move next Byte
		}

		// handle remainder block
		if( (iBlockCountOfLevel % 8) != 0 )
		{
			*pbCurrentBitmapPosition = 0x00;

			i = iBlockCountOfLevel % 8;

			if( (i%2) == 1 )
			{
				*pbCurrentBitmapPosition |= ( DYNAMICMEMORY_EXIST << (i - 1) );		// set Odd bit to 1
				gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 1;			// one bit exists
			}

			pbCurrentBitmapPosition++;
		}
	}	// for() end - loop while Block List Count


	// initialize remained member value
	gs_stDynamicMemory.qwStartAddress = DYNAMICMEMORY_START_ADDRESS + iMetaBlockCount * DYNAMICMEMORY_MIN_SIZE;
	gs_stDynamicMemory.qwEndAddress	  = kCalculateDynamicMemorySize() + DYNAMICMEMORY_START_ADDRESS;
	gs_stDynamicMemory.qwUsedSize	  = 0;	


	// initialize SpinLock
	kInitializeSpinLock(&(gs_stDynamicMemory.stSpinLock));
}


// Calculate size of Dynamic Memory Area
static QWORD kCalculateDynamicMemorySize(void)
{
	// --------------------------------------------
	// size unit : BYTE
	// --------------------------------------------
	
	QWORD qwRAMSize;

	
	// the Register used by Video Memory, Processor is at above 3 GB
	// 		So, Maximum Dynamic Memory Area is 3 GB	
	qwRAMSize = (kGetTotalRAMSize() * 1024 * 1024);		// unit : Byte

	if(qwRAMSize > (QWORD)3*1024*1024*1024)		// Max Size : 3 GB
	{
		qwRAMSize = (QWORD)3*1024*1024*1024;
	}


	// return available size(Byte) for Dynamic Memory
	return qwRAMSize - DYNAMICMEMORY_START_ADDRESS;		
}


// Calculate Metadata Block that used to manage Dynamic Memory 
// 		(Metadata Block depends on Dynamic Memory size)
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize)
{
	// --------------------------------------------
	// size unit : BYTE
	// --------------------------------------------
	
	long lBlockCountOfSmallestBlock;
	DWORD dwSizeOfAllocatedBlockListIndex;
	DWORD dwSizeOfBitmap;
	long i;


	// 1. Block List Index
	lBlockCountOfSmallestBlock = qwDynamicRAMSize / DYNAMICMEMORY_MIN_SIZE;		// Dynamic Memory Area / Minimum Size
	dwSizeOfAllocatedBlockListIndex = lBlockCountOfSmallestBlock * sizeof(BYTE);


	// 2. BITMAP Data Structure, Block List Bitmaps
	dwSizeOfBitmap = 0;
	for(i=0; (lBlockCountOfSmallestBlock >> i) > 0; i++)		// Block count decreased to half each time ( /2 )
	{
		dwSizeOfBitmap += sizeof(BITMAP);			// BITMAP Data Structure
		dwSizeOfBitmap += ( (lBlockCountOfSmallestBlock >> i) + 7 ) / 8;		// Block List Bitmap size (Bytes in Bitmap)
	}

	// aligned by DYNAMICMEMORY_MIN_SIZE
	return (dwSizeOfAllocatedBlockListIndex + dwSizeOfBitmap + DYNAMICMEMORY_MIN_SIZE - 1) / DYNAMICMEMORY_MIN_SIZE;
}


void* kAllocateMemory(QWORD qwSize)
{
	QWORD qwAlignedSize;
	QWORD qwRelativeAddress;
	long  lOffset;
	int   iSizeArrayOffset;
	int   iIndexOfBlockList;


	qwAlignedSize = kGetBuddyBlockSize(qwSize);

	if(qwAlignedSize == 0)
	{
		return NULL;
	}
	
	// if available space not enough..
	if(gs_stDynamicMemory.qwStartAddress + gs_stDynamicMemory.qwUsedSize + 
			qwAlignedSize > gs_stDynamicMemory.qwEndAddress)
	{
		return NULL;
	}


	lOffset = kAllocationBuddyBlock(qwAlignedSize);

	if(lOffset == -1)
	{
		return NULL;
	}


	iIndexOfBlockList = kGetBlockListIndexOfMatchSize(qwAlignedSize);


	// save Block List Index which allocated Buddy Block belongs to Block Size Area
	// 		use Block List Index when free the memory
	
	qwRelativeAddress = qwAlignedSize * lOffset;		// Relative Address based on Block Pool
	iSizeArrayOffset  = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;		// Allocated Block List Index offset

	gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] = (BYTE)iIndexOfBlockList;
	gs_stDynamicMemory.qwUsedSize += qwAlignedSize;


	// return absolute Address
	return (void*)(qwRelativeAddress + gs_stDynamicMemory.qwStartAddress);
}


// return size of nearest Buddy Block
static QWORD kGetBuddyBlockSize(QWORD qwSize)
{
	long i;

	for(i=0; i<gs_stDynamicMemory.iMaxLevelCount; i++)
	{
		if( qwSize <= (DYNAMICMEMORY_MIN_SIZE << i) )
			return (DYNAMICMEMORY_MIN_SIZE << i);
	}

	return 0;
}


// allocate Memory Block by Buddy Block Algorithm
// 		Memory Size (Param 1) must be aligned by Buddy Block Size
static int kAllocationBuddyBlock(QWORD qwAlignedSize)
{
	int iBlockListIndex, iFreeOffset;
	int i;


	iBlockListIndex = kGetBlockListIndexOfMatchSize(qwAlignedSize);

	if(iBlockListIndex == -1)
		return -1;


	// --------------------------------------
	// CRITICAL SECTION START
	kLockForSpinLock(&(gs_stDynamicMemory.stSpinLock));
	// --------------------------------------
	

	// Search match block and Select it
	for(i=iBlockListIndex; i<gs_stDynamicMemory.iMaxLevelCount; i++)
	{
		iFreeOffset = kFindFreeBlockInBitmap(i);

		if(iFreeOffset != -1)
		{
			break;
		}
	}
	

	if(iFreeOffset == -1)
	{
		// CRITICAL SECTION END
		kUnlockForSpinLock(&(gs_stDynamicMemory.stSpinLock));

		return -1;
	}


	// set EMPTY Flag to Free Block Offset (bit)
	kSetFlagInBitmap(i, iFreeOffset, DYNAMICMEMORY_EMPTY);


	// if Block found in higher block, devide higher one
	if(i > iBlockListIndex)
	{
		// Go down from searched Block List to searching-started Block List
		for(i = i-1; i >= iBlockListIndex; i--)
		{
			kSetFlagInBitmap(i, iFreeOffset*2  , DYNAMICMEMORY_EMPTY);		// Left block
			kSetFlagInBitmap(i, iFreeOffset*2+1, DYNAMICMEMORY_EXIST);		// Right block

			iFreeOffset = iFreeOffset*2;
		}
	}

	// --------------------------------------
	// CRITICAL SECTION END
	kUnlockForSpinLock(&(gs_stDynamicMemory.stSpinLock));
	// --------------------------------------
	
	return iFreeOffset;
}


static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize)
{
	int i;

	for(i=0; i<gs_stDynamicMemory.iMaxLevelCount; i++)
	{
		if( qwAlignedSize <= (DYNAMICMEMORY_MIN_SIZE << i) )
			return i;
	}

	return -1;
}


// find the bit which is set (DYNAMICMEMORY_EXIST)
static int kFindFreeBlockInBitmap(int iBlockListIndex)		
{
	int i, iMaxCount;
	BYTE* pbBitmap;
	QWORD* pqwBitmap;


	// No data exist in Bitmap
	if(gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount == 0)
	{
		return -1;
	}

	iMaxCount = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> iBlockListIndex;	// number of Blocks in each Block List
	pbBitmap  = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;		// indicates Bitmap in each Block List

	for(i=0; i<iMaxCount; )		// 'i' means bit index
	{
		// check "64 bit" (QWORD : 8 bit * 8 = 64 bit)
		if( ((iMaxCount - i) / 64) > 0 )		// if Block count is bigger than 64...
		{
			pqwBitmap = (QWORD*)&(pbBitmap[i/8]);

			if(*pqwBitmap == 0)		// QWORD Size Bitmap Block is free
			{
				i += 64;
				continue;
			}
		}

		// check "each bit"
		if( (pbBitmap[i/8] & (DYNAMICMEMORY_EXIST << (i%8))) != 0 )		// Byte Offset, Bit Offset
		{
			return i;
		}

		i++;
	}

	return -1;
}


static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag)
{
	// iOffset : bit offset

	BYTE* pbBitmap;

	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;
	
	if(bFlag == DYNAMICMEMORY_EXIST)
	{
		// if Data is empty, increase qwExistBitCount
		if( (pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) == 0 )
		{
			gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount++;
		}

		pbBitmap[iOffset / 8] |= (0x01 << (iOffset % 8));
	}
	else	// bFlag == DYNAMICMEMORY_EMPTY
	{
		// if Data is exist, decrease qwExistBitCount
		if( (pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0)
		{
			gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount--;
		}

		pbBitmap[iOffset / 8] &= ~(0x01 << (iOffset % 8));
	}
}


BOOL kFreeMemory(void* pvAddress)
{
	QWORD qwRelativeAddress;
	int   iSizeArrayOffset;
	QWORD qwBlockSize;
	int   iBlockListIndex;
	int   iBitmapOffset;


	if(pvAddress == NULL)
		return FALSE;


	qwRelativeAddress = ((QWORD)pvAddress) - gs_stDynamicMemory.qwStartAddress;		// Address based on Block Pool
	iSizeArrayOffset  = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

	// if not allocated.. No return
	if(gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] == 0xFF)
	{
		return FALSE;
	}

	
	iBlockListIndex = (int)gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset];
	gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] = 0xFF;

	// calculate size of allocated memory
	qwBlockSize = DYNAMICMEMORY_MIN_SIZE << iBlockListIndex;

	iBitmapOffset = qwRelativeAddress / qwBlockSize;
	
	if(kFreeBuddyBlock(iBlockListIndex, iBitmapOffset) == TRUE)
	{
		gs_stDynamicMemory.qwUsedSize -= qwBlockSize;
		
		return TRUE;
	}

	return FALSE;
}


// Free Buddy Block in Block List
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset)
{
	int iBuddyBlockOffset;
	int i;
	BOOL bFlag;


	// --------------------------------------
	// CRITICAL SECTION START
	kLockForSpinLock(&(gs_stDynamicMemory.stSpinLock));
	// --------------------------------------
	
	
	for(i=iBlockListIndex; i<gs_stDynamicMemory.iMaxLevelCount; i++)
	{
		kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EXIST);


		if((iBlockOffset % 2) == 0)
		{
			iBuddyBlockOffset = iBlockOffset + 1;
		}
		else
		{
			iBuddyBlockOffset = iBlockOffset - 1;
		}


		bFlag = kGetFlagInBitmap(i, iBuddyBlockOffset);

		if(bFlag == DYNAMICMEMORY_EMPTY)
			break;


		kSetFlagInBitmap(i, iBuddyBlockOffset, DYNAMICMEMORY_EMPTY);
		kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EMPTY);

		iBlockOffset = iBlockOffset / 2;
	}


	// --------------------------------------
	// CRITICAL SECTION END
	kUnlockForSpinLock(&(gs_stDynamicMemory.stSpinLock));
	// --------------------------------------

	
	return TRUE;
}


static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset)
{
	BYTE* pbBitmap;

	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;

	if( (pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0x00 )
	{
		return DYNAMICMEMORY_EXIST;
	}

	return DYNAMICMEMORY_EMPTY;
}


void kGetDynamicMemoryInformation(QWORD* pqwDynamicMemoryStartAddress, QWORD* pqwDynamicMemoryTotalSize,
								  QWORD* pqwMetaDataSize,			   QWORD* pqwUsedMemorySize)
{
	*pqwDynamicMemoryStartAddress = DYNAMICMEMORY_START_ADDRESS;
	*pqwDynamicMemoryTotalSize    = kCalculateDynamicMemorySize();
	*pqwMetaDataSize	          = kCalculateMetaBlockCount(*pqwDynamicMemoryTotalSize) * DYNAMICMEMORY_MIN_SIZE;
	*pqwUsedMemorySize		      = gs_stDynamicMemory.qwUsedSize;
}


DYNAMICMEMORY* kGetDynamicMemoryManager(void)
{
	return &gs_stDynamicMemory;
}



