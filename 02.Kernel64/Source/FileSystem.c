#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Task.h"
#include "Utility.h"
#include "CacheManager.h"
#include "RAMDisk.h"


static FILESYSTEMMANAGER gs_stFileSystemManager;

// File System Temporary Buffer
static BYTE gs_vbTempBuffer[FILESYSTEM_SECTORSPERCLUSTER * 512];		// size : a Cluster


// declare Function Pointer
fReadHDDInformation gs_pfReadHDDInformation = NULL;
fReadHDDSector 	    gs_pfReadHDDSector 		= NULL;
fWriteHDDSector		gs_pfWriteHDDSector		= NULL;


BOOL kInitializeFileSystem(void)
{
	BOOL bCacheEnable = FALSE;


	// init Data Structure, Sync Object
	kMemSet(&gs_stFileSystemManager, 0, sizeof(gs_stFileSystemManager));
	kInitializeMutex(&(gs_stFileSystemManager.stMutex));


	// init HDD
	if(kInitializeHDD() == TRUE)
	{
		// if init success, set Function Pointer to HDD Function
		gs_pfReadHDDInformation = kReadHDDInformation;
		gs_pfReadHDDSector		= kReadHDDSector;
		gs_pfWriteHDDSector		= kWriteHDDSector;

		// enable Cache
		bCacheEnable = TRUE;
	}
	// if HDD init failed, create RAM Disk with 8 MB
	else if(kInitializeRDD(RDD_TOTALSECTORCOUNT) == TRUE)
	{
		gs_pfReadHDDInformation = kReadRDDInformation;
		gs_pfReadHDDSector		= kReadRDDSector;
		gs_pfWriteHDDSector		= kWriteRDDSector;

		// RAM Disk is volatile to Data, So create File System every Boot time
		if(kFormat() == FALSE)
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

	

	// Mount File System (Connect)
	if(kMount() == FALSE)
	{
		return FALSE;
	}


	// Allocate space for Handle
	gs_stFileSystemManager.pstHandlePool = (FILE*)kAllocateMemory(FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));
	
	if(gs_stFileSystemManager.pstHandlePool == NULL)	// if failed, set as HDD recognition failed
	{
		gs_stFileSystemManager.bMounted = FALSE;
		return FALSE;
	}

	// Initialize Handle Pool
	kMemSet(gs_stFileSystemManager.pstHandlePool, 0, FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));

	
	// Enable Cache
	if(bCacheEnable == TRUE)
	{
		gs_stFileSystemManager.bCacheEnable = kInitializeCacheManager();
	}


	return TRUE;
}


// =======================================================
// Low Level Function
// =======================================================


// Check if MINT File System by reading MBR
// 		if true, insert File System information into FILESYSTEMMANAGER Structure
BOOL kMount(void)
{
	MBR* pstMBR;

	/////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	/////////////////////////////////
	
	// Read MBR
	if(gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE)		// read LBA 0
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));

		return FALSE;
	}

	
	// Check Signature
	pstMBR = (MBR*)gs_vbTempBuffer;

	if(pstMBR->dwSignature != FILESYSTEM_SIGNATURE)
	{
		// CRITICAL SECITON END
		kUnlock(&(gs_stFileSystemManager.stMutex));

		return FALSE;
	}

	// File System Recognition Success
	gs_stFileSystemManager.bMounted = TRUE;

	// Calculate LBA Address, Sector Count of each area
	gs_stFileSystemManager.dwReservedSectorCount 		 = pstMBR->dwReservedSectorCount;
	gs_stFileSystemManager.dwClusterLinkAreaStartAddress = pstMBR->dwReservedSectorCount + 1;
	gs_stFileSystemManager.dwClusterLinkAreaSize		 = pstMBR->dwClusterLinkSectorCount;
	gs_stFileSystemManager.dwDataAreaStartAddress		 = pstMBR->dwReservedSectorCount + pstMBR->dwClusterLinkSectorCount + 1;
	gs_stFileSystemManager.dwTotalClusterCount 			 = pstMBR->dwTotalClusterCount;
	
	/////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	/////////////////////////////////
	
	return TRUE;
}


// Create File System into HDD
BOOL kFormat(void)
{
	HDDINFORMATION* pstHDD;
	MBR* pstMBR;
	DWORD dwTotalSectorCount, dwRemainSectorCount;
	DWORD dwMaxClusterCount, dwClusterCount;
	DWORD dwClusterLinkSectorCount;
	DWORD i;


	/////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	/////////////////////////////////
	
	
	// ==========================================================================
	// Read HDD information to calculate Metadata Area Size, Cluster Count
	// ==========================================================================
	
	// Calculate Total Sector Count of HDD
	pstHDD = (HDDINFORMATION*)gs_vbTempBuffer;
	
	if(gs_pfReadHDDInformation(TRUE, TRUE, pstHDD) == FALSE)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));

		return FALSE;
	}

	dwTotalSectorCount = pstHDD->dwTotalSectors;

	// Calculate Max Cluster Count
	dwMaxClusterCount = dwTotalSectorCount / FILESYSTEM_SECTORSPERCLUSTER;

	// Calculate Sector Count of Cluster Link Table
	// 		Link Data : 4 Byte, One Sector contains 128 Link Data
	// 		Round off by 128
	dwClusterLinkSectorCount = (dwMaxClusterCount + 127) / 128;


	// Reserved Area is not used in MINT64
	// so, Data Area is Total - MBR Area - Cluster Link Table Area
	dwRemainSectorCount = dwTotalSectorCount - dwClusterLinkSectorCount - 1;
	dwClusterCount 		= dwRemainSectorCount / FILESYSTEM_SECTORSPERCLUSTER;

	// Calculate again for Data Area Cluster
	dwClusterLinkSectorCount = (dwClusterCount + 127) / 128;



	// ==========================================================================
	// Write calculated information to MBR, initialize Root Directory Area by 0
	// ==========================================================================
	
	// read MBR
	if(gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));

		return FALSE;
	}


	// Set Partition info, File System info
	pstMBR = (MBR*)gs_vbTempBuffer;

	kMemSet(pstMBR->vstPartition, 0, sizeof(pstMBR->vstPartition));

	pstMBR->dwSignature 			 = FILESYSTEM_SIGNATURE;
	pstMBR->dwReservedSectorCount 	 = 0;
	pstMBR->dwClusterLinkSectorCount = dwClusterLinkSectorCount;
	pstMBR->dwTotalClusterCount 	 = dwClusterCount;


	// Write 1 sector to MBR
	if(gs_pfWriteHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));

		return FALSE;
	}

	
	kMemSet(gs_vbTempBuffer, 0, 512);
	

	// loop from Cluster Link Table (FAT) to Root Directory
	// 		unlink all File Links (erase all Files)
	for(i=0; i<(dwClusterLinkSectorCount + FILESYSTEM_SECTORSPERCLUSTER); i++)
	{
		if(i == 0)
		{
			((DWORD*)(gs_vbTempBuffer))[0] = FILESYSTEM_LASTCLUSTER;
		}
		else
		{
			((DWORD*)(gs_vbTempBuffer))[0] = FILESYSTEM_FREECLUSTER;
		}

		// Write 1 sector
		if(gs_pfWriteHDDSector(TRUE, TRUE, i+1, 1, gs_vbTempBuffer) == FALSE)
		{
			// CRITICAL SECTION END
			kUnlock(&(gs_stFileSystemManager.stMutex));

			return FALSE;
		}
	}
	
	
	// Flush Cache Buffer
	if(gs_stFileSystemManager.bCacheEnable == TRUE)
	{
		kDiscardAllCacheBuffer(CACHE_CLUSTERLINKTABLEAREA);
		kDiscardAllCacheBuffer(CACHE_DATAAREA);
	}


	/////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	/////////////////////////////////

	return TRUE;
}


