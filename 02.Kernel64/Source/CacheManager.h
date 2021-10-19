#ifndef __CACHEMANAGER_H__
#define __CACHEMANAGER_H__

#include "Types.h"


// =========================================
// Macro
// =========================================

// Max Cache Buffer Count

#define CACHE_MAXCLUSTERLINKTABLEAREACOUNT		16
#define CACHE_MAXDATAAREACOUNT					32			

// Invalid Tag / Empty Cache Buffer

#define CACHE_INVALIDTAG						0xFFFFFFFF

// Cache Table Count

#define CACHE_MAXCACHETABLEINDEX				2

// Index of each area

#define CACHE_CLUSTERLINKTABLEAREA				0
#define CACHE_DATAAREA							1



// =========================================
// Structure
// =========================================

typedef struct kCacheBufferStruct
{
	// Index of each area
	DWORD dwTag;

	// Access time to Cache Buffer
	DWORD dwAccessTime;

	// is Data changed? (Dirty)
	BOOL bChanged;

	// Data Buffer (Data is stored here)
	BYTE* pbBuffer;
} CACHEBUFFER;


typedef struct kCacheManagerStruct
{
	// Access time field of Cluster Link Area / Data Area
	DWORD vdwAccessTime[CACHE_MAXCACHETABLEINDEX];

	// Data Buffer of Cluster Link Area / Data Area
	BYTE* vpbBuffer[CACHE_MAXCACHETABLEINDEX];

	// Cache Buffer of Cluster Link Area / Data Area
	// 		MAX(Cluster Link Area Cache Count, Data Area Cache Count)
	CACHEBUFFER vvstCacheBuffer[CACHE_MAXCACHETABLEINDEX][CACHE_MAXDATAAREACOUNT];

	// Max value of Cache Buffer
	DWORD vdwMaxCount[CACHE_MAXCACHETABLEINDEX];
} CACHEMANAGER;



// =========================================
// Function
// =========================================

BOOL kInitializeCacheManger(void);
CACHEBUFFER* kAllocateCacheBuffer(int iCacheTableIndex);
CACHEBUFFER* kFindCacheBuffer(int iCacheTableIndex, DWORD dwTag);
CACHEBUFFER* kGetVictimInCacheBuffer(int iCacheTableIndex);
void kDiscardAllCacheBuffer(int iCacheTableIndex);
BOOL kGetCacheBufferAndCount(int iCacheTableIndex, CACHEBUFFER** ppstCacheBuffer, int* piMaxCount);

static void kCutDownAccessTime(int iCacheTableIndex);



#endif
