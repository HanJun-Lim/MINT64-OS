#include "RAMDisk.h"
#include "Utility.h"
#include "DynamicMemory.h"


static RDDMANAGER gs_stRDDManager;


BOOL kInitializeRDD(DWORD dwTotalSectorCount)
{
	// init Data Structure
	kMemSet(&gs_stRDDManager, 0, sizeof(gs_stRDDManager));


	// allocate memory used for RAM Disk
	gs_stRDDManager.pbBuffer = (BYTE*)kAllocateMemory(dwTotalSectorCount * 512);
	
	if(gs_stRDDManager.pbBuffer == NULL)
	{
		return FALSE;
	}


	// set Total Sector Count, Sync Object
	gs_stRDDManager.dwTotalSectorCount = dwTotalSectorCount;

	kInitializeMutex(&(gs_stRDDManager.stMutex));

	
	return TRUE;
}


BOOL kReadRDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation)
{
	// init Data Structure
	kMemSet(pstHDDInformation, 0, sizeof(HDDINFORMATION));

	
	// set only Total Sector Count, Serial Number, Model Number
	pstHDDInformation->dwTotalSectors = gs_stRDDManager.dwTotalSectorCount;
	kMemCpy(pstHDDInformation->vwSerialNumber, "0000-0000", 9);
	kMemCpy(pstHDDInformation->vwModelNumber,  "MINT RAM Disk v1.0", 18);


	return TRUE;
}


int kReadRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer)
{
	int iRealReadCount;

	// Readable Count = MIN(End of RAM Disk, Sector Count to read)
	iRealReadCount = MIN(gs_stRDDManager.dwTotalSectorCount - dwLBA, iSectorCount);


	kMemCpy(pcBuffer, gs_stRDDManager.pbBuffer + (dwLBA * 512), iRealReadCount * 512);


	return iRealReadCount;
}


int kWriteRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer)
{
	int iRealWriteCount;

	// Writable Count = MIN(End of RAM Disk, Sector Count to write)
	iRealWriteCount = MIN(gs_stRDDManager.dwTotalSectorCount - dwLBA, iSectorCount);


	kMemCpy(gs_stRDDManager.pbBuffer + (dwLBA * 512), pcBuffer, iRealWriteCount * 512);


	return iRealWriteCount;
}