BOOL kGetHDDInformation(HDDINFORMATION* pstInformation)
{
	BOOL bResult;

	/////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	/////////////////////////////////
	
	bResult = gs_pfReadHDDInformation(TRUE, TRUE, pstInformation);

	/////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	/////////////////////////////////

	return bResult;
}



static BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer)
{
	// if Cache enabled, Call function for Cache
	if(gs_stFileSystemManager.bCacheEnable == FALSE)
	{
		return kInternalReadClusterLinkTableWithoutCache(dwOffset, pbBuffer);
	}
	else
	{
		return kInternalReadClusterLinkTableWithCache(dwOffset, pbBuffer);
	}
}


// Read one Sector in Cluster Link Table
// 		Internal Function - Cache not used
static BOOL kInternalReadClusterLinkTableWithoutCache(DWORD dwOffset, BYTE* pbBuffer)
{
	// Direct read - Cluster Link Area Start Address + Offset
	return gs_pfReadHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress,
			1, pbBuffer);
}


// Read one Sector in Cluster Link Table
// 		Internal Function - Cache used
static BOOL kInternalReadClusterLinkTableWithCache(DWORD dwOffset, BYTE* pbBuffer)
{
	CACHEBUFFER* pstCacheBuffer;


	// Check if there's match Cluster Link Table in Cache
	pstCacheBuffer = kFindCacheBuffer(CACHE_CLUSTERLINKTABLEAREA, dwOffset);

	
	// 1. if exist, Copy memory in Cache to pbBuffer
	if(pstCacheBuffer != NULL)
	{
		kMemCpy(pbBuffer, pstCacheBuffer->pbBuffer, 512);

		return TRUE;
	}



	// 2. if not exist, Read directly from HDD
	if(kInternalReadClusterLinkTableWithoutCache(dwOffset, pbBuffer) == FALSE)
	{
		return FALSE;
	}
	

	// Allocate Cache and update
	pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_CLUSTERLINKTABLEAREA);
	
	if(pstCacheBuffer == NULL)
	{
		return FALSE;
	}


	// Copy read memory to Cache Buffer, update Tag info
	kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, 512);
	pstCacheBuffer->dwTag = dwOffset;

	// Read Complete, mark Buffer Contents as not modified
	pstCacheBuffer->bChanged = FALSE;

	
	return TRUE;
}


// Allocate from Cache Buffer of Cluster Link Table Area or Data Area
// 		if there's no empty Buffer, choose oldest one
static CACHEBUFFER* kAllocateCacheBufferWithFlush(int iCacheTableIndex)
{
	CACHEBUFFER* pstCacheBuffer;
	
	
	// Try Buffer Allocation. if failed, flush oldest Buffer then reuse it
	pstCacheBuffer = kAllocateCacheBuffer(iCacheTableIndex);
	

	// if all Cache buffer occupied..
	if(pstCacheBuffer == NULL)
	{
		pstCacheBuffer = kGetVictimInCacheBuffer(iCacheTableIndex);

		// if failed to allocate oldest Cache Buffer, fail
		if(pstCacheBuffer == NULL)
		{
			kPrintf("Cache Allocate Fail\n");

			return NULL;
		}
		

		// if Cache Buffer modified, move data to HDD (Flush)
		if(pstCacheBuffer->bChanged == TRUE)
		{
			switch(iCacheTableIndex)
			{
				case CACHE_CLUSTERLINKTABLEAREA:

					if(kInternalWriteClusterLinkTableWithoutCache(
							pstCacheBuffer->dwTag, pstCacheBuffer->pbBuffer) == FALSE)
					{
						kPrintf("Cache Buffer Write Fail\n");

						return NULL;
					}
					
					break;

				case CACHE_DATAAREA:

					if(kInternalWriteClusterWithoutCache(
							pstCacheBuffer->dwTag, pstCacheBuffer->pbBuffer) == FALSE)
					{
						kPrintf("Cache Buffer Write Fail\n");

						return NULL;
					}

					break;

				// Others are error
				default:

					kPrintf("kAllocateCacheBufferWithFlush() Fail\n");

					return NULL;

					break;
			}
		}	// data modified endif
	
	}	// all Cache occupied endif


	// return allocated Cache Buffer
	return pstCacheBuffer;
}


static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer)
{
	if(gs_stFileSystemManager.bCacheEnable == FALSE)
	{
		return kInternalWriteClusterLinkTableWithoutCache(dwOffset, pbBuffer);
	}
	else
	{
		return kInternalWriteClusterLinkTableWithCache(dwOffset, pbBuffer);
	}
}


