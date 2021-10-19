#include "Task.h"
#include "Descriptor.h"
#include "MultiProcessor.h"
#include "InterruptHandler.h"
#include "DynamicMemory.h"


// Structure for Scheduler
static SCHEDULER gs_vstScheduler[MAXPROCESSORCOUNT];
static TCBPOOLMANAGER gs_stTCBPoolManager;


// =============================================
// for Task Pool, Task
// =============================================

static void kInitializeTCBPool(void)
{
	int i;

	kMemSet(&(gs_stTCBPoolManager), 0, sizeof(gs_stTCBPoolManager));


	// designate and initialize Task Pool Address
	gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
	kMemSet(TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);


	// allocate ID to TCB
	for(i=0; i<TASK_MAXCOUNT; i++)
		gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;


	gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
	gs_stTCBPoolManager.iAllocatedCount = 1;

	
	// Initialize SpinLock
	kInitializeSpinLock(&gs_stTCBPoolManager.stSpinLock);
}


static TCB* kAllocateTCB(void)
{
	TCB* pstEmptyTCB;
	int i;


	////////////////////////////////
	// CRITICAL SECTION START
	kLockForSpinLock(&gs_stTCBPoolManager.stSpinLock);
	////////////////////////////////


	if(gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount)	// if full, unable to allocate
	{
		// CRITICAL SECTION END
		kUnlockForSpinLock(&gs_stTCBPoolManager.stSpinLock);

		return NULL;
	}


	// search empty TCB
	// if upper 32 bits are 0, means not allocated
	for(i=0; i<gs_stTCBPoolManager.iMaxCount; i++)
	{
		if( (gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0 )	// if upper 32 bits are 0, means not allocated
		{
			pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);	// Start Address of Empty TCB stored
			break;
		}
	}


	// set upper 32 bits as Non-zero value for being allocated
	pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount << 32) | i;
	
	gs_stTCBPoolManager.iUseCount++;
	gs_stTCBPoolManager.iAllocatedCount++;

	if(gs_stTCBPoolManager.iAllocatedCount == 0)
	{
		gs_stTCBPoolManager.iAllocatedCount = 1;
	}


	////////////////////////////////
	// CRITICAL SECTION END
	kUnlockForSpinLock(&gs_stTCBPoolManager.stSpinLock);
	////////////////////////////////
	

	return pstEmptyTCB;
}


static void kFreeTCB(QWORD qwID)
{
	int i;


	// qwID - lower 32 bits means index (upper 32 : Allocated Count | lower 32 : TCB Index - Offset)
	i = GETTCBOFFSET(qwID);


	// TCB Initialize, set ID
	kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));	// context init


	////////////////////////////////
	// CRITICAL SECTION START
	kLockForSpinLock(&gs_stTCBPoolManager.stSpinLock);
	////////////////////////////////
	

	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;		// change status as not allocated

	gs_stTCBPoolManager.iUseCount--;


	////////////////////////////////
	// CRITICAL SECTION END
	kUnlockForSpinLock(&gs_stTCBPoolManager.stSpinLock);
	////////////////////////////////
}


// Create Task
// 		Allocate Stack in Stack Pool referring to Task ID
// 		Both Process and Thread can be created
// 		bAffinity field used to set favorite Core to execute
TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress, BYTE bAffinity)
{
	TCB* pstTask, *pstProcess;
	void* pvStackAddress;
	BYTE bCurrentAPICID;


	// check current Core Local APIC ID
	bCurrentAPICID = kGetAPICID();


	// allocate Task Data Structure
	pstTask = kAllocateTCB();

	if(pstTask == NULL)
	{
		return NULL;
	}


	// allocate Stack
	pvStackAddress = kAllocateMemory(TASK_STACKSIZE);

	if(pvStackAddress == NULL)
	{
		kFreeTCB(pstTask->stLink.qwID);

		return NULL;
	}


	// =============== CRITICAL SECTION START ===============
	kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
	// ======================================================
	
	
	// search Process that Current Process or Thread belonged
	pstProcess = kGetProcessByThread(kGetRunningTask(bCurrentAPICID));

	if(pstProcess == NULL)		// if Process not exist, do nothing
	{
		kFreeTCB(pstTask->stLink.qwID);
		kFreeMemory(pvStackAddress);
		
		// CRITICAL SECTION END
		kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

		return NULL;
	}

	// if case of creating Task, Link it with Child Thread List of belonged Process
	if(qwFlags & TASK_FLAGS_THREAD)
	{
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress   = pstProcess->pvMemoryAddress;
		pstTask->qwMemorySize	   = pstProcess->qwMemorySize;

		// To protect Scheduler Link, Add "Thread Link" to List, instead Scheduler Link
		kAddListToTail(&(pstProcess->stChildThreadList), &(pstTask->stThreadLink));
	}	
	// if Process, set value same with Parameter
	else
	{
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress   = pvMemoryAddress;
		pstTask->qwMemorySize	   = qwMemorySize;
	}

	// set Thread ID same with Task ID
	pstTask->stThreadLink.qwID = pstTask->stLink.qwID;


	// =============== CRITICAL SECTION END ===============
	kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
	// ====================================================


	// set TCB, then insert into Ready List to be scheduled
	kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

	
	// init Child Thread List
	kInitializeList(&(pstTask->stChildThreadList));


	// FPU Usage : FALSE
	pstTask->bFPUUsed = FALSE;

		
	// set current working Core APIC ID to Task
	pstTask->bAPICID = bCurrentAPICID;


	// set Processor Affinity
	pstTask->bAffinity = bAffinity;

	
	// add Task to Scheduler regarding Load Balancing
	kAddTaskToSchedulerWithLoadBalancing(pstTask);
	

	return pstTask;
}


