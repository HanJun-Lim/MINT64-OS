#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"


#if 0
// ---------- for Data used by Overall System ----------

BOOL kLockForSystemData(void)
{
	return kSetInterruptFlag(FALSE);
}


void kUnlockForSystemData(BOOL bInterruptFlag)
{
	kSetInterruptFlag(bInterruptFlag);
}
#endif



void kInitializeMutex(MUTEX* pstMutex)
{
	pstMutex->bLockFlag 	= FALSE;
	pstMutex->dwLockCount 	= 0;
	pstMutex->qwTaskID 		= TASK_INVALIDID;
}



// ---------- for Data used by Tasks ----------

void kLock(MUTEX* pstMutex)
{
	BYTE bCurrentAPICID;
	BOOL bInterruptFlag;


	// =============== Disable Interrupt ===============	
	// why added? : Local APIC ID of Task may be able to be changed by Task Load Balancing (PIT Interrupt)	
	bInterruptFlag = kSetInterruptFlag(FALSE);
	// =================================================
	

	// get Local APIC ID of current Core
	bCurrentAPICID = kGetAPICID();


	// if locked, check if locked by itself, increase Lock Count
	if(kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE)		
	{
		// if locked by myself, only increase Lock Count
		if(pstMutex->qwTaskID == kGetRunningTask(bCurrentAPICID)->stLink.qwID)
		{
			// restore Interrupt
			kSetInterruptFlag(bInterruptFlag);

			pstMutex->dwLockCount++;
			return;
		}

		// if locked by other, wait until unlocked
		while(kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE)
		{
			kSchedule();
		}
	}

	
	// set as Locked, Lock Flag handled by kTestAndSet() above
	pstMutex->dwLockCount = 1;
	pstMutex->qwTaskID = kGetRunningTask(bCurrentAPICID)->stLink.qwID;


	// =============== Restore Interrupt ===============
	kSetInterruptFlag(bInterruptFlag);
	// =================================================
}


void kUnlock(MUTEX* pstMutex)
{
	BOOL bInterruptFlag;


	// =============== Disable Interrupt ===============
	bInterruptFlag = kSetInterruptFlag(FALSE);
	// =================================================


	// Fail if not the Task that locked the Mutex
	if( (pstMutex->bLockFlag == FALSE) || 
		(pstMutex->qwTaskID != kGetRunningTask(kGetAPICID())->stLink.qwID) )
	{
		// restore Interrupt
		kSetInterruptFlag(bInterruptFlag);

		return;
	}


	// if Mutex locked twice, decrease Lock Count
	if(pstMutex->dwLockCount > 1)
	{
		pstMutex->dwLockCount--;
	}
	else
	{
		pstMutex->qwTaskID = TASK_INVALIDID;
		pstMutex->dwLockCount = 0;
		pstMutex->bLockFlag = FALSE;		// must be released most later
	}


	// =============== Restore Interrupt ===============
	kSetInterruptFlag(bInterruptFlag);
	// =================================================
}



// Initialize SpinLock

void kInitializeSpinLock(SPINLOCK* pstSpinLock)
{
	pstSpinLock->bLockFlag	 	= FALSE;
	pstSpinLock->dwLockCount 	= 0;
	pstSpinLock->bAPICID 	 	= 0xFF;
	pstSpinLock->bInterruptFlag = FALSE;
}


// ---------- for Data used by Overall System ----------

void kLockForSpinLock(SPINLOCK* pstSpinLock)
{
	BOOL bInterruptFlag;

	
	// disable Interrupt first
	bInterruptFlag = kSetInterruptFlag(FALSE);


	// if locked already, check if locked myself
	if(kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE)
	{
		// if locked myself, increase lock count
		if(pstSpinLock->bAPICID == kGetAPICID())
		{
			pstSpinLock->dwLockCount++;
			return;
		}

		// if locked by other, wait for release of lock
		while(kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE)
		{
			// call kTestAndSet() continuously for preventing Memory Bus from being locked
			while(pstSpinLock->bLockFlag == TRUE)
			{
				kPause();
			}
		}
	}


	// set as locked, Lock Flag handled by kTestAndSet() above
	pstSpinLock->dwLockCount = 1;
	pstSpinLock->bAPICID	 = kGetAPICID();

	// save Interrupt Flag and restore when executing Unlock
	pstSpinLock->bInterruptFlag = bInterruptFlag;
}


void kUnlockForSpinLock(SPINLOCK* pstSpinLock)
{
	BOOL bInterruptFlag;


	// disable Interrupt first
	bInterruptFlag = kSetInterruptFlag(FALSE);


	// if not Task locking SpinLock, fail
	if( (pstSpinLock->bLockFlag == FALSE) || (pstSpinLock->bAPICID != kGetAPICID()) )
	{
		kSetInterruptFlag(bInterruptFlag);
		return;
	}

	
	// if SpinLock locked more than twice, decrease Lock Count
	if(pstSpinLock->dwLockCount > 1)
	{
		pstSpinLock->dwLockCount--;
		return;
	}


	// set SpinLock as released, restore Interrupt Flag
	bInterruptFlag = pstSpinLock->bInterruptFlag;

	pstSpinLock->bAPICID = 0xFF;
	pstSpinLock->dwLockCount = 0;
	pstSpinLock->bInterruptFlag = FALSE;
	pstSpinLock->bLockFlag = FALSE;

	
	kSetInterruptFlag(bInterruptFlag);
}