// Write one Sector to Cluster Link Table
// 		Internal Function - Cache not used
static BOOL kInternalWriteClusterLinkTableWithoutCache(DWORD dwOffset, BYTE* pbBuffer)
{
	return gs_pfWriteHDDSector(TRUE, TRUE, dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress,
			1, pbBuffer);
}


// Write one Sector to Cluster Link Table
// 		Internal Function - Cache used
static BOOL kInternalWriteClusterLinkTableWithCache(DWORD dwOffset, BYTE* pbBuffer)
{
	CACHEBUFFER* pstCacheBuffer;


	// Check if match Cluster Link Table exist
	pstCacheBuffer = kFindCacheBuffer(CACHE_CLUSTERLINKTABLEAREA, dwOffset);


	// 1. if exist, write to Cache Buffer
	if(pstCacheBuffer != NULL)
	{
		kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, 512);

		// write complete, mark Buffer as modified
		pstCacheBuffer->bChanged = TRUE;


		return TRUE;
	}

	
	// 2. if not exist, allocate Cache Buffer and update
	pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_CLUSTERLINKTABLEAREA);

	if(pstCacheBuffer == NULL)
	{
		return FALSE;
	}
	

	// Write to Cache Buffer, update Tag Info
	kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, 512);
	pstCacheBuffer->dwTag = dwOffset;
	

	// Write Complete, so mark Buffer as modified
	pstCacheBuffer->bChanged = TRUE;


	return TRUE;
}


static BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer)
{
	if(gs_stFileSystemManager.bCacheEnable == FALSE)
	{
		return kInternalReadClusterWithoutCache(dwOffset, pbBuffer);
	}
	else
	{
		return kInternalReadClusterWithCache(dwOffset, pbBuffer);
	}
}


// Read one Cluster from Data Area Offset
// 		Internal Function - Cache not used
static BOOL kInternalReadClusterWithoutCache(DWORD dwOffset, BYTE* pbBuffer)
{
	return gs_pfReadHDDSector(TRUE, TRUE, 
			(dwOffset * FILESYSTEM_SECTORSPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress,
			FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}


// Read one Cluster from Data Area Offset
// 		Internal Function - Cache used
static BOOL kInternalReadClusterWithCache(DWORD dwOffset, BYTE* pbBuffer)
{
	CACHEBUFFER* pstCacheBuffer;


	// Check if matching Data Cluster exist
	pstCacheBuffer = kFindCacheBuffer(CACHE_DATAAREA, dwOffset);
	

	// 1. if Data exists in Cache Buffer, Copy Cache Data
	if(pstCacheBuffer != NULL)
	{
		kMemCpy(pbBuffer, pstCacheBuffer->pbBuffer, FILESYSTEM_CLUSTERSIZE);

		return TRUE;
	}

	
	// 2. if Data not exist in Cache Buffer, Read HDD directly
	if(kInternalReadClusterWithoutCache(dwOffset, pbBuffer) == FALSE)
	{
		return FALSE;
	}


	// Allocate Cache Buffer and update
	pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_DATAAREA);

	if(pstCacheBuffer == NULL)
	{
		return FALSE;
	}
	

	// Copy read contents to Cache Buffer, update Tag Info
	kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, FILESYSTEM_CLUSTERSIZE);
	pstCacheBuffer->dwTag = dwOffset;


	// Read Complete, so, mark Buffer Contents as not modified
	pstCacheBuffer->bChanged = FALSE;


	return TRUE;
}


static BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer)
{
	if(gs_stFileSystemManager.bCacheEnable == FALSE)
	{
		return kInternalWriteClusterWithoutCache(dwOffset, pbBuffer);
	}
	else
	{
		return kInternalWriteClusterWithCache(dwOffset, pbBuffer);
	}
}


// Write one Cluster to Data Area Offset
// 		Internal Function - Cache not used
static BOOL kInternalWriteClusterWithoutCache(DWORD dwOffset, BYTE* pbBuffer)
{
	return gs_pfWriteHDDSector(TRUE, TRUE,
			(dwOffset * FILESYSTEM_SECTORSPERCLUSTER) + gs_stFileSystemManager.dwDataAreaStartAddress,
			FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}


// Write one Cluster to Data Area Offset
// 		Internal Function - Cache used
static BOOL kInternalWriteClusterWithCache(DWORD dwOffset, BYTE* pbBuffer)
{
	CACHEBUFFER* pstCacheBuffer;


	// Check if matching Cluster Data exist in Cache Buffer
	pstCacheBuffer = kFindCacheBuffer(CACHE_DATAAREA, dwOffset);


	// 1. if Data exist, Write to Cache
	if(pstCacheBuffer != NULL)
	{
		kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, FILESYSTEM_CLUSTERSIZE);

		// Write Complete, so mark Buffer Contents as modified
		pstCacheBuffer->bChanged = TRUE;

		return TRUE;
	}


	// 2. if Data not exist, Allocate Cache and update 
	pstCacheBuffer = kAllocateCacheBufferWithFlush(CACHE_DATAAREA);

	if(pstCacheBuffer == NULL)
	{
		return FALSE;
	}


	// Write to Cache Buffer and update Tag Info
	kMemCpy(pstCacheBuffer->pbBuffer, pbBuffer, FILESYSTEM_CLUSTERSIZE);
	pstCacheBuffer->dwTag = dwOffset;


	// Write Complete, so, mark Buffer Contents as modified
	pstCacheBuffer->bChanged = TRUE;

	
	return TRUE;
}