static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize)
{
	// initialize CONTEXT
	kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));


	// --------------- Context Setting ---------------
	
	// set RSP, RBP (related to stack)
	pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
	pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;		// below Return Address


	// insert kExitTask() into "Return Address" Area
	// 	to move to kExitTask(), exiting Entry Point Function
	*(QWORD*)((QWORD)pvStackAddress + qwStackSize - 8) = (QWORD)kExitTask;

	// set Segment Selector
	// 		if Kernel Task, set Kernel-level Segment Descriptor
	if( (qwFlags & TASK_FLAGS_USERLEVEL) == 0 )
	{
		pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT | SELECTOR_RPL_0;
		pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
		pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
		pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
		pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
		pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
	}
	//		if User Task, set User-level Segment Descriptor
	else
	{
		pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_USERCODESEGMENT | SELECTOR_RPL_3;
		pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
		pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
		pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
		pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
		pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
	}


	// set RIP, Interrupt Flag
	pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;

	// set RFLAGS Register
	// 		IF bit (bit 9) for enabled Interrupt, IOPL bit (bit 12~13) for accessing I/O port in User-level
	pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x3200;
	
	// -----------------------------------------------
	

	// save Stack, Flag
	pstTCB->pvStackAddress = pvStackAddress;
	pstTCB->qwStackSize = qwStackSize;
	pstTCB->qwFlags = qwFlags;
}



// =============================================
// for Scheduler
// =============================================

void kInitializeScheduler(void)
{
	int i;
	int j;
	BYTE bCurrentAPICID;
	TCB* pstTask;


	// check current Core Local APIC ID
	bCurrentAPICID = kGetAPICID();


	// if Bootstrap Processor, initialize Task Pool, Scheduler Data Structure
	if(bCurrentAPICID == 0)
	{
		// init Task Pool
		kInitializeTCBPool();

		
		// initialize Ready List, Execution count per Priority, Wait List, SpinLock
		for(j=0; j<MAXPROCESSORCOUNT; j++)
		{
			for(i=0; i<TASK_MAXREADYLISTCOUNT; i++)
			{
				kInitializeList(&(gs_vstScheduler[j].vstReadyList[i]));
				gs_vstScheduler[j].viExecuteCount[i] = 0;
			}

			// init Wait List
			kInitializeList(&(gs_vstScheduler[j].stWaitList));

			// init SpinLock
			kInitializeSpinLock(&(gs_vstScheduler[j].stSpinLock));
		}
	}


	// allocate TCB and set as "Running Task"
	// the First Process in the kernel is the Task that performed Boot
	pstTask = kAllocateTCB();
	gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstTask;

	
	// Console Shell runs on BSP, Idle Task runs on AP
	// set Local APIC ID, Processor Affinity as current Core APIC ID
	pstTask->bAPICID = bCurrentAPICID;
	pstTask->bAffinity = bCurrentAPICID;

	
	// Bootstrap Processor runs "Idle Task, Console Shell (CLI) or Window Manager Task (GUI)" - Main()
	if(bCurrentAPICID == 0)
	{
		pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
	}
	// Application Processor runs "Idle Task" - MainForApplicationProcessor()
	else
	{
		pstTask->qwFlags = TASK_FLAGS_LOWEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE;
	}

	
	pstTask->qwParentProcessID = pstTask->stLink.qwID;
	pstTask->pvMemoryAddress   = (void*)0x100000;		// Kernel Code/Data Area in IA-32 mode (Data Structure, etc..)
	pstTask->qwMemorySize	   = 0x500000;				// 		from 0x100000 to 0x5FFFFF
	pstTask->pvStackAddress	   = (void*)0x600000;		// Kernel Stack Area in IA-32 mode
	pstTask->qwStackSize	   = 0x100000;				// 		from 0x600000 to 0x6FFFFF


	// init var used to calculate Process Usage
	gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask = 0;
	gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 0;

	
	// recent FPU Task ID : TASK_INVALIDID
	gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID = TASK_INVALIDID;
}


