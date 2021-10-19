#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"
#include "Mouse.h"


static INTERRUPTMANAGER gs_stInterruptManager;


void kInitializeHandler(void)
{
	kMemSet(&gs_stInterruptManager, 0, sizeof(gs_stInterruptManager));
}


void kSetSymmetricIOMode(BOOL bSymmetricIOMode)
{
	gs_stInterruptManager.bSymmetricIOMode = bSymmetricIOMode;
}


void kSetInterruptLoadBalancing(BOOL bUseLoadBalancing)
{
	gs_stInterruptManager.bUseLoadBalancing = bUseLoadBalancing;
}


void kIncreaseInterruptCount(int iIRQ)
{
	// increase Interrupt Count of current Core
	gs_stInterruptManager.vvqwCoreInterruptCount[kGetAPICID()][iIRQ]++;
}


void kSendEOI(int iIRQ)
{
	// if not Symmetric I/O Mode, PIC Mode
	// so, you should send EOI to PIC Controller
	if(gs_stInterruptManager.bSymmetricIOMode == FALSE)
	{
		kSendEOIToPIC(iIRQ);
	}
	else
	{
		kSendEOIToLocalAPIC();
	}
}


INTERRUPTMANAGER* kGetInterruptManager(void)
{
	return &gs_stInterruptManager;
}


void kProcessLoadBalancing(int iIRQ)
{
	QWORD qwMinCount = 0xFFFFFFFFFFFFFFFF;
	int   iMinCountCoreIndex;
	int   iCoreCount;
	int   i;
	BOOL  bResetCount = FALSE;
	BYTE  bAPICID;


	bAPICID = kGetAPICID();


	// if Load Balancing disabled or no need to Load Balancing, exit
	if( (gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] == 0) ||
		((gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] % INTERRUPT_LOADBALANCINGDIVIDOR) != 0) ||
		(gs_stInterruptManager.bUseLoadBalancing == FALSE) )
	{
		return;
	}


	// get Core Count, and select Core which has minimum Interrupt Handle Count
	iMinCountCoreIndex = 0;
	iCoreCount = kGetProcessorCount();

	for(i=0; i<MAXPROCESSORCOUNT; i++)
	{
		if(gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] < qwMinCount)
		{
			// FIXME : handle the processor without count of timer interrupt as invalid
			if(gs_stInterruptManager.vvqwCoreInterruptCount[i][0] == 0)
			{
				continue;
			}
			
			qwMinCount = gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ];
			iMinCountCoreIndex = i;
		}


		// if getting close to max value, set all count to 0
		else if(gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] >= 0xFFFFFFFFFFFFFFFE)
		{
			bResetCount = TRUE;
		}
	}
	

	// modify I/O Redirection Table to send Interrupt to Local APIC which has minimum Interrupt Handling count
	kRoutingIRQToAPICID(iIRQ, iMinCountCoreIndex);

 
	// if getting close to max value, set all count to 0
	if(bResetCount == TRUE)
	{
		for(i=0; i<iCoreCount; i++)
		{
			gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] = 0;
		}
	}
}


// ---------- Interrupt Handlers below ----------

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode)
{
	char vcBuffer[100];
	BYTE bAPICID;
	TCB* pstTask;


	// get APIC ID that exception occured
	bAPICID = kGetAPICID();

	// get Task that running on current core
	pstTask = kGetRunningTask(bAPICID);


	kPrintStringXY(0, 0, "================================================");
	kPrintStringXY(0, 1, "               Exception Occured                ");
	// print Exception Vector Number, Core ID, Error Code
	kSPrintf(vcBuffer,   "  Vector: %d    Core ID:0x%X    ErrorCode:0x%X  ", iVectorNumber, bAPICID, qwErrorCode);
	kPrintStringXY(0, 2, vcBuffer);
	// print Task ID
	kSPrintf(vcBuffer,   "        Task ID:0x%Q", pstTask->stLink.qwID);
	kPrintStringXY(0, 3, vcBuffer);
	kPrintStringXY(0, 4, "================================================");
	

	// if User-level Task, exit itself and switch to other Task
	if(pstTask->qwFlags & TASK_FLAGS_USERLEVEL)
	{
		kEndTask(pstTask->stLink.qwID);

		// kEndTask() also switches to other Task, below loop not executed
		while(1);
	}
	// if Kernel-level Task, go to infinite loop
	else
	{
		while(1);
	}


}


