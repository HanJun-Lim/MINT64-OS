#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"
#include "MultiProcessor.h"
#include "VBE.h"
#include "2DGraphics.h"
#include "MPConfigurationTable.h"
#include "Mouse.h"
#include "WindowManagerTask.h"
#include "SystemCall.h"



// Main() for Application Processor
void MainForApplicationProcessor(void);

// Change to Multicore / Multiprocessor Mode
BOOL kChangeToMultiCoreMode(void);

// Function for Graphic Mode Test
void kStartGraphicModeTest(void);



void Main(void)
{
	int iCursorX, iCursorY;
	BYTE bButton;
	int iX;
	int iY;


	// read BSP Flag in BootLoader.asm
	// if Flag means Application Processor, go to MainForApplicationProcessor()
	if(*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0)
	{
		MainForApplicationProcessor();
	}


	// Bootstrap Processor boot complete.
	// so, set Flag as Application Processor
	*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) = 0;


	// initialize Console first, then execute works below
	kInitializeConsole(0, 10);
	
	kPrintf("Switch to IA-32e Mode Success\n");
	kPrintf("IA-32e C Language Kernel Start..............[Pass]\n");
	kPrintf("Initialize Console..........................[Pass]\n");


	
	// print Boot status
	kGetCursor(&iCursorX, &iCursorY);
	kPrintf("GDT Initialize And Switch For IA-32e Mode...[    ]");
	kInitializeGDTTableAndTSS();
	kLoadGDTR(GDTR_STARTADDRESS);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	kPrintf("TSS Segment Load............................[    ]");
	kLoadTR(GDT_TSSSEGMENT);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	kPrintf("IDT Initialize..............................[    ]");
	kInitializeIDTTables();
	kLoadIDTR(IDTR_STARTADDRESS);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	kPrintf("Total RAM Size Check........................[    ]");
	kCheckTotalRAMSize();
	kSetCursor(45, iCursorY++);
	kPrintf("Pass], Size = %d MB\n", kGetTotalRAMSize());



	kPrintf("TCB Pool And Scheduler Initialize...........[Pass]\n");
	iCursorY++;
	kInitializeScheduler();
	kInitializePIT(MSTOCOUNT(1), 1);		// 1 Interrupt per 1ms (1ms, periodic)



	kPrintf("Dynamic Memory Initialize...................[Pass]\n");
	iCursorY++;
	kInitializeDynamicMemory();



	kPrintf("Keyboard Activate And Queue Initialize......[    ]");
	
	// Activate Keyboard
	if(kInitializeKeyboard() == TRUE)
	{
		kSetCursor(45, iCursorY++);
		kPrintf("Pass\n");
		kChangeKeyboardLED(FALSE, FALSE, FALSE);
	}
	else
	{
		kSetCursor(45, iCursorY++);
		kPrintf("Fail\n");
		while(1);
	}



	kPrintf("Mouse Activate And Queue Initialize.........[    ]");
	
	// Activate Mouse
	if(kInitializeMouse() == TRUE)
	{
		kEnableMouseInterrupt();

		kSetCursor(45, iCursorY++);
		kPrintf("Pass\n");
	}
	else
	{
		kSetCursor(45, iCursorY++);
		kPrintf("Fail\n");

		while(1);
	}



	// Initialize PIC Controller / Activate all interrupt
	kPrintf("PIC Controller And Interrupt Initialize.....[    ]");
	kInitializePIC();
	kMaskPICInterrupt(0);
	kEnableInterrupt();
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");


	/*
	// Initialize Hard Disk
	kPrintf("HDD Initialize..............................[    ]");
	if(kInitializeHDD() == TRUE)
	{
		kSetCursor(45, iCursorY++);
		kPrintf("Pass\n");
	}
	else
	{
		kSetCursor(45, iCursorY++);
		kPrintf("Fail\n");
	}
	*/


	// Initialize File System
	kPrintf("File System Initialize......................[    ]");
	if(kInitializeFileSystem() == TRUE)
	{
		kSetCursor(45, iCursorY++);
		kPrintf("Pass\n");
	}
	else
	{
		kSetCursor(45, iCursorY++);
		kPrintf("Fail\n");
	}


	// Initialize Serial Port
	kPrintf("Serial Port Initialize......................[Pass]\n");
	iCursorY++;
	kInitializeSerialPort();



	// Change to Multicore Processor Mode
	//		activate AP, I/O Mode, Interrupt/Task Load Balancing
	kPrintf("Change To MultiCore Processor Mode..........[    ]");

	if(kChangeToMultiCoreMode() == TRUE)
	{
		kSetCursor(45, iCursorY++);
		kPrintf("Pass\n");
	}
	else
	{
		kSetCursor(45, iCursorY++);
		kPrintf("Fail\n");
	}



	// Initialize MSR Registers for System Call
	kPrintf("System Call MSR Initialize..................[Pass]\n");
	iCursorY++;
	kInitializeSystemCall();



	// Create Idle Task as System Thread, Start Shell
	kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | 
				TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask, kGetAPICID());



	// if not Graphic Mode, execute Console Shell (CUI Shell)
	if(*(BYTE*)VBE_STARTGRAPHICMODEFLAGADDRESS == 0)
	{
		kStartConsoleShell();
	}
	// if Graphic Mode, execute Window Manager Task (GUI Shell)
	else
	{
		//kStartGraphicModeTest();
		kStartWindowManager();
	}
}