void kSetRunningTask(BYTE bAPICID, TCB* pstTask)
{
	// =============== CRITICAL SECTION START ===============
	kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	// ======================================================
	
	gs_vstScheduler[bAPICID].pstRunningTask = pstTask;

	// =============== CRITICAL SECTION END ===============
	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	// ====================================================
}


TCB* kGetRunningTask(BYTE bAPICID)
{
	TCB* pstRunningTask;

	// =============== CRITICAL SECTION START ===============
	kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	// ======================================================

	pstRunningTask = gs_vstScheduler[bAPICID].pstRunningTask;

	// =============== CRITICAL SECTION END ===============
	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	// ====================================================
	
	return pstRunningTask;
}


static TCB* kGetNextTaskToRun(BYTE bAPICID)
{
	TCB* pstTarget = NULL;
	int iTaskCount, i, j;
	

	// Check twice
	// 		if there are Tasks in the queue but all the Tasks in all the queues are executed once,
	// 		then all the queue may fail to select the Task by yield of all queue for Processor
	for(j=0; j<2; j++)
	{
		// search next Task to run in all Ready List
		for(i=0; i<TASK_MAXREADYLISTCOUNT; i++)
		{
			iTaskCount = kGetListCount(&(gs_vstScheduler[bAPICID].vstReadyList[i]));		// number of process

			// if Task count in List bigger then Execution count,
			// run Task on current Priority
			if(gs_vstScheduler[bAPICID].viExecuteCount[i] < iTaskCount)
			{
				pstTarget = (TCB*)kRemoveListFromHeader(&(gs_vstScheduler[bAPICID].vstReadyList[i]));
				gs_vstScheduler[bAPICID].viExecuteCount[i]++;

				break;
			}
			// all Task in LIST executed
			else
			{
				gs_vstScheduler[bAPICID].viExecuteCount[i] = 0;
			}
		}

		if(pstTarget != NULL)		// if next Task to run found..
		{
			break;
		}
	}

	return pstTarget;
}


static BOOL kAddTaskToReadyList(BYTE bAPICID, TCB* pstTask)
{
	BYTE bPriority;

	
	bPriority = GETPRIORITY(pstTask->qwFlags);
	

	if(bPriority == TASK_FLAGS_WAIT)
	{
		kAddListToTail(&(gs_vstScheduler[bAPICID].stWaitList), pstTask);
		
		return TRUE;
	}
	else if(bPriority >= TASK_MAXREADYLISTCOUNT)
	{
		return FALSE;
	}
	

	kAddListToTail(&(gs_vstScheduler[bAPICID].vstReadyList[bPriority]), pstTask);
	

	return TRUE;
}


// switch to Other Process
// 		WARNING : No calling when Interrupt or Exception occured
BOOL kSchedule(void)
{
	TCB* pstRunningTask, *pstNextTask;
	BOOL bPreviousInterrupt;
	BYTE bCurrentAPICID;


	// Interrupt should not be occured when switching
	// set Interrupt as not occur
	bPreviousInterrupt = kSetInterruptFlag(FALSE);


	// check Current Local APIC ID
	bCurrentAPICID = kGetAPICID();

	// check if there's the Task to switch
	if(kGetReadyTaskCount(bCurrentAPICID) < 1)
	{
		kSetInterruptFlag(bPreviousInterrupt);

		return FALSE;
	}


	// =============== CRITICAL SECTION START ===============
	kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
	// ======================================================


	// get Next Task to run
	pstNextTask = kGetNextTaskToRun(bCurrentAPICID);

	if(pstNextTask == NULL)
	{
		// CRITICAL SECTION END
		kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		kSetInterruptFlag(bPreviousInterrupt);

		return FALSE;
	}


	// modify currently running Task information, and then switch Context
	pstRunningTask = gs_vstScheduler[bCurrentAPICID].pstRunningTask;
	gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstNextTask;


	// if switched from Idle Task, increase Processor Usage Time  ( idle -> normal )
	if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
	{
		// Idle Task Time = All Processor Time - Used Processor Time
		gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask += 
			TASK_PROCESSORTIME - gs_vstScheduler[bCurrentAPICID].iProcessorTime;
	}


	// if Next Task is not the last FPU used Task, set TS bit
	if(gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
	{
		kSetTS();
	}
	else
	{
		kClearTS();
	}


	// update Available Processor Time
	gs_vstScheduler[bCurrentAPICID].iProcessorTime = TASK_PROCESSORTIME;


	// if Flags is ENDTASK, no need to save Context
	// insert Wait List, switch Context
	if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)
	{
		kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstRunningTask);

		// =============== CRITICAL SECTION END ===============
		kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		// ====================================================

		// switch Task
		kSwitchContext(NULL, &(pstNextTask->stContext));
	}
	else
	{
		kAddTaskToReadyList(bCurrentAPICID, pstRunningTask);

		// =============== CRITICAL SECTION END ===============
		kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		// ====================================================

		// switch Task
		kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
	}


	// restore Interrupt Flag
	kSetInterruptFlag(bPreviousInterrupt);

	return FALSE;
}


