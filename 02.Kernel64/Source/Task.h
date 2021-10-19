#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"
#include "Synchronization.h"


// ---------------------------
// ---------- Macro ----------
// ---------------------------

// SS, RSP, RFLAGS, CS, RIP + 19 Registers saved by ISR
#define TASK_REGISTERCOUNT			(5 + 19)
#define TASK_REGISTERSIZE			8


// Register Offset for CONTEXT Structure
#define TASK_GSOFFSET				0
#define TASK_FSOFFSET				1
#define TASK_ESOFFSET				2
#define TASK_DSOFFSET				3
#define TASK_R15OFFSET				4
#define TASK_R14OFFSET				5
#define TASK_R13OFFSET				6
#define TASK_R12OFFSET				7
#define TASK_R11OFFSET				8
#define TASK_R10OFFSET				9
#define TASK_R9OFFSET				10
#define TASK_R8OFFSET				11
#define TASK_RSIOFFSET				12
#define TASK_RDIOFFSET				13
#define TASK_RDXOFFSET				14
#define TASK_RCXOFFSET				15
#define TASK_RBXOFFSET				16
#define TASK_RAXOFFSET				17
#define TASK_RBPOFFSET				18
#define TASK_RIPOFFSET				19
#define TASK_CSOFFSET				20
#define TASK_RFLAGSOFFSET			21
#define TASK_RSPOFFSET				22
#define TASK_SSOFFSET				23


// Task Pool Address
#define TASK_TCBPOOLADDRESS			0x800000
#define TASK_MAXCOUNT				1024


// Stack Pool Address and Size
#define TASK_STACKPOOLADDRESS		(TASK_TCBPOOLADDRESS + sizeof(TCB) * TASK_MAXCOUNT)
#define TASK_STACKSIZE				(64 * 1024)			// 64 KB


// Invalid Stack ID
#define TASK_INVALIDID				0xFFFFFFFFFFFFFFFF


// Max time for one Process	(5ms)
#define TASK_PROCESSORTIME			5


// number of Ready List
#define TASK_MAXREADYLISTCOUNT		5


// Task Priority
#define TASK_FLAGS_HIGHEST			0
#define TASK_FLAGS_HIGH				1
#define TASK_FLAGS_MEDIUM			2
#define TASK_FLAGS_LOW				3
#define TASK_FLAGS_LOWEST			4

#define TASK_FLAGS_WAIT				0xFF


// Task Flags
#define TASK_FLAGS_ENDTASK			0x8000000000000000
#define TASK_FLAGS_SYSTEM			0x4000000000000000
#define TASK_FLAGS_PROCESS			0x2000000000000000
#define TASK_FLAGS_THREAD			0x1000000000000000
#define TASK_FLAGS_IDLE				0x0800000000000000
#define TASK_FLAGS_USERLEVEL		0x0400000000000000


// Function Macro
#define GETPRIORITY(x)				( (x) & 0xFF )											// x : Flags
#define SETPRIORITY(x, priority)	( (x) = ( (x) & 0xFFFFFFFFFFFFFF00 ) | (priority) )		// x : Flags
#define GETTCBOFFSET(x)				( (x) & 0xFFFFFFFF )									// x : Task ID

#define GETTCBFROMTHREADLINK(x)		(TCB*)( (QWORD)(x) - offsetof(TCB, stThreadLink) )		// x : Thread Link


// If this value set in Processor Affinity field, regard as it has No favorite CPU 
#define TASK_LOADBALANCINGID		0xFF



// -------------------------------
// ---------- Structure ----------
// -------------------------------

#pragma pack(push, 1)


// for Context
typedef struct kContextStruct
{
	QWORD vqRegister[TASK_REGISTERCOUNT];
} CONTEXT;


