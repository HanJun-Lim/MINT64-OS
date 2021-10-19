#ifndef __RAMDISK_H__
#define __RAMDISK_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"


// -----------------------------
// Macro
// -----------------------------

#define RDD_TOTALSECTORCOUNT		(8 * 1024 * 1024 / 512)



// -----------------------------
// Structure
// -----------------------------

#pragma pack(push, 1)


typedef struct kRDDManagerStruct
{
	// Allocated memory for RAM Disk
	BYTE* pbBuffer;

	// Total Sector Count
	DWORD dwTotalSectorCount;

	// Sync Object
	MUTEX stMutex;
} RDDMANAGER;


#pragma pack(pop)



// -----------------------------
// Function
// -----------------------------

BOOL kInitializeRDD(DWORD dwTotalSectorCount);
BOOL kReadRDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
int  kReadRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
int  kWriteRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);



#endif