// switch to Other Process when Interrupt occured
// 		WARNING : must be called when Interrupt or Exception occured
BOOL kScheduleInInterrupt(void)
{
	TCB*  pstRunningTask, *pstNextTask;
	char* pcContextAddress;
	BYTE  bCurrentAPICID;
	QWORD qwISTStartAddress;


	// check Current Local APIC ID
	bCurrentAPICID = kGetAPICID();


	// =============== CRITICAL SECTION START ===============
	kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
	// ======================================================


	// if there's no Task to switch, exit
	pstNextTask = kGetNextTaskToRun(bCurrentAPICID);

	if(pstNextTask == NULL)			// is there Task to switch?
	{
		// CRITICAL SECTION END
		kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

		return FALSE;
	}

	// ------------------------------------------------
	// handling Task switching
	// 		overlap Next Task's Context to Context saved by Interrupt Handler
	// ------------------------------------------------
	

	// Stack for Interrupt Handling allocated as 64 KB per 1 Core,
	// so, calculate IST Address using Local APIC ID
	// 		IST is Stack, so, Context saved from Top of Stack
	qwISTStartAddress = IST_STARTADDRESS + IST_SIZE - (IST_SIZE / MAXPROCESSORCOUNT * bCurrentAPICID);
	pcContextAddress = (char*)qwISTStartAddress - sizeof(CONTEXT);

	pstRunningTask = gs_vstScheduler[bCurrentAPICID].pstRunningTask;
	gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstNextTask;


	// if Idle Task.. increase Processor Usage Time
	if( (pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE )
	{
		gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;	// Timer Interrupt
	}


	// if End Flag set.. insert Wait List, not saving Context
	if(pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)
	{
		kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstRunningTask);
	}
	// if Task not ended, copy Context in IST, move current Task to Ready List
	else
	{
		kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
	}

	
	// if Next Task is not Last FPU Used Task, set TS bit
	if(gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
	{
		kSetTS();
	}
	else
	{
		kClearTS();
	}


	// =============== CRITICAL SECTION END ===============
	kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
	// ====================================================	
	

	// copy Context to IST for Auto Task Switching
	kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));


	// if ENDTASK Flag clear, add Task to Scheduler
	if( (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) != TASK_FLAGS_ENDTASK)
	{
		// add Task to Scheduler with Load Balancing
		kAddTaskToSchedulerWithLoadBalancing(pstRunningTask);
	}


	// update Available Processor Time
	gs_vstScheduler[bCurrentAPICID].iProcessorTime = TASK_PROCESSORTIME;


	return TRUE;
}


void kDecreaseProcessorTime(BYTE bAPICID)
{
	gs_vstScheduler[bAPICID].iProcessorTime--;
}


BOOL kIsProcessorTimeExpired(BYTE bAPICID)
{
	if(gs_vstScheduler[bAPICID].iProcessorTime <= 0)
	{
		return TRUE;
	}

	return FALSE;
}



static TCB* kRemoveTaskFromReadyList(BYTE bAPICID, QWORD qwTaskID)
{
	TCB* pstTarget;
	BYTE bPriority;
	

	// invalid Task ID
	if(GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT)
	{
		return NULL;
	}


	// search target TCB in TCB Pool, and check if ID matches
	pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);

	if(pstTarget->stLink.qwID != qwTaskID)		// not matching with search result
	{
		return NULL;
	}


	// remove Task from Ready List which Task exists
	bPriority = GETPRIORITY(pstTarget->qwFlags);

	if(bPriority >= TASK_MAXREADYLISTCOUNT)
	{
		return NULL;
	}


	pstTarget = kRemoveList(&(gs_vstScheduler[bAPICID].vstReadyList[bPriority]), qwTaskID);

	return pstTarget;
}