void kCommonInterruptHandler(int iVectorNumber)
{
	char vcBuffer[] = "[INT:  , ]";
	static int g_iCommonInterruptCount = 0;
	int iIRQ;


	// ==================================================================
	// interrupt alert, print interrupt message
	
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	vcBuffer[8] = '0' + g_iCommonInterruptCount;

	g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
	
	kPrintStringXY(70, 0, vcBuffer);

	// ==================================================================
	
	
	// get IRQ Number from Interrupt Vector
	iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

	// send EOI
	kSendEOI(iIRQ);

	// update Interrupt Occur Count
	kIncreaseInterruptCount(iIRQ);

	// Load Balancing
	kProcessLoadBalancing(iIRQ);
}


void kKeyboardHandler(int iVectorNumber)
{
	char vcBuffer[] = "[INT:  , ]";
	static int g_iKeyboardInterruptCount = 0;
	BYTE bTemp;
	int iIRQ;


	// ==================================================================
	// interrupt alert, print interrupt message
	
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	vcBuffer[8] = '0' + g_iKeyboardInterruptCount;
	
	g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;

	kPrintStringXY(0, 0, vcBuffer);

	// ==================================================================


	// read Data in Output Buffer (Port 0x60)
	// and insert it into Keyboard Queue or Mouse Queue
	if(kIsOutputBufferFull() == TRUE)
	{
		if(kIsMouseDataInOutputBuffer() == FALSE)
		{
			bTemp = kGetKeyboardScanCode();
			
			// insert into Key Queue
			kConvertScanCodeAndPutQueue(bTemp);
		}
		else
		{
			// Keyboard or Mouse Data uses Output Buffer.
			//		so, kGetKeyboardScanCode() can be used to read Mouse Data
			bTemp = kGetKeyboardScanCode();

			// insert into Mouse Queue
			kAccumulateMouseDataAndPutQueue(bTemp);
		}
	}


	// get IRQ from Interrupt Vector
	iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

	// send EOI
	kSendEOI(iIRQ);

	// update Interrupt Occur Count
	kIncreaseInterruptCount(iIRQ);

	// Load Balancing
	kProcessLoadBalancing(iIRQ);
}


void kTimerHandler(int iVectorNumber)
{
	char vcBuffer[] = "[INT:  , ]";
	static int g_iTimerInterruptCount = 0;
	int iIRQ;
	BYTE bCurrentAPICID;

	
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	vcBuffer[8] = '0' + g_iTimerInterruptCount;
	g_iTimerInterruptCount = (g_iTimerInterruptCount + 1) % 10;

	kPrintStringXY(70, 0, vcBuffer);


	// get IRQ from Interrupt Vector
	iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

	// send EOI
	kSendEOI(iIRQ);

	// update Interrupt Occur Count
	kIncreaseInterruptCount(iIRQ);

	// IRQ 0 (PIT) handled on Bootstrap Processor only
	bCurrentAPICID = kGetAPICID();

	if(bCurrentAPICID == 0)
	{
		// increase Tick Count
		g_qwTickCount++;			// from Utility.h
	}

	// decrease Available Processor Time
	kDecreaseProcessorTime(bCurrentAPICID);

	// if Available Processor Time expired, switch Task
	if(kIsProcessorTimeExpired(bCurrentAPICID) == TRUE)
	{
		kScheduleInInterrupt();
	}
	
}


