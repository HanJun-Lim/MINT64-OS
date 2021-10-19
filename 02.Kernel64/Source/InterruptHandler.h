#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "Types.h"
#include "MultiProcessor.h"


// ---------- Macro ----------

// Interrupt Vector Max Count is 16 (handle only ISA Bus Interrupt)

#define INTERRUPT_MAXVECTORCOUNT			16

// The Time performing Load Balancing

#define INTERRUPT_LOADBALANCINGDIVIDOR		10



// ---------- Structure ----------

typedef struct kInterruptManagerStruct
{
	// Interrupt Handling Count per Core
	// Max Core Count * Max Interrupt Vector Count
	QWORD vvqwCoreInterruptCount[MAXPROCESSORCOUNT][INTERRUPT_MAXVECTORCOUNT];

	// Use Load Balancing?
	BOOL bUseLoadBalancing;

	// Using Symmetric I/O Mode now?
	BOOL bSymmetricIOMode;
} INTERRUPTMANAGER;



// ---------- Function ----------

void kSetSymmetricIOMode(BOOL bSymmetricIOMode);
void kSetInterruptLoadBalancing(BOOL bUseLoadBalancing);
void kIncreaseInterruptCount(int iIRQ);
void kSendEOI(int iIRQ);
INTERRUPTMANAGER* kGetInterruptManager(void);
void kProcessLoadBalancing(int iIRQ);


void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode);
void kCommonInterruptHandler(int iVectorNumber);
void kKeyboardHandler(int iVectorNumber);
void kTimerHandler(int iVectorNumber);
void kDeviceNotAvailableHandler(int iVectorNumber);
void kHDDHandler(int iVectorNumber);
void kMouseHandler(int iVectorNumber);



#endif