// Return Scheduler ID including target Task, Lock SpinLock of target Scheduler
static BOOL kFindSchedulerOfTaskAndLock(QWORD qwTaskID, BYTE* pbAPICID)
{
	TCB* pstTarget;
	BYTE bAPICID;

	
	while(1)
	{
		// search TCB using Task ID, and check the TCB running in which Scheduler
		pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);

		if( (pstTarget == NULL) || (pstTarget->stLink.qwID != qwTaskID) )
		{
			return FALSE;
		}

		
		// get Core ID which Current Task running on
		bAPICID = pstTarget->bAPICID;

		
		////////////////////////////////
		// CRITICAL SECTION START
		kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
		////////////////////////////////
		

		// check twice for correct lock of SpinLock
		pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);

		if(pstTarget->bAPICID == bAPICID)
		{
			break;
		}


		/*
		   if "Local APIC ID of TCB before Lock" != "Local APIC ID of TCB after Lock",
		   it means Task moves to other Core during getting SpinLock
		   so, release SpinLock and get SpinLock of which Task moved in
		*/

		////////////////////////////////
		// CRITICAL SECTION END
		kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
		////////////////////////////////

	}	// searching match APIC ID loop end

	*pbAPICID = bAPICID;
	
	return TRUE;
}


BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority)
{
	TCB* pstTarget;
	BYTE bAPICID;


	// invalid Priority
	if(bPriority > TASK_MAXREADYLISTCOUNT)
	{
		return FALSE;
	}

	
	// search Local APIC ID including target Task, lock SpinLock

	// =============== CRITICAL SECTION START ===============
	if(kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE)
	{
		return FALSE;
	}
	// ======================================================
	

	// if executing now, modify Priority only
	// move to target List when IRQ 0 (PIT Controller Interrupt) occurs
	pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;

	if(pstTarget->stLink.qwID == qwTaskID)
	{
		SETPRIORITY(pstTarget->qwFlags, bPriority);
	}

	// if ready, search target from Ready List and move to matching List
	else
	{
		// if failed to search Task in Ready List, search Task directly and set Priority
		pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);

		if(pstTarget == NULL)		// Target Task not exist in Ready List (or invalid ID)
		{
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));		// access directly

			if(pstTarget != NULL)
			{
				SETPRIORITY(pstTarget->qwFlags, bPriority);
			}
		}
		else
		{
			// set Priority and re-insert into Ready List
			SETPRIORITY(pstTarget->qwFlags, bPriority);
			kAddTaskToReadyList(bAPICID, pstTarget);
		}
	}


	// =============== CRITICAL SECTION END ===============
	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	// ====================================================


	return TRUE;
}


BOOL kEndTask(QWORD qwTaskID)
{
	TCB* pstTarget;
	BYTE bPriority;
	BYTE bAPICID;


	// =============== CRITICAL SECTION START ===============
	if(kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE)
	{
		return FALSE;
	}
	// ======================================================
	

	// if target Task is running, set ENDTASK bit, switch Task
	pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
	
	if(pstTarget->stLink.qwID == qwTaskID)		// if Task executing..
	{
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
		

		// CRITICAL SECTION END
		kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

		
		// apply this code if the Task is running on current Scheduler
		if(kGetAPICID() == bAPICID)
		{
			kSchedule();

			// below code not executed
			while(1);
		}

		return TRUE;
	}
	
	// if Task not executing, search it in Ready Queue and connect to Wait List
	// if Task not found in Ready List, search Task directly and set ENDTASK bit
	pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);

	if(pstTarget == NULL)		// if Process not in Ready List
	{
		// direct search using Task ID
		pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));

		if(pstTarget != NULL)	// Process exist, but not in Ready List
		{
			pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
			SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
		}
			
		// CRITICAL SECTION END
		kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

		return TRUE;
	}

	pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
	SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
	kAddListToTail(&(gs_vstScheduler[bAPICID].stWaitList), pstTarget);


	// =============== CRITICAL SECTION END ===============
	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	// ====================================================
	
	return TRUE;
}


// End Task itself
void kExitTask(void)
{
	kEndTask(gs_vstScheduler[kGetAPICID()].pstRunningTask->stLink.qwID);
}


// Return all Task count in Ready List
int kGetReadyTaskCount(BYTE bAPICID)
{
	int iTotalCount = 0;
	int i;

	
	// =============== CRITICAL SECTION START ===============
	kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	// ======================================================	


	// get Task count by checking all Ready Queue
	for(i=0; i<TASK_MAXREADYLISTCOUNT; i++)
	{
		iTotalCount += kGetListCount(&(gs_vstScheduler[bAPICID].vstReadyList[i]));
	}


	// =============== CRITICAL SECTION END ===============
	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	// ====================================================


	return iTotalCount;
}


