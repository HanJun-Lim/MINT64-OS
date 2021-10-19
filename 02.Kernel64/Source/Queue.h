#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "Types.h"

/////////////////////////////////////////
// Structure
/////////////////////////////////////////

#pragma pack(push, 1)

typedef struct kQueueManagerStruct
{
	// Data size, Max data count
	int iDataSize;
	int iMaxDataCount;

	// Pointer of Queue buffer, Index of Insertion/Deletion
	void* pvQueueArray;
	int	  iPutIndex;
	int	  iGetIndex;

	// Last Operation : insert or delete
	BOOL bLastOperationPut;
} QUEUE;

#pragma pack(pop)



/////////////////////////////////////////
// Function
/////////////////////////////////////////

void kInitializeQueue(QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize);
BOOL kIsQueueFull(const QUEUE* pstQueue);
BOOL kIsQueueEmpty(const QUEUE* pstQueue);
BOOL kPutQueue(QUEUE* pstQueue, const void* pvData);
BOOL kGetQueue(QUEUE* pstQueue, void* pvData);


#endif