void MainForApplicationProcessor(void)
{
	QWORD qwTickCount;

	
	// set GDT Table
	kLoadGDTR(GDTR_STARTADDRESS);
	

	// set TSS Descriptor
	// 		TSS Segment and Descriptor created as many as AP Count by Descriptor.c
	// 		so, allocate TSS Descriptor using APIC ID
	kLoadTR(GDT_TSSSEGMENT + (kGetAPICID() * sizeof(GDTENTRY16)));


	// set IDT Table
	kLoadIDTR(IDTR_STARTADDRESS);


	// initialize Scheduler
	kInitializeScheduler();


	// activate Local APIC of current Core
	kEnableSoftwareLocalAPIC();

	
	// set Task Priority Register to 0 for all Interrupt reception
	kSetTaskPriority(0);


	// initialize Local Vector Table of Local APIC
	kInitializeLocalVectorTable();


	// enable Interrupt
	kEnableInterrupt();


	// initialize MSR Registers for System Call
	kInitializeSystemCall();

	
	// print once for testing Symmetric I/O Mode after AP starts
	//kPrintf("Application Processor [APIC ID: %d] Is Activated\n", kGetAPICID());


	// execute Idle Task
	kIdleTask();
}



BOOL kChangeToMultiCoreMode(void)
{
	MPCONFIGURATIONMANAGER* pstMPManager;
	BOOL bInterruptFlag;
	int i;


	// activate AP
	if(kStartUpApplicationProcessor() == FALSE)
	{
		return FALSE;
	}


	// --------------------------------------------
	// Change to Symmetric I/O Mode
	// --------------------------------------------
	
	// check if current Mode is PIC Mode by referring MP Configuration Manager
	pstMPManager = kGetMPConfigurationManager();

	
	// if currently PIC Mode, need to deactivate PIC Mode
	if(pstMPManager->bUsePICMode == TRUE)
	{
		// send 0x70 to Port 0x22 first, and then send 0x01 to Port 0x23 to access IMCR Register
		kOutPortByte(0x22, 0x70);
		kOutPortByte(0x23, 0x01);
	}


	// mask all Interrupt of PIC Controller not to occur Interrupt
	kMaskPICInterrupt(0xFFFF);


	// activate Local APIC of all Core
	kEnableGlobalLocalAPIC();


	// activate Local APIC of current Core
	kEnableSoftwareLocalAPIC();


	// ---------- disable Interrupt ----------
	bInterruptFlag = kSetInterruptFlag(FALSE);


	// set Task Priority Register as 0 to receive all Interrupt
	kSetTaskPriority(0);

	
	// initialize Local Vector Table of Local APIC
	kInitializeLocalVectorTable();


	// set Symmetric I/O Mode
	kSetSymmetricIOMode(TRUE);


	// initialize I/O APIC
	kInitializeIORedirectionTable();


	// ---------- restore previous Interrupt Flag ----------
	kSetInterruptFlag(bInterruptFlag);


	// activate Interrupt Load Balancing
	kSetInterruptLoadBalancing(TRUE);


	// activate Task Load Balancing
	for(i=0; i<MAXPROCESSORCOUNT; i++)
	{
		kSetTaskLoadBalancing(i, TRUE);
	}

	
	return TRUE;
}



