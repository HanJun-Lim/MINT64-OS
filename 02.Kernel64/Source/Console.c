#include <stdarg.h>
#include "Console.h"



// for Console Information Management
CONSOLEMANAGER gs_stConsoleManager = {0, };

// Screen Buffer for Graphic Mode
static CHARACTER gs_vstScreenBuffer[CONSOLE_WIDTH * CONSOLE_HEIGHT];

// send Key Event in GUI Console Shell Window to Console Shell Task
static KEYDATA gs_vstKeyQueueBuffer[CONSOLE_GUIKEYQUEUE_MAXCOUNT];



void kInitializeConsole(int iX, int iY)
{
	kMemSet(&gs_stConsoleManager, 0, sizeof(gs_stConsoleManager));		// init Console Manager
	kMemSet(&gs_vstScreenBuffer, 0, sizeof(gs_vstScreenBuffer));		// init Screen Buffer


	// if the System starts with Text Mode, Screen Buffer is Video Memory (0xB8000)
	if(kIsGraphicMode() == FALSE)
	{
		gs_stConsoleManager.pstScreenBuffer = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	}

	// if the System starts with Graphic Mode, Screen Buffer is GUI Screen Buffer
	else
	{
		gs_stConsoleManager.pstScreenBuffer = gs_vstScreenBuffer;

		
		// also init Key Queue, Mutex
		kInitializeQueue(&(gs_stConsoleManager.stKeyQueueForGUI), gs_vstKeyQueueBuffer, 
						 CONSOLE_GUIKEYQUEUE_MAXCOUNT, sizeof(KEYDATA));
		kInitializeMutex(&(gs_stConsoleManager.stLock));
	}


	// set Cursor position
	kSetCursor(iX, iY);
}


// set Cursor Position
// 		and set Character Input Position
void kSetCursor(int iX, int iY)
{
	int iLinearValue;
	int iOldX;
	int iOldY;
	int i;

	
	// calculate Cursor Pos
	iLinearValue = iY * CONSOLE_WIDTH + iX;

	
	// if the System starts with Text Mode, send Cursor Pos to CRT Controller
	if(kIsGraphicMode() == FALSE)
	{
		// send 0x0E to CRTC Control Address Register (Port : 0x3D4) to select UPPERCURSOR Register
		kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
	
		// send Upper Cursor Byte to CRTC Controll Data Register (Port : 0x3D5)
		kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);
	
	
		// send 0x0F to CRTC Control Address Register (Port : 0x3D4) to select LOWERCURSOR Register
		kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
	
		// send Lower Cursor Byte to CRTC Controll Data Register (Port : 0x3D5)
		kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xFF);
	}

	// if the System starts with Graphic Mode, move Cursor Pos
	else
	{
		// erase previous Cursor
		for(i=0; i<CONSOLE_WIDTH * CONSOLE_HEIGHT; i++)
		{
			// if the Cursor exist, erase it
			if( (gs_stConsoleManager.pstScreenBuffer[i].bCharactor == '_') &&
				(gs_stConsoleManager.pstScreenBuffer[i].bAttribute == 0x00) )
			{
				gs_stConsoleManager.pstScreenBuffer[i].bCharactor = ' ';
				gs_stConsoleManager.pstScreenBuffer[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;

				break;
			}
		}


		// print Cursor at new position
		gs_stConsoleManager.pstScreenBuffer[iLinearValue].bCharactor = '_';
		gs_stConsoleManager.pstScreenBuffer[iLinearValue].bAttribute = 0x00;
	}


	// update Print Position
	gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;
}


// return Current Cursor Position
void kGetCursor(int* piX, int* piY)
{
	*piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
	*piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}


void kPrintf(const char* pcFormatString, ... )
{
	va_list ap;						// type to hold information about variable arguments
	char	vcBuffer[1024];
	int 	iNextPrintOffset;


	// handling with vsprintf() by using va_list
	va_start(ap, pcFormatString);					// sets pointer of variable argument list
	kVSPrintf(vcBuffer, pcFormatString, ap);
	va_end(ap);										// sets pointer of variable argument list to NULL

	// print Format String to screen
	iNextPrintOffset = kConsolePrintString(vcBuffer);
	
	// update Cursor Pos
	kSetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}