// Find Empty Cluster in Cluster Link Table
static DWORD kFindFreeCluster(void)
{
	DWORD dwLinkCountInSector;
	DWORD dwLastSectorOffset, dwCurrentSectorOffset;
	DWORD i, j;


	// if File System not mounted, fail
	if(gs_stFileSystemManager.bMounted == FALSE)
	{
		return FILESYSTEM_LASTCLUSTER;
	}


	// get Sector Offset of Cluster Link Table that allocates Cluster recently
	dwLastSectorOffset = gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset;

	
	// search Empty Cluster
	for(i=0; i<gs_stFileSystemManager.dwClusterLinkAreaSize; i++)
	{
		// if found sector is the last one, 
		if( (dwLastSectorOffset + i) == (gs_stFileSystemManager.dwClusterLinkAreaSize - 1) )
		{
			dwLinkCountInSector = gs_stFileSystemManager.dwTotalClusterCount % 128;
		}
		else
		{
			dwLinkCountInSector = 128;		// 128 is the max count of Link Information in Sector
		}


		// get Sector Offset of Cluster Link Table to read
		dwCurrentSectorOffset = (dwLastSectorOffset + i) % gs_stFileSystemManager.dwClusterLinkAreaSize;

		// one sector of Cluster Link Table saved in gs_vbTempBuffer (Temporary Buffer)
		if(kReadClusterLinkTable(dwCurrentSectorOffset, gs_vbTempBuffer) == FALSE)
		{
			return FILESYSTEM_LASTCLUSTER;
		}


		// search Empty Cluster in Sector
		for(j=0; j<dwLinkCountInSector; j++)		// j means Link Information Offset in Sector
		{
			if( ((DWORD*)gs_vbTempBuffer)[j] == FILESYSTEM_FREECLUSTER )		// Link Information : 4 Byte
			{
				break;
			}
		}

		// if Empty Cluster found, return index
		if(j != dwLinkCountInSector)
		{
			gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset = dwCurrentSectorOffset;

			return (dwCurrentSectorOffset * 128) + j;		// return Link Information Offset
		}

	}	// Cluster Searching loop end

	return FILESYSTEM_LASTCLUSTER;
}


// Set data to Cluster Link Table
static BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData)
{
	DWORD dwSectorOffset;

	if(gs_stFileSystemManager.bMounted == FALSE)
	{
		return FALSE;
	}

	
	dwSectorOffset = dwClusterIndex / 128;

	if(kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE)
	{
		return FALSE;
	}

	
	((DWORD*)gs_vbTempBuffer)[dwClusterIndex % 128] = dwData;		// set Link Data

	
	if(kWriteClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE)
	{
		return FALSE;
	}

	
	return TRUE;
}


static BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* pdwData)
{
	DWORD dwSectorOffset;

	if(gs_stFileSystemManager.bMounted == FALSE)
	{
		return FALSE;
	}

	
	dwSectorOffset = dwClusterIndex / 128;


	// if Sector Offset is out of Cluster Link Area Size range...
	if(dwSectorOffset > gs_stFileSystemManager.dwClusterLinkAreaSize)
	{
		return FALSE;
	}

	if(kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE)
	{
		return FALSE;
	}

	*pdwData = ((DWORD*)gs_vbTempBuffer)[dwClusterIndex % 128];


	return TRUE;
}


// return Empty Directory Entry in Root Directory
static int kFindFreeDirectoryEntry(void)
{
	DIRECTORYENTRY* pstEntry;
	int i;


	if(gs_stFileSystemManager.bMounted == FALSE)
	{
		return -1;
	}

	// Read Root Directory
	if(kReadCluster(0, gs_vbTempBuffer) == FALSE)
	{
		return -1;
	}

	// Search Directory Entry which has Start Cluster Index 0
	pstEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;

	for(i=0; i<FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++)
	{
		if(pstEntry[i].dwStartClusterIndex == 0)
		{
			return i;
		}
	}

	return -1;
}


static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry)
{
	DIRECTORYENTRY* pstRootEntry;

	
	// Failed when
	// 1. File System not mounted
	// 2. invalid iIndex
	if( (gs_stFileSystemManager.bMounted == FALSE) || 
		(iIndex < 0) || (iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT) )
	{
		return FALSE;
	}

	// Read Root Directory
	if(kReadCluster(0, gs_vbTempBuffer) == FALSE)
	{
		return FALSE;
	}


	pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;

	kMemCpy(pstRootEntry + iIndex, pstEntry, sizeof(DIRECTORYENTRY));


	if(kWriteCluster(0, gs_vbTempBuffer) == FALSE)
	{
		return FALSE;
	}


	return TRUE;
}


static BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry)
{
	DIRECTORYENTRY* pstRootEntry;

	if( (gs_stFileSystemManager.bMounted == FALSE) ||
		(iIndex < 0) || (iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT) )
	{
		return FALSE;
	}

	// Read Root Directory
	if(kReadCluster(0, gs_vbTempBuffer) == FALSE)
	{
		return FALSE;
	}

	pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
	kMemCpy(pstEntry, pstRootEntry + iIndex, sizeof(DIRECTORYENTRY));


	return TRUE;
}


static int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry)
{
	DIRECTORYENTRY* pstRootEntry;
	int i;
	int iLength;


	if(gs_stFileSystemManager.bMounted == FALSE)
	{
		return -1;
	}

	if(kReadCluster(0, gs_vbTempBuffer) == FALSE)
	{
		return -1;
	}

	
	iLength = kStrLen(pcFileName);
	pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;

	for(i=0; i<FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++)
	{
		if(kMemCmp(pstRootEntry[i].vcFileName, pcFileName, iLength) == 0)
		{
			kMemCpy(pstEntry, pstRootEntry + i, sizeof(DIRECTORYENTRY));
			return i;
		}
	}

	return -1;
}


void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager)
{
	kMemCpy(pstManager, &gs_stFileSystemManager, sizeof(gs_stFileSystemManager));
}



// =======================================================
// High Level Function
// =======================================================


// Allocate Empty Handle
static void* kAllocateFileDirectoryHandle(void)
{
	int i;
	FILE* pstFile;


	// Search Handle Pool for Empty Handle
	pstFile = gs_stFileSystemManager.pstHandlePool;

	for(i=0; i<FILESYSTEM_HANDLE_MAXCOUNT; i++)
	{
		// if Empty, return Empty Handle Address
		if(pstFile->bType == FILESYSTEM_TYPE_FREE)
		{
			pstFile->bType = FILESYSTEM_TYPE_FILE;
			return pstFile;
		}

		pstFile++;
	}

	return NULL;
}


// Free Used Handle
static void kFreeFileDirectoryHandle(FILE* pstFile)
{
	kMemSet(pstFile, 0, sizeof(FILE));

	pstFile->bType = FILESYSTEM_TYPE_FREE;
}


