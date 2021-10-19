#include "Mouse.h"
#include "Keyboard.h"
#include "Queue.h"
#include "AssemblyUtility.h"


// manage Mouse Status
static MOUSEMANAGER gs_stMouseManager = {0, };

// define Mouse Queue, Mouse Queue Buffer
static QUEUE 	 gs_stMouseQueue;
static MOUSEDATA gs_vstMouseQueueBuffer[MOUSE_MAXQUEUECOUNT];



BOOL kInitializeMouse(void)
{
	// init Queue
	kInitializeQueue(&gs_stMouseQueue, gs_vstMouseQueueBuffer, MOUSE_MAXQUEUECOUNT, sizeof(MOUSEDATA));


	// init SpinLock
	kInitializeSpinLock(&(gs_stMouseManager.stSpinLock));


	// activate Mouse
	if(kActivateMouse() == TRUE)
	{
		// enable Mouse Interrupt
		kEnableMouseInterrupt();

		return TRUE;
	}

	return FALSE;
}


BOOL kActivateMouse(void)
{
	int i, j;
	BOOL bPreviousInterrupt;
	BOOL bResult;


	////////////////////////////////////
	// disable Interrupt
	bPreviousInterrupt = kSetInterruptFlag(FALSE);
	////////////////////////////////////
	
	
	// activate Mouse Device by sending 0xA8 (Mouse Activation Command) to Port 0x64 (Control Register)
	kOutPortByte(0x64, 0xA8);


	// send 0xD4 (Sending Data to Mouse Command) to Port 0x64 (Control Register)
	// 		- I'm going to send Input Buffer Data (1 byte) to Mouse, not Keyboard
	kOutPortByte(0x64, 0xD4);


	// wait for Input Buffer (Port 0x60) empty. if empty, send Activation Command to Mouse
	// 		sufficient wait time : loop for 0xFFFF
	for(i=0; i<0xFFFF; i++)
	{
		// if Input Buffer empty, Keyboard Command can be sent
		if(kIsInputBufferFull() == FALSE)
		{
			break;
		}
	}


	// send 0xF4 (Activate Mouse Command) to Input Buffer (Port 0x60 - Mouse)
	kOutPortByte(0x60, 0xF4);


	// wait for ACK
	bResult = kWaitForACKAndPutOtherScanCode();


	////////////////////////////////////
	// restore previous Interrupt status
	kSetInterruptFlag(bPreviousInterrupt);
	////////////////////////////////////
	

	return bResult;
}


void kEnableMouseInterrupt(void)
{
	BYTE bOutputPortData;
	int i;


	// read Command Byte (Contents in Command Register - Port 0x64)
	//		by sending 0x20 (Read Command Byte (PS/2 Controller Configuration Byte) Command)
	kOutPortByte(0x64, 0x20);


	// wait for Output Port Data
	for(i=0; i<0xFFFF; i++)
	{
		// if Output Buffer(Port 0x60) full, Data can be read
		if(kIsOutputBufferFull() == TRUE)
		{
			break;
		}
	}


	// read Command Byte value
	bOutputPortData = kInPortByte(0x60);


	// set Mouse Interrupt Bit, and then send Command Byte
	// 		Below 2 bytes of Command Byte are used to enable Keyboard/Mouse Interrupt
	// 		Bit 0 : Keyboard, Bit 1 : Mouse
	bOutputPortData |= 0x02;


	// send Write Command Byte Command (0x60) to Command Register (Port 0x64)
	kOutPortByte(0x64, 0x60);


	// wait for Input Buffer empty
	for(i=0; i<0xFFFF; i++)
	{
		if(kIsInputBufferFull() == FALSE)
		{
			break;
		}
	}

	
	// send bOutputPortData to Input Buffer (Port 0x60)
	kOutPortByte(0x60, bOutputPortData);
}