// Return total Task count
int kGetTaskCount(BYTE bAPICID)
{
	int iTotalCount;


	// Task in Ready List + Task in Wait List
	iTotalCount = kGetReadyTaskCount(bAPICID);
	

	// =============== CRITICAL SECTION START ===============
	kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	// ======================================================
	
	iTotalCount += kGetListCount(&(gs_vstScheduler[bAPICID].stWaitList)) + 1;

	// =============== CRITICAL SECTION END ===============
	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	// ====================================================
	

	return iTotalCount;
}


TCB* kGetTCBInTCBPool(int iOffset)
{
	if( (iOffset < -1) || (iOffset > TASK_MAXCOUNT) )
		return NULL;
	
	return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}


BOOL kIsTaskExist(QWORD qwID)
{
	TCB* pstTCB;
	
	pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
	
	if( (pstTCB == NULL) || (pstTCB->stLink.qwID != qwID) )		// invaild or not exist
		return FALSE;

	return TRUE;
}


// Return percentage of Processor Usage
QWORD kGetProcessorLoad(BYTE bAPICID)
{
	return gs_vstScheduler[bAPICID].qwProcessorLoad;
}


static TCB* kGetProcessByThread(TCB* pstThread)
{
	TCB* pstProcess;

	// if Process, return itself
	if(pstThread->qwFlags & TASK_FLAGS_PROCESS)
		return pstThread;

	// if Thread, extract TCB from TCB Pool through qwParentProcessID
	pstProcess = kGetTCBInTCBPool( GETTCBOFFSET(pstThread->qwParentProcessID) );

	// if Process not exist or Task ID not match, return NULL
	if( (pstProcess == NULL) || (pstProcess->stLink.qwID != pstThread->qwParentProcessID) )
		return NULL;

	return pstProcess;
}



// Add Task to proper Scheduler using Task count of each Scheduler
// 		if Load Balancing disabled, insert Task into current Core
// 		even though Load Balancing disabled, APIC ID should be set in pstTask
void kAddTaskToSchedulerWithLoadBalancing(TCB* pstTask)
{
	BYTE bCurrentAPICID;
	BYTE bTargetAPICID;

	
	// check APIC of Core which Task running
	bCurrentAPICID = pstTask->bAPICID;


	// carry out Load Balancing when..
	// 		1. use Load Balancing
	// 		2. Affinity is 0xFF (TASK_LOADBALANCINGID)
	if( (gs_vstScheduler[bCurrentAPICID].bUseLoadBalancing == TRUE) &&
		(pstTask->bAffinity == TASK_LOADBALANCINGID) )
	{
		// select Scheduler to add Task
		bTargetAPICID = kFindSchedulerOfMinimumTaskCount(pstTask);
	}

	// if Process Affinity field contains other Core APIC ID,
	// move Task to target Scheduler
	else if( (pstTask->bAffinity != bCurrentAPICID) && 
			 (pstTask->bAffinity != TASK_LOADBALANCINGID) )
	{
		bTargetAPICID = pstTask->bAffinity;
	}

	// if Load Balancing disabled, insert Task into current Scheduler
	else
	{
		bTargetAPICID = bCurrentAPICID;
	}


	// ============================================
	// CRITICAL SECTION START (current Scheduler)
	kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
	// ============================================
	

	// if current Scheduler != target Scheduler to add Task, move Task
	// if current Task is the last FPU used Task, FPU Context should be stored in Memory
	// (because FPU is not shared)
	if( (bCurrentAPICID != bTargetAPICID) &&
		(pstTask->stLink.qwID == gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID) )
	{
		// TS bit should be cleared before saving FPU.
		// otherwise, Exception 7 (Device Not Available) occurs
		kClearTS();
		kSaveFPUContext(pstTask->vqwFPUContext);
		gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID = TASK_INVALIDID;
	}
	

	// ============================================
	// CRITICAL SECTION END (current Scheduler)
	kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
	// ============================================
	

	// ============================================
	// CRITICAL SECTION START (target Scheduler)
	kLockForSpinLock(&(gs_vstScheduler[bTargetAPICID].stSpinLock));
	// ============================================


	// set APIC ID of Core which Task going to running, add Task to target Scheduler
	pstTask->bAPICID = bTargetAPICID;
	kAddTaskToReadyList(bTargetAPICID, pstTask);


	// ============================================
	// CRITICAL SECTION END (target Scheduler)
	kUnlockForSpinLock(&(gs_vstScheduler[bTargetAPICID].stSpinLock));
	// ============================================
}


