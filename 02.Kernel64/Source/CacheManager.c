#include "CacheManager.h"
#include "FileSystem.h"
#include "DynamicMemory.h"


// File System Cache Data Structure
static CACHEMANAGER gs_stCacheManager;


BOOL kInitializeCacheManager(void)
{
	int i;


	// init Data Structure
	kMemSet(&gs_stCacheManager, 0, sizeof(gs_stCacheManager));


	// init Access time
	gs_stCacheManager.vdwAccessTime[CACHE_CLUSTERLINKTABLEAREA] = 0;
	gs_stCacheManager.vdwAccessTime[CACHE_DATAAREA]				= 0;
	

	// set Max value of Cache Buffer
	gs_stCacheManager.vdwMaxCount[CACHE_CLUSTERLINKTABLEAREA] = CACHE_MAXCLUSTERLINKTABLEAREACOUNT;
	gs_stCacheManager.vdwMaxCount[CACHE_DATAAREA]			  = CACHE_MAXDATAAREACOUNT;


	// ******************************************************
	// allocate memory for Cluster Link Table Area
	// 		Cluster Link Table managed by 512 byte (Sector)
	// ******************************************************
	gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] = 
		(BYTE*)kAllocateMemory(CACHE_MAXCLUSTERLINKTABLEAREACOUNT * 512);
	
	if(gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] == NULL)
	{
		return FALSE;
	}

	// divide allocated memory and then register to Cache Buffer
	for(i=0; i<CACHE_MAXCLUSTERLINKTABLEAREACOUNT; i++)
	{
		// allocate memory to Cache Buffer
		gs_stCacheManager.vvstCacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].pbBuffer =
			gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA] + (i * 512);

		// set Tag as invalid
		gs_stCacheManager.vvstCacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].dwTag =
			CACHE_INVALIDTAG;
	}


	// ******************************************************
	// allocate memory for Data Area
	// 		Data Area managed by 4 KB (Cluster)
	// ******************************************************
	gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] =
		(BYTE*)kAllocateMemory(CACHE_MAXDATAAREACOUNT * FILESYSTEM_CLUSTERSIZE);

	if(gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] == NULL)
	{
		kFreeMemory(gs_stCacheManager.vpbBuffer[CACHE_CLUSTERLINKTABLEAREA]);
		return FALSE;
	}
	
	// divide allocated memory and then register to Cache Buffer
	for(i=0; i<CACHE_MAXDATAAREACOUNT; i++)
	{
		// allocate memory to Cache Buffer
		gs_stCacheManager.vvstCacheBuffer[CACHE_DATAAREA][i].pbBuffer =
			gs_stCacheManager.vpbBuffer[CACHE_DATAAREA] + (i * FILESYSTEM_CLUSTERSIZE);

		// set Tag as invalid
		gs_stCacheManager.vvstCacheBuffer[CACHE_DATAAREA][i].dwTag =
			CACHE_INVALIDTAG;
	}


	return TRUE;
}


CACHEBUFFER* kAllocateCacheBuffer(int iCacheTableIndex)
{
	CACHEBUFFER* pstCacheBuffer;
	int i;


	// Check if valid iCacheTableIndex
	if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX)
	{
		return FALSE;
	}

	// if Access Time field reaches Max value, Cut down All Access Time
	kCutDownAccessTime(iCacheTableIndex);


	// Search empty Cache Buffer - linear search
	pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
	
	for(i=0; i<gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++)
	{
		if(pstCacheBuffer[i].dwTag == CACHE_INVALIDTAG)
		{
			// set as allocated temporarily
			pstCacheBuffer[i].dwTag = CACHE_INVALIDTAG - 1;
			
			// update Access Time
			pstCacheBuffer[i].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;


			return &(pstCacheBuffer[i]);
		}
	}


	return NULL;
}


// Return Cache Buffer which has same Tag with dwTag
CACHEBUFFER* kFindCacheBuffer(int iCacheTableIndex, DWORD dwTag)
{
	CACHEBUFFER* pstCacheBuffer;
	int i;

	
	// is iCacheTableIndex vaild?
	if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX)
	{
		return FALSE;
	}

	// if Access Time reaches Max value, Cut down All Access Time
	kCutDownAccessTime(iCacheTableIndex);


	// Search empty Cache Buffer - linear search
	pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];

	for(i=0; i<gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++)
	{
		if(pstCacheBuffer[i].dwTag == dwTag)
		{
			// update Access Time
			pstCacheBuffer[i].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;

			return &(pstCacheBuffer[i]);
		}
	}

	
	return NULL;
}