// Get absoulte value of x
#define ABS(x)		( ((x) >= 0) ? (x) : -(x) )


// Return random X, Y pos
void kGetRandomXY(int* piX, int* piY)
{
	int iRandom;

	
	// calculate X
	iRandom = kRandom();
	*piX = ABS(iRandom) % 1000;


	// calculate Y
	iRandom = kRandom();
	*piY = ABS(iRandom) % 700;
}


// Return random color
COLOR kGetRandomColor(void)
{
	int iR, iG, iB;
	int iRandom;

	
	iRandom = kRandom();
	iR = ABS(iRandom) % 256;

	iRandom = kRandom();
	iG = ABS(iRandom) % 256;

	iRandom = kRandom();
	iB = ABS(iRandom) % 256;


	return RGB(iR, iG, iB);
}


/*
// Draw window frame
void kDrawWindowFrame(int iX, int iY, int iWidth, int iHeight, const char* pcTitle)
{
	char* pcTestString1 = "This is window prototype of MINT64 OS";
	char* pcTestString2 = "Coming soon";
	VBEMODEINFOBLOCK* pstVBEMode;
	COLOR* pstVideoMemory;
	RECT stScreenArea;


	// get VBE Mode Information Block
	pstVBEMode = kGetVBEModeInfoBlock();


	// set Screen area
	stScreenArea.iX1 = 0;
	stScreenArea.iY1 = 0;
	stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
	stScreenArea.iY2 = pstVBEMode->wYResolution - 1;


	// set Graphic Memory Address
	pstVideoMemory = (COLOR*)((QWORD)pstVBEMode->dwPhysicalBasePointer & 0xFFFFFFFF);


	// ----- draw Window Layout, Width : 2 pixel -----
	kInternalDrawRect(&stScreenArea, pstVideoMemory, 
					  iX	, iY	, iX + iWidth	 , iY + iHeight	   , RGB(109, 218, 22), FALSE);
	kInternalDrawRect(&stScreenArea, pstVideoMemory, 
					  iX + 1, iY + 1, iX + iWidth - 1, iY + iHeight - 1, RGB(109, 218, 22), FALSE);

	

	// ----- draw Title bar -----
	kInternalDrawRect(&stScreenArea, pstVideoMemory, 
					  iX, iY + 3, iX + iWidth - 1, iY + 21, RGB(79, 204, 11), TRUE);

	
	// print Title
	kInternalDrawText(&stScreenArea, pstVideoMemory, 
					  iX + 6, iY + 3, RGB(255, 255, 255), RGB(79, 204, 11), pcTitle, kStrLen(pcTitle));


	// draw Title bar effect
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + 1, iY + 1, iX + iWidth - 1, iY + 1 , RGB(183, 249, 171));
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + 1, iY + 2, iX + iWidth - 1, iY + 2 , RGB(150, 210, 140));

	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + 1, iY + 2, iX + 1, iY + 20, RGB(183, 249, 171));
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + 2, iY + 2, iX + 2, iY + 20, RGB(150, 210, 140));

	
	// draw line below Title bar
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + 2, iY + 19, iX + iWidth - 2, iY + 19, RGB(46, 59, 30));
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + 2, iY + 20, iX + iWidth - 2, iY + 20, RGB(46, 59, 30));



	// ----- draw Close Button, located at right top -----
	kInternalDrawRect(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18, iY + 1, iX + iWidth - 2, iY + 19, RGB(255, 255, 255), TRUE);


	// draw Close Button effect : 2 pixel
	kInternalDrawRect(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2		  , iY + 1	   , iX + iWidth - 2	, iY + 19 - 1, RGB(86, 86, 86), TRUE);
	kInternalDrawRect(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 1	  , iY + 1	   , iX + iWidth - 2 - 1, iY + 19 - 1, RGB(86, 86, 86), TRUE);
	kInternalDrawRect(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18 + 1, iY + 19	   , iX + iWidth - 2	, iY + 19	 , RGB(86, 86, 86), TRUE);
	kInternalDrawRect(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18 + 1, iY + 19 - 1, iX + iWidth - 2	, iY + 19 - 1, RGB(86, 86, 86), TRUE);

	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18	  , iY + 1	  , iX + iWidth - 2 - 1		, iY + 1	 , RGB(229, 229, 229));
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18	  , iY + 1 + 1, iX + iWidth - 2 - 2		, iY + 1 + 1 , RGB(229, 229, 229));
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18	  , iY + 1	  , iX + iWidth - 2 - 18	, iY + 19	 , RGB(229, 229, 229));
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18 + 1, iY + 1	  , iX + iWidth - 2 - 18 + 1, iY + 19 - 1, RGB(229, 229, 229));

	
	// draw 'X', Width : 3 pixel
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18 + 4, iY + 1 + 4 , iX + iWidth - 2 - 4, iY + 19 - 4, RGB(71, 199, 21));
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18 + 5, iY + 1 + 4 , iX + iWidth - 2 - 4, iY + 19 - 5, RGB(71, 199, 21));
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18 + 4, iY + 1 + 5 , iX + iWidth - 2 - 5, iY + 19 - 4, RGB(71, 199, 21));
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18 + 4, iY + 19 - 4, iX + iWidth - 2 - 4, iY + 1 + 4 , RGB(71, 199, 21));
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18 + 5, iY + 19 - 4, iX + iWidth - 2 - 4, iY + 1 + 5 , RGB(71, 199, 21));
	kInternalDrawLine(&stScreenArea, pstVideoMemory, 
					  iX + iWidth - 2 - 18 + 4, iY + 19 - 5, iX + iWidth - 2 - 5, iY + 1 + 4 , RGB(71, 199, 21));



	// ----- draw inside -----
	kInternalDrawRect(&stScreenArea, pstVideoMemory, 
					  iX + 2, iY + 21, iX + iWidth - 2, iY + iHeight - 2, RGB(255, 255, 255), TRUE);

	
	// print Test String
	kInternalDrawText(&stScreenArea, pstVideoMemory, 
					  iX + 10, iY + 30, RGB(0, 0, 0), RGB(255, 255, 255), pcTestString1, kStrLen(pcTestString1));
	kInternalDrawText(&stScreenArea, pstVideoMemory, 
					  iX + 10, iY + 50, RGB(0, 0, 0), RGB(255, 255, 255), pcTestString2, kStrLen(pcTestString2));
}



// Added for Mouse Cursor

// width, height of Mouse (20*20)
#define MOUSE_CURSOR_WIDTH		20
#define MOUSE_CURSOR_HEIGHT		20


// image of Mouse Cursor
BYTE gs_vwMouseBuffer[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT] =
{
	1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1, 0, 0,
	0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 3, 2, 1, 1, 2, 3, 2, 2, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 2, 2, 1, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0,
	0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0,
	0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
};


// color of Cursor image
#define MOUSE_CURSOR_OUTERLINE		RGB(  0,   0,   0)
#define MOUSE_CURSOR_OUTER			RGB( 79, 204,  11)
#define MOUSE_CURSOR_INNER			RGB(232, 255, 232)


// print Mouse Cursor at (X, Y)
void kDrawCursor(RECT* pstArea, COLOR* pstVideoMemory, int iX, int iY)
{
	int i;
	int j;
	BYTE* pbCurrentPos;


	// set start pos of Cursor Data
	pbCurrentPos = gs_vwMouseBuffer;

	
	// print Pixel of Mouse Data
	for(j=0; j<MOUSE_CURSOR_HEIGHT; j++)
	{
		for(i=0; i<MOUSE_CURSOR_WIDTH; i++)
		{
			switch(*pbCurrentPos)
			{
				// if 0, No print
				case 0:
					break;

				// if 1, print outer line
				case 1:
					kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY, MOUSE_CURSOR_OUTERLINE);

					break;

				// if 2, print boundary line between outer line and inner color
				case 2:
					kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY, MOUSE_CURSOR_OUTER);

					break;
				
				// if 3, print inner cursor color
				case 3:
					kInternalDrawPixel(pstArea, pstVideoMemory, i + iX, j + iY, MOUSE_CURSOR_INNER);

					break;
			}

			// move pos of current Cursor Data
			pbCurrentPos++;
		}
	}
}



// Graphic Mode Test
void kStartGraphicModeTest(void)
{
	VBEMODEINFOBLOCK* pstVBEMode;
	int iX, iY;
	COLOR* pstVideoMemory;
	RECT stScreenArea;
	int iRelativeX, iRelativeY;
	BYTE bButton;


	// get VBE Mode Information Block
	pstVBEMode = kGetVBEModeInfoBlock();


	// set Screen area
	stScreenArea.iX1 = 0;
	stScreenArea.iY1 = 0;
	stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
	stScreenArea.iY2 = pstVBEMode->wYResolution - 1;


	// set Graphic Memory Address
	pstVideoMemory = (COLOR*)pstVBEMode->dwPhysicalBasePointer;

	
	// set initial pos of Mouse as center of screen
	iX = pstVBEMode->wXResolution / 2;
	iY = pstVBEMode->wYResolution / 2;


	// --------------------------------------------------------
	// print Mouse Cursor, handle Mouse movement
	// --------------------------------------------------------
	
	// print background
	kInternalDrawRect(&stScreenArea, pstVideoMemory, 
					  stScreenArea.iX1, stScreenArea.iY1, stScreenArea.iX2, stScreenArea.iY2,
					  RGB(232, 255, 232), TRUE);

	// print Mouse Cursor at current pos
	kDrawCursor(&stScreenArea, pstVideoMemory, iX, iY);


	while(1)
	{
		// wait for Mouse Data
		if(kGetMouseDataFromMouseQueue(&bButton, &iRelativeX, &iRelativeY) == FALSE)
		{
			kSleep(0);
			continue;
		}

		
		// print background at the position of previous Mouse Cursor
		kInternalDrawRect(&stScreenArea, pstVideoMemory, 
						  iX, iY, iX + MOUSE_CURSOR_WIDTH, iY + MOUSE_CURSOR_HEIGHT, 
						  RGB(232, 255, 232), TRUE);

		
		// calculate current position (current pos = previous pos + distance of Mouse movement)
		iX += iRelativeX;
		iY += iRelativeY;


		// make Mouse Cursor not to be out of Screen
		if(iX < stScreenArea.iX1)
		{
			iX = stScreenArea.iX1;
		}
		else if(iX > stScreenArea.iX2)
		{
			iX = stScreenArea.iX2;
		}
		
		if(iY < stScreenArea.iY1)
		{
			iY = stScreenArea.iY1;
		}
		else if(iY > stScreenArea.iY2)
		{
			iY = stScreenArea.iY2;
		}


		// print Window prototype when left Button pushed
		if(bButton & MOUSE_LBUTTONDOWN)
		{
			kDrawWindowFrame(iX - 10, iY - 10, 400, 200, "MINT64 OS Test Window");
		}
		// print background when right Button pushed
		else if(bButton & MOUSE_RBUTTONDOWN)
		{
			kInternalDrawRect(&stScreenArea, pstVideoMemory,
							  stScreenArea.iX1, stScreenArea.iY1, stScreenArea.iX2, stScreenArea.iY2,
							  RGB(232, 255, 232), TRUE);
		}
		

		// print Mouse Cursor image at new Mouse position
		kDrawCursor(&stScreenArea, pstVideoMemory, iX, iY);
	}
}


*/