// Return Scheduler ID to add Task
// 		pstTask should contain filled Flag field, filled Processor Affinity field
static BYTE kFindSchedulerOfMinimumTaskCount(const TCB* pstTask)
{
	BYTE bPriority;
	BYTE i;
	int  iCurrentTaskCount;
	int  iMinTaskCount;
	BYTE bMinCoreIndex;
	int  iTempTaskCount;
	int  iProcessorCount;
	INTERRUPTMANAGER* pstInterruptManager;


	// get Core count
	iProcessorCount = kGetProcessorCount();
	
	// if single Core, execute Task in current Core
	if(iProcessorCount == 1)
	{
		return pstTask->bAPICID;
	}

	
	// get Priority
	bPriority = GETPRIORITY(pstTask->qwFlags);

	// get Task count in Ready List[bPriority] in current Scheduler
	iCurrentTaskCount = kGetListCount(&(gs_vstScheduler[pstTask->bAPICID].vstReadyList[bPriority]));

	
	// search for ID of Scheduler which contains minimum Task count with over 2 difference
	iMinTaskCount = TASK_MAXCOUNT;
	bMinCoreIndex = pstTask->bAPICID;


	pstInterruptManager = kGetInterruptManager();


	// "i" means APIC ID, "iTempTaskCount" means total Task count of APIC ID[i]
	//for(i=0; i<iProcessorCount; i++)
	for(i=0; i<MAXPROCESSORCOUNT; i++)
	{
		if( (i == pstTask->bAPICID) || (pstInterruptManager->vvqwCoreInterruptCount[i][0] == 0) )	// FIXME
		{
			continue;
		}


		// check all Scheduler
		iTempTaskCount = kGetListCount(&(gs_vstScheduler[i].vstReadyList[bPriority]));

		// update info when
		// 		1. 2 or more difference between APIC ID[i] Task List count and current Task List count
		// 		2. APIC ID[i] Task List count < previous minimum Task count
		if( (iTempTaskCount + 2 <= iCurrentTaskCount) && (iTempTaskCount < iMinTaskCount) )
		{
			bMinCoreIndex = i;
			iMinTaskCount = iTempTaskCount;
		}
	}	// search minimum Task count in all Core loop end


	return bMinCoreIndex;
}


BYTE kSetTaskLoadBalancing(BYTE bAPICID, BOOL bUseLoadBalancing)
{
	gs_vstScheduler[bAPICID].bUseLoadBalancing = bUseLoadBalancing;
}


BOOL kChangeProcessorAffinity(QWORD qwTaskID, BYTE bAffinity)
{
	TCB* pstTarget;
	BYTE bAPICID;


	// find Local APIC ID including Task, lock SpinLock
	// =============== CRITICAL SECTION START ===============
	if(kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE)
	{
		return FALSE;
	}
	// ======================================================
	

	// if currently running Task, change Affinity field only
	pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;

	if(pstTarget->stLink.qwID == qwTaskID)
	{
		// change Process Affinity
		pstTarget->bAffinity = bAffinity;


		// =============== CRITICAL SECTION END ===============
		kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
		// ====================================================
	}
	// if not running Task, search Task from Ready List and move
	else
	{
		// if Task not found in Ready List, search directly and set Affinity
		pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);

		if(pstTarget == NULL)
		{
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));

			if(pstTarget != NULL)
			{
				pstTarget->bAffinity = bAffinity;
			}
		}
		else
		{
			pstTarget->bAffinity = bAffinity;
		}


		// =============== CRITICAL SECTION END ===============
		kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
		// ====================================================

		// add Task to Scheduler considering Load Balancing
		kAddTaskToSchedulerWithLoadBalancing(pstTarget);
	}


	return TRUE;
}



// =============================================
// for Idle Task
// =============================================