// Gather Mouse Packet and insert them into Queue
BOOL kAccumulateMouseDataAndPutQueue(BYTE bMouseData)
{
	BOOL bResult;


	// branch code by Number of received byte
	switch(gs_stMouseManager.iByteCount)
	{
		case 0:
			gs_stMouseManager.stCurrentData.bButtonStatusAndFlag = bMouseData;
			gs_stMouseManager.iByteCount++;

			break;

		case 1:
			gs_stMouseManager.stCurrentData.bXMovement = bMouseData;
			gs_stMouseManager.iByteCount++;

			break;

		case 2:
			gs_stMouseManager.stCurrentData.bYMovement = bMouseData;
			gs_stMouseManager.iByteCount++;

			break;

		default:
			gs_stMouseManager.iByteCount = 0;

			break;
	}


	// if 3 bytes all received, 
	// 		insert them into Mouse Queue, and init Number of received byte
	if(gs_stMouseManager.iByteCount >= 3)
	{
		//////////////////////////////////
		// CRITICAL SECTION START
		kLockForSpinLock(&(gs_stMouseManager.stSpinLock));
		//////////////////////////////////


		// insert Mouse Packet Data into Mouse Queue
		bResult = kPutQueue(&gs_stMouseQueue, &gs_stMouseManager.stCurrentData);


		//////////////////////////////////
		// CRITICAL SECTION END
		kUnlockForSpinLock(&(gs_stMouseManager.stSpinLock));
		//////////////////////////////////

		
		// init Number of received byte
		gs_stMouseManager.iByteCount = 0;
	}


	return bResult;
}


// Get Mouse Data from Mouse Queue
BOOL kGetMouseDataFromMouseQueue(BYTE* pbButtonStatus, int* piRelativeX, int* piRelativeY)
{
	MOUSEDATA stData;
	BOOL bResult;

	
	// if Queue is empty, cannot get Data
	if(kIsQueueEmpty(&(gs_stMouseQueue)) == TRUE)
	{
		return FALSE;
	}


	/////////////////////////////////////
	// CRITICAL SECTION START
	kLockForSpinLock(&(gs_stMouseManager.stSpinLock));
	/////////////////////////////////////

	// get Data from Queue
	bResult = kGetQueue(&(gs_stMouseQueue), &stData);

	/////////////////////////////////////
	// CRITICAL SECTION END
	kUnlockForSpinLock(&(gs_stMouseManager.stSpinLock));
	/////////////////////////////////////
	

	// fail if failed to get Data
	if(bResult == FALSE)
	{
		return FALSE;
	}


	// analyze Mouse Data (Packet)
	// 		1. Mouse Button Status Flag located at Bit 0~2 on Byte 1
	*pbButtonStatus = stData.bButtonStatusAndFlag & 0x7;


	// 		2. set Distance of X Movement
	// 		   X Sign Bit at Bit 4, set Bit means negative
	*piRelativeX = stData.bXMovement & 0xFF;

	if(stData.bButtonStatusAndFlag & 0x10)
	{
		// make upper bits set
		*piRelativeX |= (0xFFFFFF00);
	}


	//		3. set Distance of Y Movement
	//		   Y Sign Bit at Bit 5, set Bit means negative
	*piRelativeY = stData.bYMovement & 0xFF;

	if(stData.bButtonStatusAndFlag & 0x20)
	{
		// make upper bits set
		*piRelativeY |= (0xFFFFFF00);
	}


	// +/- Direction of Y is opposite to Y Coordinate on Screen
	*piRelativeY = -*piRelativeY;
	

	return TRUE;
}


BOOL kIsMouseDataInOutputBuffer(void)
{
	// To verify Mouse Data, read Status Register (Port 0x64) before read Output Buffer (Port 0x60)
	// 		verify by checking AUXB field (Bit 5)
	if(kInPortByte(0x64) & 0x20)
	{
		return TRUE;
	}


	return FALSE;
}