// Create File
static BOOL kCreateFile(const char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex)
{
	DWORD dwCluster;

	
	// Search Empty Cluster, then set as allocated
	dwCluster = kFindFreeCluster();

	if( (dwCluster == FILESYSTEM_LASTCLUSTER) || (kSetClusterLinkData(dwCluster, FILESYSTEM_LASTCLUSTER) == FALSE) )
	{
		return FALSE;
	}


	// Search Empty Directory Entry
	*piDirectoryEntryIndex = kFindFreeDirectoryEntry();		// Free Directory Entry Address saved

	if(*piDirectoryEntryIndex == -1)
	{
		kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
		return FALSE;
	}
	
	
	// Set Directory Entry
	kMemCpy(pstEntry->vcFileName, pcFileName, kStrLen(pcFileName) + 1);
	pstEntry->dwStartClusterIndex = dwCluster;
	pstEntry->dwFileSize = 0;
	

	// Register Directory Entry
	if(kSetDirectoryEntryData(*piDirectoryEntryIndex, pstEntry) == FALSE)
	{
		kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
		return FALSE;
	}


	return TRUE;
}


// Free Clusters connecting each File Cluster
static BOOL kFreeClusterUntilEnd(DWORD dwClusterIndex)
{
	DWORD dwCurrentClusterIndex;
	DWORD dwNextClusterIndex;


	// Initialize Cluster Index
	dwCurrentClusterIndex = dwClusterIndex;

	while(dwCurrentClusterIndex != FILESYSTEM_LASTCLUSTER)
	{
		// Get Next Cluster Index
		if(kGetClusterLinkData(dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE)
		{
			return FALSE;
		}

		// Free Current Cluster
		if(kSetClusterLinkData(dwCurrentClusterIndex, FILESYSTEM_FREECLUSTER) == FALSE)
		{
			return FALSE;
		}

		dwCurrentClusterIndex = dwNextClusterIndex;
	}

	return TRUE;
}


// Open/Create File
FILE* kOpenFile(const char* pcFileName, const char* pcMode)
{
	DIRECTORYENTRY stEntry;
	int iDirectoryEntryOffset;
	int iFileNameLength;
	DWORD dwSecondCluster;
	FILE* pstFile;

	
	// Check if valid File Name
	iFileNameLength = kStrLen(pcFileName);

	if( (iFileNameLength > (sizeof(stEntry.vcFileName) - 1)) || (iFileNameLength == 0) )
	{
		return NULL;
	}
	
	
	////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////
	

	// =============================================================================
	// Check if File exists. when not found, Create File referring to Option (pcMode)
	// =============================================================================
	
	iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);

	if(iDirectoryEntryOffset == -1)		// if File not exists
	{
		// 'r' (read) option returns NULL (Fail)
		if(pcMode[0] == 'r')
		{
			// CRITICAL SECTION END
			kUnlock(&(gs_stFileSystemManager.stMutex));

			return NULL;
		}

		// other options ('w' or 'a') create File
		if(kCreateFile(pcFileName, &stEntry, &iDirectoryEntryOffset) == FALSE)
		{
			// CRITICAL SECTION END
			kUnlock(&(gs_stFileSystemManager.stMutex));

			return NULL;
		}
	}


	// =============================================================================
	// if the mode is 'w', Free clusters related to File, Set File Size 0 (TRUNC)
	// =============================================================================
	
	else if(pcMode[0] == 'w')
	{
		// Search Next Cluster of First Cluster
		if(kGetClusterLinkData(stEntry.dwStartClusterIndex, &dwSecondCluster) == FALSE)
		{
			// CRITICAL SECTION END
			kUnlock(&(gs_stFileSystemManager.stMutex));

			return NULL;
		}

		// make First Cluster to Last Cluster
		if(kSetClusterLinkData(stEntry.dwStartClusterIndex, FILESYSTEM_LASTCLUSTER) == FALSE)
		{	
			// CRITICAL SECTION END
			kUnlock(&(gs_stFileSystemManager.stMutex));

			return NULL;
		}

		// Free Clusters from Second Cluster to Last Cluster
		if(kFreeClusterUntilEnd(dwSecondCluster) == FALSE)
		{
			// CRITICAL SECTION END
			kUnlock(&(gs_stFileSystemManager.stMutex));

			return NULL;
		}

		// set File Size 0
		stEntry.dwFileSize = 0;

		if(kSetDirectoryEntryData(iDirectoryEntryOffset, &stEntry) == FALSE)
		{
			// CRITICAL SECTION END
			kUnlock(&(gs_stFileSystemManager.stMutex));

			return NULL;
		}
	}
	

	// =============================================================================
	// Allocate File Handle, then set data
	// =============================================================================
	
	pstFile = kAllocateFileDirectoryHandle();

	if(pstFile == NULL)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));

		return NULL;
	}

	
	// set File Information to File Handle
	pstFile->bType = FILESYSTEM_TYPE_FILE;

	pstFile->stFileHandle.iDirectoryEntryOffset  = iDirectoryEntryOffset;
	pstFile->stFileHandle.dwFileSize 			 = stEntry.dwFileSize;
	pstFile->stFileHandle.dwStartClusterIndex	 = stEntry.dwStartClusterIndex;
	pstFile->stFileHandle.dwCurrentClusterIndex	 = stEntry.dwStartClusterIndex;
	pstFile->stFileHandle.dwPreviousClusterIndex = stEntry.dwStartClusterIndex;
	pstFile->stFileHandle.dwCurrentOffset		 = 0;
	
	
	// if 'a' option set, move to End Of File
	if(pcMode[0] == 'a')
	{
		kSeekFile(pstFile, 0, FILESYSTEM_SEEK_END);
	}


	////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////
	
	return pstFile;
}