void kDeviceNotAvailableHandler(int iVectorNumber)
{
	TCB* pstFPUTask, *pstCurrentTask;
	QWORD qwLastFPUTaskID;
	BYTE bCurrentAPICID;

	
	// =============================================
	// FPU Exception Alert
	char vcBuffer[] = "[EXC:  , ]";
	static int g_iFPUInterruptCount = 0;

	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;
	vcBuffer[8] = '0' + g_iFPUInterruptCount;

	g_iFPUInterruptCount = (g_iFPUInterruptCount + 1) % 10;

	kPrintStringXY(0, 0, vcBuffer);
	// =============================================
	

	// get Local APIC ID of current Core
	bCurrentAPICID = kGetAPICID();
	
	
	// TS = 0
	// Added to find out first FPU usage point after Task Switching
	kClearTS();

	// ---------------------------------------------------------------------
	// check recent FPU Task. if exist, save FPU to Task
	// ---------------------------------------------------------------------
	qwLastFPUTaskID = kGetLastFPUUsedTaskID(bCurrentAPICID);
	pstCurrentTask  = kGetRunningTask(bCurrentAPICID);


	// if Last FPU used Task ID is itself, do nothing
	if(qwLastFPUTaskID == pstCurrentTask->stLink.qwID)
	{
		return;
	}
	// if previous FPU used Task exist, save FPU Context
	else if(qwLastFPUTaskID != TASK_INVALIDID)		// Save FPU when FPU Task exist
	{
		// Get TCB of Last FPU Task
		pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));

		// FPU Task TCB exist && Last FPU Task ID matches qwID in TCB
		if( (pstFPUTask != NULL) && (pstFPUTask->stLink.qwID == qwLastFPUTaskID) )
		{
			kSaveFPUContext(pstFPUTask->vqwFPUContext);
		}
	}
	// ---------------------------------------------------------------------

	
	// ---------------------------------------------------------------------
	// check current Task if has used FPU. 
	// if have not used, Initialize
	// else, restore FPU Context
	// ---------------------------------------------------------------------
	if(pstCurrentTask->bFPUUsed == FALSE)
	{
		kInitializeFPU();
		pstCurrentTask->bFPUUsed = TRUE;
	}
	else
	{
		kLoadFPUContext(pstCurrentTask->vqwFPUContext);
	}
	// ---------------------------------------------------------------------

	
	// LastFPUUsed = Current Task ID
	kSetLastFPUUsedTaskID(bCurrentAPICID, pstCurrentTask->stLink.qwID);
}


void kHDDHandler(int iVectorNumber)
{
	char vcBuffer[] = "[INT:  , ]";
	static int g_iHDDInterruptCount = 0;
	BYTE bTemp;
	int iIRQ;


	// get IRQ from Interrupt Vector
	iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;


	// ---------- HDD Interrupt Alert ----------
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	vcBuffer[8] = '0' + g_iHDDInterruptCount;
	g_iHDDInterruptCount = (g_iHDDInterruptCount + 1) % 10;
	
	
	kPrintStringXY(10, 0, vcBuffer);
	// -----------------------------------------
	

	// handle IRQ 14 (Primary PATA Port)
	if(iIRQ == 14)
	{
		kSetHDDInterruptFlag(TRUE, TRUE);
	}
	// handle IRQ 15 (Secondary PATA Port)
	else
	{
		kSetHDDInterruptFlag(FALSE, TRUE);
	}
	

	// send EOI
	kSendEOI(iIRQ);

	// update Interupt Occur Count
	kIncreaseInterruptCount(iIRQ);

	// Load Balancing
	kProcessLoadBalancing(iIRQ);
}


void kMouseHandler(int iVectorNumber)
{
	char vcBuffer[] = "[INT:  , ]";
	static int g_iMouseInterruptCount = 0;
	BYTE bTemp;
	int iIRQ;


	// =================================================
	// print Interrupt Message
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	vcBuffer[8] = '0' + g_iMouseInterruptCount;
	g_iMouseInterruptCount = (g_iMouseInterruptCount + 1) % 10;

	kPrintStringXY(0, 0, vcBuffer);
	// =================================================
	

	if(kIsOutputBufferFull() == TRUE)
	{
		// check if received Data is of Mouse or Keyboard
		if(kIsMouseDataInOutputBuffer() == FALSE)
		{
			bTemp = kGetKeyboardScanCode();

			// insert into Key Queue
			kConvertScanCodeAndPutQueue(bTemp);
		}
		else
		{
			// Keyboard and Mouse uses Output Buffer
			// 		so, kGetKeyboardScanCode() can be used to read Mouse Data
			bTemp = kGetKeyboardScanCode();

			// insert into Mouse Queue
			kAccumulateMouseDataAndPutQueue(bTemp);
		}
	}


	// get IRQ Number from Interrupt Vector
	iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;
	
	// send EOI
	kSendEOI(iIRQ);

	// update Interrupt Occurence Count
	kIncreaseInterruptCount(iIRQ);

	// Load Balancing
	kProcessLoadBalancing(iIRQ);
}