// for Task Control Block - Managing Process, Thread Status
// FPU Context Added : MUST BE ALIGNED BY MULTIPLE OF 16
typedef struct kTaskControlBlockStruct
{
	// Next Data, ID
	LISTLINK stLink;

	// Flags
	QWORD qwFlags;

	// Start Address, Size of Process Memory Area
	void* pvMemoryAddress;
	QWORD qwMemorySize;

	// ============================================
	// Thread Information below
	// ============================================
	
	// ID, Position of Child Process 
	LISTLINK stThreadLink;

	// Parent Process ID
	QWORD qwParentProcessID;

	// FPU Context : MUST be aligned by Multiple of 16
	QWORD vqwFPUContext[512/8];

	// Child Process List
	LIST stChildThreadList;

	// Context
	CONTEXT stContext;

	// Stack Address and Size
	void* pvStackAddress;
	QWORD qwStackSize;

	// FPU Usage
	BOOL bFPUUsed;

	// Processor Affinity
	BYTE bAffinity;

	// Local APIC ID of currently running Core
	BYTE bAPICID;

	// Padding for alignment : total 816 byte
	char vcPadding[9];
} TCB;


// for managing TCB Pool
typedef struct kTCBPoolManagerStruct
{
	// for Synchronization
	SPINLOCK stSpinLock;

	// Task Pool information
	TCB* pstStartAddress;
	int  iMaxCount;
	int  iUseCount;

	// number of allocated TCB
	int iAllocatedCount;
} TCBPOOLMANAGER;


// for managing scheduler
typedef struct kSchedulerStruct
{
	// SpinLock for Synchronization
	SPINLOCK stSpinLock;

	// Currently Running Task
	TCB* pstRunningTask;

	// Available Processor Time
	int iProcessorTime;

	// Ready List, seperated by priority - MLQ
	LIST vstReadyList[TASK_MAXREADYLISTCOUNT];

	// Wait Task
	LIST stWaitList;

	// number of execution in Ready List
	int viExecuteCount[TASK_MAXREADYLISTCOUNT];

	// for calculating Processor Load
	QWORD qwProcessorLoad;

	// Processor time usage of Idle Task
	QWORD qwSpendProcessorTimeInIdleTask;

	// Task ID of recent FPU Task
	QWORD qwLastFPUUsedTaskID;

	// Use Load Balancing?
	BOOL bUseLoadBalancing;
} SCHEDULER;


#pragma pack(pop)



// ------------------------------
// ---------- Function ----------
// ------------------------------

// for Task Pool, Task

static void kInitializeTCBPool(void);
static TCB* kAllocateTCB(void);
static void kFreeTCB(QWORD qwID);
TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress, BYTE bAffinity);
static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize);

// for Scheduler

void kInitializeScheduler(void);
void kSetRunningTask(BYTE bAPICID, TCB* pstTask);
TCB* kGetRunningTask(BYTE bAPICID);
static TCB* kGetNextTaskToRun(BYTE bAPICID);
static BOOL kAddTaskToReadyList(BYTE bAPICID, TCB* pstTask);
BOOL kSchedule(void);
BOOL kScheduleInInterrupt(void);
void kDecreaseProcessorTime(BYTE bAPICID);
BOOL kIsProcessorTimeExpired(BYTE bAPICID);

static TCB* kRemoveTaskFromReadyList(BYTE bAPICID, QWORD qwTaskID);
static BOOL kFindSchedulerOfTaskAndLock(QWORD qwTaskID, BYTE* pbAPICID);
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority);
BOOL kEndTask(QWORD qwTaskID);
void kExitTask(void);
int  kGetReadyTaskCount(BYTE bAPICID);
int  kGetTaskCount(BYTE bAPICID);
TCB* kGetTCBInTCBPool(int iOffset);
BOOL kIsTaskExist(QWORD qwID);
QWORD kGetProcessorLoad(BYTE bAPICID);

static TCB* kGetProcessByThread(TCB* pstThread);

void kAddTaskToSchedulerWithLoadBalancing(TCB* pstTask);
static BYTE kFindSchedulerOfMinimumTaskCount(const TCB* pstTask);
BYTE kSetTaskLoadBalancing(BYTE bAPICID, BOOL bUseLoadBalancing);
BOOL kChangeProcessorAffinity(QWORD qwTaskID, BYTE bAffinity);

// for Idle Task

void kIdleTask(void);
void kHaltProcessorByLoad(BYTE bAPICID);

// for FPU

QWORD kGetLastFPUUsedTaskID(BYTE bAPICID);
void  kSetLastFPUUsedTaskID(BYTE bAPICID, QWORD qwTaskID);



#endif