// Read File, then copy it to Buffer
DWORD kReadFile(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile)
{
	DWORD dwTotalCount;
	DWORD dwReadCount;
	DWORD dwOffsetInCluster;
	DWORD dwCopySize;
	FILEHANDLE* pstFileHandle;
	DWORD dwNextClusterIndex;

	
	// if Handle is not FILE Type, fail
	if( (pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE) )
	{
		return 0;
	}

	pstFileHandle = &(pstFile->stFileHandle);


	// if EOF or Last Cluster, end
	if( (pstFileHandle->dwCurrentOffset == pstFileHandle->dwFileSize) ||		// EOF
		(pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER) )		// Last Cluster
	{
		return 0;
	}
	

	// Bytes to read = MIN( Required bytes to read, Remaining bytes until EOF )
	dwTotalCount = MIN(dwSize * dwCount, pstFileHandle->dwFileSize - pstFileHandle->dwCurrentOffset);


	////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////
	
	dwReadCount = 0;

	while(dwReadCount != dwTotalCount)
	{
		// ================================================
		// Read Cluster, then write it to Buffer
		// ================================================
		
		// read Current Cluster
		if(kReadCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE)
		{
			break;
		}

		// Calculate Offset which includes File Pointer in Cluster
		dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;

		// if there's several Clusters to read, 
		// Read remaining data on Current Cluster, and then move Next Cluster
		// 		Size to copy = MIN( Max size to copy in Cluster, Bytes to read )
		dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwReadCount);

		kMemCpy((char*)pvBuffer + dwReadCount, gs_vbTempBuffer + dwOffsetInCluster, dwCopySize);

		// Update Read Count, File Pointer
		dwReadCount += dwCopySize;
		pstFileHandle->dwCurrentOffset += dwCopySize;

		
		// ================================================
		// if EOF on Current Cluster, move Next Cluster
		// ================================================
		
		if( (pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0 )
		{
			if(kGetClusterLinkData(pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE)
			{
				break;
			}

			// Update Cluster Info
			pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
			pstFileHandle->dwCurrentClusterIndex  = dwNextClusterIndex;
		}

	} 	// Copying File contents to Buffer loop end
	
	
	////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////

	
	// Return Byte read count
	return dwReadCount;
}


// Update Directory Entry on Root Directory
static BOOL kUpdateDirectoryEntry(FILEHANDLE* pstFileHandle)
{
	DIRECTORYENTRY stEntry;


	if( (pstFileHandle == NULL) || 
		(kGetDirectoryEntryData(pstFileHandle->iDirectoryEntryOffset, &stEntry) == FALSE) )
	{
		return FALSE;
	}
	

	// Update File Size, Start Cluster Index
	stEntry.dwFileSize = pstFileHandle->dwFileSize;
	stEntry.dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;


	if(kSetDirectoryEntryData(pstFileHandle->iDirectoryEntryOffset, &stEntry) == FALSE)
	{
		return FALSE;
	}


	return TRUE;
}


// Write data in Buffer to File
DWORD kWriteFile(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile)
{
	DWORD dwWriteCount;
	DWORD dwTotalCount;
	DWORD dwOffsetInCluster;
	DWORD dwCopySize;
	DWORD dwAllocatedClusterIndex;
	DWORD dwNextClusterIndex;
	FILEHANDLE* pstFileHandle;

	
	// if Handle is not File Type, fail
	if( (pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE) )
	{
		return 0;
	}

	pstFileHandle = &(pstFile->stFileHandle);

	
	// Total Byte Count
	dwTotalCount = dwSize * dwCount;


	////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////

	dwWriteCount = 0;

	while(dwWriteCount != dwTotalCount)
	{
		// =========================================================
		// if Current Cluser is EOF, Allocate Cluster and connect
		// =========================================================

		if(pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER)
		{
			// Search Free Cluster
			dwAllocatedClusterIndex = kFindFreeCluster();
			
			if(dwAllocatedClusterIndex == FILESYSTEM_LASTCLUSTER)
			{
				break;
			}

			// set Found Cluster to FILESYSTEM_LASTCLUSTER
			if(kSetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_LASTCLUSTER) == FALSE)
			{
				break;
			}

			// connect Allocated Cluster with Last Cluster
			if(kSetClusterLinkData(pstFileHandle->dwPreviousClusterIndex, dwAllocatedClusterIndex) == FALSE)
			{
				kSetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_FREECLUSTER);
				break;
			}
			
			
			// Current Cluster Index = Allocated Cluster Index
			pstFileHandle->dwCurrentClusterIndex = dwAllocatedClusterIndex;

			// Fill Temporary Buffer with 0
			kMemSet(gs_vbTempBuffer, 0, sizeof(gs_vbTempBuffer));
		}

		// ====================================================================================
		// if Filling one Cluster not completed, Read Cluster then Copy to Temporary Buffer
		// ====================================================================================
		
		else if( ((pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) != 0) || 
				 ((dwTotalCount - dwWriteCount) < FILESYSTEM_CLUSTERSIZE) )
		{
			if(kReadCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE)
			{
				break;
			}
		}

		// Calculate File Pointer Offset in Cluster
		dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;

		// if there's several Cluster to write, 
		// Write remaining data on Current Cluster, and then move next
		// 		Size to copy = MIN( Max Size to copy in Cluster, Bytes to write )
		dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwWriteCount);

		kMemCpy(gs_vbTempBuffer + dwOffsetInCluster, (char*)pvBuffer + dwWriteCount, dwCopySize);

		// Write Temporary Buffer on Disk
		if(kWriteCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE)
		{
			break;
		}
		
		// Update Write Count, File Pointer Position
		dwWriteCount += dwCopySize;
		pstFileHandle->dwCurrentOffset += dwCopySize;


		// ================================================================
		// if Filling current Cluster completed, move to Next Cluster
		// ================================================================

		if( (pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0 )
		{
			if(kGetClusterLinkData(pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE)
			{
				break;
			}
			
			pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
			pstFileHandle->dwCurrentClusterIndex  = dwNextClusterIndex;
		}

	}	// Write File contents loop end

	
	// =========================================================
	// if File Size changed, update Directory Entry Info
	// =========================================================
	if(pstFileHandle->dwFileSize < pstFileHandle->dwCurrentOffset)
	{
		pstFileHandle->dwFileSize = pstFileHandle->dwCurrentOffset;
		kUpdateDirectoryEntry(pstFileHandle);
	}


	////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////
	
	return dwWriteCount;
}


// Fill 0 while dwCount
BOOL kWriteZero(FILE* pstFile, DWORD dwCount)
{
	BYTE* pbBuffer;
	DWORD dwRemainCount;
	DWORD dwWriteCount;


	// failed when Handle is NULL
	if(pstFile == NULL)
	{
		return FALSE;
	}

	pbBuffer = (BYTE*)kAllocateMemory(FILESYSTEM_CLUSTERSIZE);

	if(pbBuffer == NULL)
	{
		return FALSE;
	}

	// fill with 0
	kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);
	dwRemainCount = dwCount;

	// Write 0 in Cluster by Cluster
	while(dwRemainCount != 0)
	{
		dwWriteCount = MIN(dwRemainCount, FILESYSTEM_CLUSTERSIZE);
		
		if(kWriteFile(pbBuffer, 1, dwWriteCount, pstFile) != dwWriteCount)
		{
			kFreeMemory(pbBuffer);
			return FALSE;
		}

		dwRemainCount -= dwWriteCount;
	}

	kFreeMemory(pbBuffer);
	return TRUE;
}