int kConsolePrintString(const char* pcBuffer)		// returns next print offset value to set cursor
{
	CHARACTER* pstScreen;
	int i,j;
	int iLength;
	int iPrintOffset;


	// set Screen Buffer
	pstScreen = gs_stConsoleManager.pstScreenBuffer;

	
	// get print position
	iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;

	iLength = kStrLen(pcBuffer);

	
	// print string
	for(i=0; i<iLength; i++)
	{
		// carriage return
		if(pcBuffer[i] == '\n')
		{
			iPrintOffset += (CONSOLE_WIDTH - (iPrintOffset % CONSOLE_WIDTH));
		}
		// tab
		else if(pcBuffer[i] == '\t')
		{
			iPrintOffset += (8 - (iPrintOffset % 8));
		}
		// normal string
		else
		{
			pstScreen[iPrintOffset].bCharactor = pcBuffer[i];
			pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
			iPrintOffset++;
		}

		// scroll if print position over max value of screen (80 * 25)
		if(iPrintOffset >= (CONSOLE_HEIGHT * CONSOLE_WIDTH))
		{
			// copy lines except first one to each upper line
			kMemCpy(pstScreen, pstScreen + CONSOLE_WIDTH, (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(CHARACTER));


			// last line filled with ' '
			for(j=(CONSOLE_HEIGHT- 1 ) * (CONSOLE_WIDTH); j<(CONSOLE_HEIGHT * CONSOLE_WIDTH); j++)
			{
				pstScreen[j].bCharactor = ' ';
				pstScreen[j].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
			}

			iPrintOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
		}	
	}

	return iPrintOffset;
}


void kClearScreen(void)
{
	CHARACTER* pstScreen;
	int i;


	// set Screen Buffer
	pstScreen = gs_stConsoleManager.pstScreenBuffer;


	for(i=0; i<CONSOLE_WIDTH * CONSOLE_HEIGHT; i++)
	{
		pstScreen[i].bCharactor = ' ';
		pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}

	kSetCursor(0,0);
}


BYTE kGetCh(void)
{
	KEYDATA stData;
	
	while(1)
	{
		// if Text Mode, get Key Data from Key Queue of Kernel
		if(kIsGraphicMode() == FALSE)
		{
			while(kGetKeyFromKeyQueue(&stData) == FALSE)
			{
				kSchedule();
			}
		}
		else
		{
			while(kGetKeyFromGUIKeyQueue(&stData) == FALSE)
			{
				// if Shell Task need to exit, break loop
				if(gs_stConsoleManager.bExit == TRUE)
				{
					return 0xFF;
				}

				kSchedule();
			}
		}


		// if KEYDOWN Flag, return ASCII Code
		if(stData.bFlags == KEY_FLAGS_DOWN)
		{
			return stData.bASCIICode;
		}
	}
}


void kPrintStringXY(int iX, int iY, const char* pcString)
{
	CHARACTER* pstScreen;
	int i;


	// set Screen Buffer
	pstScreen = gs_stConsoleManager.pstScreenBuffer;


	// calculate print position
	pstScreen += (iY * 80) + iX;

	
	for(i=0; pcString[i] != 0; i++)
	{
		pstScreen[i].bCharactor = pcString[i];
		pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}
}



CONSOLEMANAGER* kGetConsoleManager(void)
{
	return &gs_stConsoleManager;
}


BOOL kGetKeyFromGUIKeyQueue(KEYDATA* pstData)
{
	BOOL bResult;


	if(kIsQueueEmpty(&(gs_stConsoleManager.stKeyQueueForGUI)) == TRUE)
	{
		return FALSE;
	}


	///////////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stConsoleManager.stLock));
	///////////////////////////////////////////
	
	// get Data from Queue
	bResult = kGetQueue(&(gs_stConsoleManager.stKeyQueueForGUI), pstData);

	///////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stConsoleManager.stLock));
	///////////////////////////////////////////


	return bResult;
}


BOOL kPutKeyToGUIKeyQueue(KEYDATA* pstData)
{
	BOOL bResult;


	if(kIsQueueFull(&(gs_stConsoleManager.stKeyQueueForGUI)) == TRUE)
	{
		return FALSE;
	}


	///////////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stConsoleManager.stLock));
	///////////////////////////////////////////
	
	bResult = kPutQueue(&(gs_stConsoleManager.stKeyQueueForGUI), pstData);

	///////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stConsoleManager.stLock));
	///////////////////////////////////////////


	return bResult;
}


void kSetConsoleShellExitFlag(BOOL bFlag)
{
	gs_stConsoleManager.bExit = bFlag;
}