static void kCutDownAccessTime(int iCacheTableIndex)
{
	CACHEBUFFER stTemp;
	CACHEBUFFER* pstCacheBuffer;
	BOOL bSorted;
	int i, j;


	if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX)
	{
		return;
	}

	// if Access time under Max value yet, No need to cut down
	if(gs_stCacheManager.vdwAccessTime[iCacheTableIndex] < 0xFFFFFFFE)
	{
		return;
	}

	
	// Sort Cache Buffer with ascending-ordering
	// 		Sorting Algorithm : Bubble Sort
	pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];

	for(j=0; j<gs_stCacheManager.vdwMaxCount[iCacheTableIndex] - 1; j++)
	{
		// default : sorted
		bSorted = TRUE;

		for(i=0; i<gs_stCacheManager.vdwMaxCount[iCacheTableIndex] - 1 - j; i++)
		{
			// compare with near Data
			// 		locate bigger one on right
			if(pstCacheBuffer[i].dwAccessTime > pstCacheBuffer[i+1].dwAccessTime)
			{
				bSorted = FALSE;

				// change Cache i with Cache i+1
				kMemCpy(&stTemp, &(pstCacheBuffer[i]), sizeof(CACHEBUFFER));
				kMemCpy(&(pstCacheBuffer[i]), &(pstCacheBuffer[i+1]), sizeof(CACHEBUFFER));
				kMemCpy(&(pstCacheBuffer[i+1]), &stTemp, sizeof(CACHEBUFFER));
			}
		}

		// if sorted, break
		if(bSorted == TRUE)
		{
			break;
		}
	}

	
	// Sorting complete, so, bigger Index means more recent Cache
	for(i=0; i<gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++)
	{
		pstCacheBuffer[i].dwAccessTime = i;
	}

	gs_stCacheManager.vdwAccessTime[iCacheTableIndex] = i;
}


CACHEBUFFER* kGetVictimInCacheBuffer(int iCacheTableIndex)
{
	DWORD dwOldTime;
	CACHEBUFFER* pstCacheBuffer;
	int iOldIndex;
	int i;

	if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX)
	{
		return FALSE;
	}

	
	// Search oldest Cache Buffer - LRU (Least Recently Used)
	iOldIndex = -1;
	dwOldTime = 0xFFFFFFFF;


	// Return Cache Buffer that oldest or not in use
	pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];

	for(i=0; i<gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++)
	{
		// if invalid one found, return
		if(pstCacheBuffer[i].dwTag == CACHE_INVALIDTAG)
		{
			iOldIndex = i;
			break;
		}

		// if Access Time is older than dwOldTime, save
		if(pstCacheBuffer[i].dwAccessTime < dwOldTime)
		{
			dwOldTime = pstCacheBuffer[i].dwAccessTime;
			iOldIndex = i;
		}
	}

	
	// if failed to find Cache Buffer, problem occured
	if(iOldIndex == -1)
	{
		kPrintf("Cache Buffer Find Error\n");

		return NULL;
	}


	// update Access Time of selected Buffer
	pstCacheBuffer[iOldIndex].dwAccessTime = gs_stCacheManager.vdwAccessTime[iCacheTableIndex]++;

	
	return &(pstCacheBuffer[iOldIndex]);
}


void kDiscardAllCacheBuffer(int iCacheTableIndex)
{
	CACHEBUFFER* pstCacheBuffer;
	int i;


	// set Cache Buffer empty
	pstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];

	for(i=0; i<gs_stCacheManager.vdwMaxCount[iCacheTableIndex]; i++)
	{
		pstCacheBuffer[i].dwTag = CACHE_INVALIDTAG;
	}

	// init Access Time
	gs_stCacheManager.vdwAccessTime[iCacheTableIndex] = 0;
}


BOOL kGetCacheBufferAndCount(int iCacheTableIndex, CACHEBUFFER** ppstCacheBuffer, int* piMaxCount)
{
	if(iCacheTableIndex > CACHE_MAXCACHETABLEINDEX)
	{
		return FALSE;
	}

	
	// return Cache Buffer Pointer, Cache Buffer Max Count
	*ppstCacheBuffer = gs_stCacheManager.vvstCacheBuffer[iCacheTableIndex];
	*piMaxCount		 = gs_stCacheManager.vdwMaxCount[iCacheTableIndex];


	return TRUE;
}



