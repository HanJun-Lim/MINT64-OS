#ifndef __MOUSE_H__
#define __MOUSE_H__

#include "Types.h"
#include "Synchronization.h"


// --------------- Macro ---------------

// for Mouse Queue

#define MOUSE_MAXQUEUECOUNT			100

// for Mouse Button Status

#define MOUSE_LBUTTONDOWN			0x01
#define MOUSE_RBUTTONDOWN			0x02
#define MOUSE_MBUTTONDOWN			0x04



// --------------- Structure --------------- 

#pragma pack(push, 1)


// for PS/2 Mouse Packet, Data stored into Mouse Queue
typedef struct kMousePacketStruct
{
	// Button Status, X coord, Y coord
	BYTE bButtonStatusAndFlag;

	// Distance of X movement
	BYTE bXMovement;

	// Distance of Y movement
	BYTE bYMovement;
} MOUSEDATA;


#pragma pack(pop)


// for Mouse Status Management
typedef struct kMouseManagerStruct
{
	// for Synchronization
	SPINLOCK stSpinLock;

	// Count of received byte, value range is 0~2
	int iByteCount;

	// Mouse Data which is currently receiving
	MOUSEDATA stCurrentData;
} MOUSEMANAGER;



// --------------- Function --------------- 

BOOL kInitializeMouse(void);
BOOL kAccumulateMouseDataAndPutQueue(BYTE bMouseData);
BOOL kActivateMouse(void);
void kEnableMouseInterrupt(void);
BOOL kIsMouseDataInOutputBuffer(void);
BOOL kGetMouseDataFromMouseQueue(BYTE* pbButtonStatus, int* piRelativeX, int* piRelativeY);



#endif
