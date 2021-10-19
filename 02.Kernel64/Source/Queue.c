#include "Queue.h"


// initialize Queue
void kInitializeQueue(QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize)
{
	// save Max data count, Size, Buffer address of Queue
	pstQueue->iMaxDataCount = iMaxDataCount;
	pstQueue->iDataSize 	= iDataSize;
	pstQueue->pvQueueArray	= pvQueueBuffer;
	
	// initialize Queue Index, set Last Operation to FALSE (delete)
	pstQueue->iPutIndex		= 0;
	pstQueue->iGetIndex		= 0;
	pstQueue->bLastOperationPut = FALSE;
}


// check if Queue is full
BOOL kIsQueueFull(const QUEUE* pstQueue)
{
	if( (pstQueue->iPutIndex == pstQueue->iGetIndex) && (pstQueue->bLastOperationPut == TRUE) )
		return TRUE;
	
	return FALSE;
}


// check if Queue is empty
BOOL kIsQueueEmpty(const QUEUE* pstQueue)
{
	if( (pstQueue->iPutIndex == pstQueue->iGetIndex) && (pstQueue->bLastOperationPut == FALSE) )
		return TRUE;

	return FALSE;
}


// put data into Queue
BOOL kPutQueue(QUEUE* pstQueue, const void* pvData)
{
	// if Queue is full, insertion is failed
	if(kIsQueueFull(pstQueue) == TRUE)
		return FALSE;

	// copy pvData to location pointed by PutIndex
	kMemCpy((char*)pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iPutIndex), pvData, pstQueue->iDataSize);

	// modify PutIndex, LastOperationPut
	pstQueue->iPutIndex = (pstQueue->iPutIndex + 1) % pstQueue->iMaxDataCount;
	pstQueue->bLastOperationPut = TRUE;

	return TRUE;
}


// get data from Queue
BOOL kGetQueue(QUEUE* pstQueue, void* pvData)
{
	if(kIsQueueEmpty(pstQueue) == TRUE)
		return FALSE;

	kMemCpy(pvData, (char*)pstQueue->pvQueueArray + (pstQueue->iDataSize * pstQueue->iGetIndex), pstQueue->iDataSize);

	pstQueue->iGetIndex = (pstQueue->iGetIndex + 1) % pstQueue->iMaxDataCount;
	pstQueue->bLastOperationPut = FALSE;

	return TRUE;
}