// Move File Pointer
int kSeekFile(FILE* pstFile, int iOffset, int iOrigin)
{
	DWORD dwRealOffset;
	DWORD dwClusterOffsetToMove;
	DWORD dwCurrentClusterOffset;
	DWORD dwLastClusterOffset;
	DWORD dwMoveCount;
	DWORD i;
	DWORD dwStartClusterIndex;
	DWORD dwPreviousClusterIndex;
	DWORD dwCurrentClusterIndex;
	FILEHANDLE* pstFileHandle;

	
	if( (pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE) )
	{
		return 0;	// exit, not fail
	}

	pstFileHandle = &(pstFile->stFileHandle);


	// ============================================================
	// Calculate Position to move by combining Origin, Offset
	// ============================================================
	
	switch(iOrigin)
	{
		// based on BOF
		case FILESYSTEM_SEEK_SET:
			if(iOffset <= 0)
			{
				dwRealOffset = 0;
			}
			else
			{
				dwRealOffset = iOffset;
			}

			break;
			
		// based on Current Pos
		case FILESYSTEM_SEEK_CUR:
			if( (iOffset < 0) && (pstFileHandle->dwCurrentOffset <= (DWORD)-iOffset) )
			{
				dwRealOffset = 0;
			}
			else
			{
				dwRealOffset = pstFileHandle->dwCurrentOffset + iOffset;
			}

			break;

		// based on EOF
		case FILESYSTEM_SEEK_END:
			if( (iOffset < 0) && (pstFileHandle->dwFileSize <= (DWORD)-iOffset) )
			{
				dwRealOffset = 0;
			}
			else
			{
				dwRealOffset = pstFileHandle->dwFileSize + iOffset;
			}

			break;
	}	// switch end

	
	// ==================================================================================
	// Search Cluster Link considering Total Cluster Count, Current File Pointer Pos
	// ==================================================================================
	
	// Last Cluster Offset
	dwLastClusterOffset    = pstFileHandle->dwFileSize / FILESYSTEM_CLUSTERSIZE;
	
	// Cluster Offset to move
	dwClusterOffsetToMove  = dwRealOffset / FILESYSTEM_CLUSTERSIZE;
	
	// Current Cluster Offset
	dwCurrentClusterOffset = pstFileHandle->dwCurrentOffset / FILESYSTEM_CLUSTERSIZE;

	
	// if the Cluster Offset to move is over Last Cluster Offset...
	// Move to Last Cluster, then fill remainings with 0 by calling fwrite()
	if(dwLastClusterOffset <= dwClusterOffsetToMove)
	{
		dwMoveCount = dwLastClusterOffset - dwCurrentClusterOffset;
		dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
	}
	// if the Cluster Offset to move is same with Current Cluster Offset or next to it...
	else if(dwCurrentClusterOffset <= dwClusterOffsetToMove)
	{
		dwMoveCount = dwClusterOffsetToMove - dwCurrentClusterOffset;
		dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
	}
	// if the Cluster Offset to move is previous to Current Cluster...
	else
	{
		dwMoveCount = dwClusterOffsetToMove;
		dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;
	}


	////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////

	// Move Cluster
	dwCurrentClusterIndex = dwStartClusterIndex;

	for(i=0; i<dwMoveCount; i++)
	{
		dwPreviousClusterIndex = dwCurrentClusterIndex;

		if(kGetClusterLinkData(dwPreviousClusterIndex, &dwCurrentClusterIndex) == FALSE)
		{
			// CRITICAL SECTION END
			kUnlock(&(gs_stFileSystemManager.stMutex));

			return -1;
		}
	}


	// Update Cluster Info
	if(dwMoveCount > 0)
	{
		pstFileHandle->dwPreviousClusterIndex = dwPreviousClusterIndex;
		pstFileHandle->dwCurrentClusterIndex  = dwCurrentClusterIndex;
	}
	// if the case moving to First Cluster...
	else if(dwStartClusterIndex == pstFileHandle->dwStartClusterIndex)
	{
		pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwStartClusterIndex;
		pstFileHandle->dwCurrentClusterIndex  = pstFileHandle->dwStartClusterIndex;
	}
	
	
	// ======================================================================
	// Update File Pointer.
	// if File Offset is over File Size, Fill newly added size with 0
	// ======================================================================
	
	if(dwLastClusterOffset < dwClusterOffsetToMove)
	{
		pstFileHandle->dwCurrentOffset = pstFileHandle->dwFileSize;

		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));

		if(kWriteZero(pstFile, dwRealOffset - pstFileHandle->dwFileSize) == FALSE)
		{
			return 0;
		}
	}

	pstFileHandle->dwCurrentOffset = dwRealOffset;


	////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////

	return 0;
}


// Close File
int kCloseFile(FILE* pstFile)
{
	if( (pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE) )
	{
		return -1;
	}

	kFreeFileDirectoryHandle(pstFile);

	return 0;
}


// Check if File opened by verifying Handle Pool
BOOL kIsFileOpened(const DIRECTORYENTRY* pstEntry)
{
	int i;
	FILE* pstFile;

	
	pstFile = gs_stFileSystemManager.pstHandlePool;
	
	// Linear search 
	for(i=0; i<FILESYSTEM_HANDLE_MAXCOUNT; i++)
	{
		if( (pstFile[i].bType == FILESYSTEM_TYPE_FILE) && 
			(pstFile[i].stFileHandle.dwStartClusterIndex == pstEntry->dwStartClusterIndex) )
		{
			return TRUE;
		}
	}

	return FALSE;
}


