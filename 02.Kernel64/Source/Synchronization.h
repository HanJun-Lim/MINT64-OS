#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include "Types.h"


// ========== Structure ==========

#pragma pack(push, 1)


typedef struct kMutexStruct
{
	// Task ID, count of Lock
	volatile QWORD qwTaskID;
	volatile DWORD dwLockCount;

	// Lock Flag
	volatile BOOL bLockFlag;

	// Padding for 8-byte Alignment
	BYTE vbPadding[3];
} MUTEX;


typedef struct kSpinLockStruct
{
	// Local APIC ID, Lock Count
	volatile DWORD dwLockCount;
	volatile BYTE  bAPICID;

	// Lock Flag
	volatile BOOL bLockFlag;

	// Interrupt Flag
	volatile BOOL bInterruptFlag;

	// Padding for alignment by 8 bytes
	BYTE vbPadding[1];
} SPINLOCK;


#pragma pack(pop)



// ========== Function ==========


#if 0
// ---------- for Data used by Overall System ----------
BOOL kLockForSystemData(void);
void kUnlockForSystemData(BOOL bInterruptFlag);


void kInitializeMutex(MUTEX* pstMutex);
#endif

void kInitializeSpinLock(SPINLOCK* pstSpinLock);
void kLockForSpinLock(SPINLOCK* pstSpinLock);
void kUnlockForSpinLock(SPINLOCK* pstSpinLock);


// ---------- for Data used by Tasks ----------
void kLock(MUTEX* pstMutex);
void kUnlock(MUTEX* pstMutex);



#endif