// Idle Task
// 		handle Task in Wait List
void kIdleTask(void)
{
	TCB* pstTask, *pstChildThread, *pstProcess;
	QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
	QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
	QWORD qwTaskID, qwChildThreadID;
	int i, iCount;
	void* pstThreadLink;
	BYTE bCurrentAPICID;
	BYTE bProcessAPICID;


	// get current Core Local APIC ID
	bCurrentAPICID = kGetAPICID();


	// save base information for Process Usage calculation
	qwLastSpendTickInIdleTask = gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask;
	qwLastMeasureTickCount = kGetTickCount();

	while(1)
	{
		// save current status
		qwCurrentMeasureTickCount = kGetTickCount();
		qwCurrentSpendTickInIdleTask = gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask;


		// Process Usage Calculation
		// 		100 - (Idle Task Processor Time) * 100 / (All Task Processor Time)
		if(qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0)
		{
			gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 0;
		}
		else
		{
			gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 100 - 
				(qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) *
				100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount);
		}

		// save current state to previous state
		qwLastMeasureTickCount = qwCurrentMeasureTickCount;
		qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;


		// halt Processor by Processor Load
		kHaltProcessorByLoad(bCurrentAPICID);


	
		// if Awaiting Task in Wait List, end Task
		if(kGetListCount(&(gs_vstScheduler[bCurrentAPICID].stWaitList)) > 0)
		{
			while(1)
			{
				// =============== CRITICAL SECTION START ===============
				kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
				// ======================================================

				pstTask = kRemoveListFromHeader(&(gs_vstScheduler[bCurrentAPICID].stWaitList));
					
				// =============== CRITICAL SECTION END ===============
				kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
				// ====================================================

				
				if(pstTask == NULL)
				{
					break;
				}


				if(pstTask->qwFlags & TASK_FLAGS_PROCESS)
				{
					// if Child Thread exist, end all Thread, then insert into Child Thread List again
					iCount = kGetListCount(&(pstTask->stChildThreadList));

					for(i=0; i<iCount; i++)
					{
						// =============== CRITICAL SECTION START ===============
						kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
						// ======================================================


						// extract Address from Thread Link Address to end Thread
						pstThreadLink = (TCB*)kRemoveListFromHeader(&(pstTask->stChildThreadList));

						if(pstThreadLink == NULL)
						{
							// CRITICAL SECTION END 
							kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
							
							break;
						}

						
						// the information connected with Child Thread List is the Start Address of stThreadLink
						// additional calculation needed to get Start Address of TCB Struct
						pstChildThread = GETTCBFROMTHREADLINK(pstThreadLink);


						// insert into Child Thread List again to be removed itself
						kAddListToTail(&(pstTask->stChildThreadList), &(pstChildThread->stThreadLink));
						qwChildThreadID = pstChildThread->stLink.qwID;

						
						// =============== CRITICAL SECTION END ===============
						kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
						// ====================================================
						

						// find Child Thread then end
						kEndTask(pstChildThread->stLink.qwID);

					}	// end child thread loop end


					// if Child Thread remained, insert into Wait List again for completely ended Thread
					if(kGetListCount(&(pstTask->stChildThreadList)) > 0)
					{
						// =============== CRITICAL SECTION START ===============
						kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
						// ======================================================


						kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstTask);


						// =============== CRITICAL SECTION END ===============
						kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
						// ====================================================


						continue;		// while(1) continue
					}
					
					// Process end : remove allocated memory
					else
					{
						// User Level Process allocates memory when it created
						// need to free allocated memory when Task End process
						if(pstTask->qwFlags & TASK_FLAGS_USERLEVEL)
						{
							kFreeMemory(pstTask->pvMemoryAddress);
						}
					}
				}

				else if(pstTask->qwFlags & TASK_FLAGS_THREAD)
				{
					// if Thread, remove from Child Thread List of Process
					pstProcess = kGetProcessByThread(pstTask);
					
					if(pstProcess != NULL)
					{
						// =============== CRITICAL SECTION ===============

						// find Scheduler ID which Process belongs
						if(kFindSchedulerOfTaskAndLock(pstProcess->stLink.qwID, &bProcessAPICID) == TRUE)
						{
							kRemoveList(&(pstProcess->stChildThreadList), pstTask->stLink.qwID);


							kUnlockForSpinLock(&(gs_vstScheduler[bProcessAPICID].stSpinLock));
						}
						// ================================================	
					}
				}

				// below code executed when Task ended successfully
				// return TCB and Stack
				qwTaskID = pstTask->stLink.qwID;

				kFreeMemory(pstTask->pvStackAddress);
				kFreeTCB(qwTaskID);

				kPrintf("IDLE : Task ID[0x%q] is Completely ended.\n", qwTaskID);

			}	// end Process/Thread loop end

		}	// if awaiting Task in Wait List end 

		kSchedule();
	}
}


void kHaltProcessorByLoad(BYTE bAPICID)
{
	if(gs_vstScheduler[bAPICID].qwProcessorLoad < 40)
	{
		kHlt();
		kHlt();
		kHlt();
	}
	else if(gs_vstScheduler[bAPICID].qwProcessorLoad < 80)
	{
		kHlt();
		kHlt();
	}
	else if(gs_vstScheduler[bAPICID].qwProcessorLoad < 95)
	{
		kHlt();
	}
}



QWORD kGetLastFPUUsedTaskID(BYTE bAPICID)
{
	return gs_vstScheduler[bAPICID].qwLastFPUUsedTaskID;
}


void kSetLastFPUUsedTaskID(BYTE bAPICID, QWORD qwTaskID)
{
	gs_vstScheduler[bAPICID].qwLastFPUUsedTaskID = qwTaskID;
}