// Remove File
int kRemoveFile(const char* pcFileName)
{
	DIRECTORYENTRY stEntry;
	int iDirectoryEntryOffset;
	int iFileNameLength;


	// Check File Name
	iFileNameLength = kStrLen(pcFileName);

	if( (iFileNameLength > (sizeof(stEntry.vcFileName) - 1)) || (iFileNameLength == 0) )
	{
		return -1;
	}


	////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////

	// Check if File exists
	iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);

	if(iDirectoryEntryOffset == -1)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));
		return -1;
	}


	// if File opened, Cannot remove
	if(kIsFileOpened(&stEntry) == TRUE)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));
		return -1;	
	}


	// Free All Cluster constituting File
	if(kFreeClusterUntilEnd(stEntry.dwStartClusterIndex) == FALSE)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));
		return -1;
	}
	

	// Set Directory Entry as Empty
	kMemSet(&stEntry, 0, sizeof(stEntry));

	if(kSetDirectoryEntryData(iDirectoryEntryOffset, &stEntry) == FALSE)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));
		return -1;
	}


	////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////
	
	return 0;
}


// Open Directory
DIR* kOpenDirectory(const char* pcDirectoryName)
{
	DIR* pstDirectory;
	DIRECTORYENTRY* pstDirectoryBuffer;


	////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////
	
	pstDirectory = kAllocateFileDirectoryHandle();

	if(pstDirectory == NULL)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));
		return NULL;
	}


	// Allocate Buffer for saving Root Directory
	pstDirectoryBuffer = (DIRECTORYENTRY*)kAllocateMemory(FILESYSTEM_CLUSTERSIZE);

	if(pstDirectoryBuffer == NULL)
	{
		kFreeFileDirectoryHandle(pstDirectory);
		
		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));

		return NULL;
	}

	
	// Read Root Directory
	if(kReadCluster(0, (BYTE*)pstDirectoryBuffer) == FALSE)
	{
		kFreeFileDirectoryHandle(pstDirectory);
		kFreeMemory(pstDirectoryBuffer);

		// CRITICAL SECTION END
		kUnlock(&(gs_stFileSystemManager.stMutex));

		return NULL;
	}

	
	pstDirectory->bType = FILESYSTEM_TYPE_DIRECTORY;
	pstDirectory->stDirectoryHandle.iCurrentOffset = 0;
	pstDirectory->stDirectoryHandle.pstDirectoryBuffer = pstDirectoryBuffer;

	
	////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////
	
	return pstDirectory;
}


// Return Directory Entry, then move next
struct kDirectoryEntryStruct* kReadDirectory(DIR* pstDirectory)
{
	DIRECTORYHANDLE* pstDirectoryHandle;
	DIRECTORYENTRY*  pstEntry;

	
	// if Handle Type is not DIR, fail
	if( (pstDirectory == NULL) || (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY) )
	{
		return NULL;
	}

	pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);


	if( (pstDirectoryHandle->iCurrentOffset < 0) ||
	 	(pstDirectoryHandle->iCurrentOffset >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT) )
	{
		return NULL;
	}


	////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////


	pstEntry = pstDirectoryHandle->pstDirectoryBuffer;

	while(pstDirectoryHandle->iCurrentOffset < FILESYSTEM_MAXDIRECTORYENTRYCOUNT)
	{
		// if File exists, return found Directory Entry
		if(pstEntry[pstDirectoryHandle->iCurrentOffset].dwStartClusterIndex != 0)
		{
			// CRITICAL SECTION END
			kUnlock(&(gs_stFileSystemManager.stMutex));

			return &(pstEntry[pstDirectoryHandle->iCurrentOffset++]);
		}

		pstDirectoryHandle->iCurrentOffset++;
	}


	////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////

	return NULL;
}


// Move Directory Pointer to Beginning
void kRewindDirectory(DIR* pstDirectory)
{
	DIRECTORYHANDLE* pstDirectoryHandle;

	
	if( (pstDirectory == NULL) ||
		(pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY) )
	{
		return;
	}

	pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);


	////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////

	pstDirectoryHandle->iCurrentOffset = 0;

	////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////
}


// Close opened Directory
int kCloseDirectory(DIR* pstDirectory)
{
	DIRECTORYHANDLE* pstDirectoryHandle;

	
	if( (pstDirectory == NULL) ||
		(pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY) )
	{
		return -1;
	}

	pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);


	////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////
	
	kFreeMemory(pstDirectoryHandle->pstDirectoryBuffer);
	kFreeFileDirectoryHandle(pstDirectory);

	////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	////////////////////////////////////
	
	return 0;
}


// Write File System Cache to HDD
BOOL kFlushFileSystemCache(void)
{
	CACHEBUFFER* pstCacheBuffer;
	int iCacheCount;
	int i;

	
	// if Cache deactivated, No need to execute this function
	if(gs_stFileSystemManager.bCacheEnable == FALSE)
	{
		return FALSE;
	}


	///////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stFileSystemManager.stMutex));
	///////////////////////////////
	

	// 1. Get Cache info of Cluster Link Table Area, 
	// 		and Write modified Cache Buffer to HDD
	kGetCacheBufferAndCount(CACHE_CLUSTERLINKTABLEAREA, &pstCacheBuffer, &iCacheCount);

	for(i=0; i<iCacheCount; i++)
	{
		// if Cache modified, Write modified contents to dwTag directly
		if(pstCacheBuffer[i].bChanged == TRUE)
		{
			if(kInternalWriteClusterLinkTableWithoutCache(
					pstCacheBuffer[i].dwTag, pstCacheBuffer[i].pbBuffer) == FALSE)
			{
				return FALSE;
			}

			// HDD contents is up to date now, so, mark it as not modified
			pstCacheBuffer[i].bChanged = FALSE;
		}
	}

	
	// 2. Get Cache info of Data Area,
	// 		and Write modified Cache Buffer to HDD
	kGetCacheBufferAndCount(CACHE_DATAAREA, &pstCacheBuffer, &iCacheCount);

	for(i=0; i<iCacheCount; i++)
	{
		// if Cache modified, Write modified contents to dwTag directly
		if(pstCacheBuffer[i].bChanged == TRUE)
		{
			if(kInternalWriteClusterWithoutCache(
					pstCacheBuffer[i].dwTag, pstCacheBuffer[i].pbBuffer) == FALSE)
			{
				return FALSE;
			}

			// HDD contents is up to date now, so, mark it as not modified
			pstCacheBuffer[i].bChanged = FALSE;
		}
	}

	
	///////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stFileSystemManager.stMutex));
	///////////////////////////////


	return TRUE;
}



