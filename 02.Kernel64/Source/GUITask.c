#include "GUITask.h"
#include "Window.h"
#include "MultiProcessor.h"
#include "Font.h"
#include "InterruptHandler.h"
#include "Console.h"
#include "Task.h"
#include "ConsoleShell.h"
#include "FileSystem.h"
#include "JPEG.h"
#include "DynamicMemory.h"



// ---------------------------------------------
// Base GUI Task
// ---------------------------------------------

void kBaseGUITask(void)
{
	QWORD qwWindowID;
	int iMouseX, iMouseY;
	int iWindowWidth, iWindowHeight;
	EVENT stReceivedEvent;
	MOUSEEVENT* pstMouseEvent;
	KEYEVENT* pstKeyEvent;
	WINDOWEVENT* pstWindowEvent;

	
	// ========== judge Graphic Mode ==========
	
	if(kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI Mode\n");

		return;
	}


	// ========== create Window ==========
	
	kGetCursorPosition(&iMouseX, &iMouseY);


	// set Window size, Window Title
	iWindowWidth = 500;
	iWindowHeight = 200;


	// call kCreatwWindow(), Window location based on Mouse Cursor
	qwWindowID = kCreateWindow(iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2, iWindowWidth, iWindowHeight,
							   WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE, "Hello World Window");

	if(qwWindowID == WINDOW_INVALIDID)
	{
		return;
	}


	// ========== loop for Event Handling ==========
	
	while(1)
	{
		// get Event in Event Queue
		if(kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			kSleep(0);

			continue;
		}


		// handle Event by Event Type
		switch(stReceivedEvent.qwType)
		{
			// handle Mouse Event
			case EVENT_MOUSE_MOVE:
			case EVENT_MOUSE_LBUTTONDOWN:
			case EVENT_MOUSE_LBUTTONUP:
			case EVENT_MOUSE_RBUTTONDOWN:
			case EVENT_MOUSE_RBUTTONUP:
			case EVENT_MOUSE_MBUTTONDOWN:
			case EVENT_MOUSE_MBUTTONUP:

				// insert Mouse Event handling code here
				pstMouseEvent = &(stReceivedEvent.stMouseEvent);
				
				break;


			// handle Key Event
			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:

				// insert Key Event handling code here
				pstKeyEvent = &(stReceivedEvent.stKeyEvent);
				
				break;

			
			// handle Window Event
			case EVENT_WINDOW_SELECT:
			case EVENT_WINDOW_DESELECT:
			case EVENT_WINDOW_MOVE:
			case EVENT_WINDOW_RESIZE:
			case EVENT_WINDOW_CLOSE:

				// insert Window Event handling code here
				pstWindowEvent = &(stReceivedEvent.stWindowEvent);


				// ******************************************************
				// if Received Event is CLOSE, 
				// 		delete Window and end Task
				// ******************************************************
				if(stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
				{
					kDeleteWindow(qwWindowID);
					return;
				}

				break;
			
			
			default:

				// insert unknown Event handling code here
				break;
		}

	}	// while(1)

}


// ---------------------------------------------
// Hello World GUI Task
// ---------------------------------------------

void kHelloWorldGUITask(void)
{
	QWORD qwWindowID;
	int iMouseX, iMouseY;
	int iWindowWidth, iWindowHeight;
	EVENT stReceivedEvent;
	MOUSEEVENT* pstMouseEvent;
	KEYEVENT* pstKeyEvent;
	WINDOWEVENT* pstWindowEvent;
	int iY;
	char vcTempBuffer[50];
	static int s_iWindowCount = 0;

	// Event Type String
	char* vpcEventString[] = {
		"Unknown"			,
		"MOUSE_MOVE"		,
		"MOUSE_LBUTTONDOWN" ,
		"MOUSE_LBUTTONUP"	,
		"MOUSE_RBUTTONDOWN"	,
		"MOUSE_RBUTTONUP"	,
		"MOUSE_MBUTTONDOWN"	,
		"MOUSE_MBUTTONUP"	,
		"WINDOW_SELECT"		,
		"WINDOW_DESELECT"	,
		"WINDOW_MOVE"		,
		"WINDOW_RESIZE"		,
		"WINDOW_CLOSE"		,
		"KEY_DOWN"			,
		"KEY_UP"			};

	RECT stButtonArea;
	QWORD qwFindWindowID;
	EVENT stSendEvent;
	int i;


	// ========== judge Graphic Mode ==========
	if(kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI Mode\n");

		return;
	}


	// ========== create Window ==========
	
	kGetCursorPosition(&iMouseX, &iMouseY);

	
	// set Window size, Window title
	iWindowWidth = 500;
	iWindowHeight = 200;


	// call kCreateWindow(), Window location based on Mouse Cursor 
	// 		give each Window unique title
	kSPrintf(vcTempBuffer, "Hello World Window %d", s_iWindowCount++);

	qwWindowID = kCreateWindow(iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2, 
							   iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT, vcTempBuffer);

	if(qwWindowID == WINDOW_INVALIDID)
	{
		return;
	}


	// ========== draw the part of printing Window Event (Window Manager -> Window) ==========
	
	// Event Information position to print
	iY = WINDOW_TITLEBAR_HEIGHT + 10;


	// draw line of Event Information area, Window ID
	kDrawRect(qwWindowID, 10, iY + 8, iWindowWidth - 10, iY + 70, RGB(0, 0, 0), FALSE);

	kSPrintf(vcTempBuffer, "GUI Event Information [Window ID: 0x%Q]", qwWindowID);
	kDrawText(qwWindowID, 20, iY, RGB(0, 0, 0), RGB(255, 255, 255), vcTempBuffer, kStrLen(vcTempBuffer));

	
	// ========== draw Event Send Button ==========
	
	// set Button Area
	kSetRectangleData(10, iY + 80, iWindowWidth - 10, iWindowHeight - 10, &stButtonArea);

	// draw Button
	kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "User Message Send Button (Up)", RGB(0, 0, 0));
	
	// show Window
	kShowWindow(qwWindowID, TRUE);


	// ========== Event Handling loop ==========
	
	while(1)
	{
		// get received Event in Event Queue
		if(kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			kSleep(0);

			continue;
		}


		// erase previous Window Event Information by filling area with Background color
		kDrawRect(qwWindowID, 11, iY + 20, iWindowWidth - 11, iY + 69, WINDOW_COLOR_BACKGROUND, TRUE);


		// handle received Event by Event Type
		switch(stReceivedEvent.qwType)
		{
			// handle Mouse Event
			case EVENT_MOUSE_MOVE:
			case EVENT_MOUSE_LBUTTONDOWN:
			case EVENT_MOUSE_LBUTTONUP:
			case EVENT_MOUSE_RBUTTONDOWN:
			case EVENT_MOUSE_RBUTTONUP:
			case EVENT_MOUSE_MBUTTONDOWN:
			case EVENT_MOUSE_MBUTTONUP:

				// insert Mouse Event handling code here
				pstMouseEvent = &(stReceivedEvent.stMouseEvent);

				
				// print Mouse Event Type
				kSPrintf(vcTempBuffer, "Mouse Event : %s", vpcEventString[stReceivedEvent.qwType]);
				kDrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

				
				// print Mouse Data
				kSPrintf(vcTempBuffer, "Data: X = %d, Y = %d, Button = %X",
						 pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY, pstMouseEvent->bButtonStatus);
				kDrawText(qwWindowID, 20, iY + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));


				// --------------------------------------------------
				// if Mouse Up or Down Event, redraw Button
				// --------------------------------------------------

				if(stReceivedEvent.qwType == EVENT_MOUSE_LBUTTONDOWN)
				{
					// is Button Up or Down Event on the Button?
					if(kIsInRectangle(&stButtonArea, pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY) == TRUE)
					{
						// set as "down" by fill Button color with bright green
						kDrawButton(qwWindowID, &stButtonArea, RGB(79, 204, 11), 
									"User Message Send Button (Down)", RGB(255, 255, 255));
						
						kUpdateScreenByID(qwWindowID);


						// ******************************************
						// send User Event to other Window
						// ******************************************
						
						// send Event to all created Window
						stSendEvent.qwType = EVENT_USER_TESTMESSAGE;
						stSendEvent.vqwData[0] = qwWindowID;
						stSendEvent.vqwData[1] = 0x1234;
						stSendEvent.vqwData[2] = 0x5678;


						// send Event by looping for count of created Window
						for(i=0; i<s_iWindowCount; i++)
						{
							// search Window by Window Title
							kSPrintf(vcTempBuffer, "Hello World Window %d", i);
							qwFindWindowID = kFindWindowByTitle(vcTempBuffer);


							if((qwFindWindowID != WINDOW_INVALIDID) && (qwFindWindowID != qwWindowID))
							{
								// send Event to other Window
								kSendEventToWindow(qwFindWindowID, &stSendEvent);
							}
						}

					}	// if LButton Down on the Button

				}	// if LButtonDown on Window
				
				else if(stReceivedEvent.qwType == EVENT_MOUSE_LBUTTONUP)
				{
					// set as "up" by fill Button color with white
					kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "User Message Send Button (Up)",
								RGB(0, 0, 0));
				}

				break;


			// handle Key Event
			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:

				// insert Key Event handling code here
				pstKeyEvent = &(stReceivedEvent.stKeyEvent);


				// print Key Event Type
				kSPrintf(vcTempBuffer, "Key Event: %s", vpcEventString[stReceivedEvent.qwType]);
				kDrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));


				// print Key Data
				kSPrintf(vcTempBuffer, "Data: Key = %c, Flag = %X", pstKeyEvent->bASCIICode, pstKeyEvent->bFlags);
				kDrawText(qwWindowID, 20, iY + 40, RGB(0, 0 ,0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

				break;

			// handle Window Event
			case EVENT_WINDOW_SELECT:
			case EVENT_WINDOW_DESELECT:
			case EVENT_WINDOW_MOVE:
			case EVENT_WINDOW_RESIZE:
			case EVENT_WINDOW_CLOSE:

				// insert Window Event handling code here
				pstWindowEvent = &(stReceivedEvent.stWindowEvent);

				
				// print Window Event Type
				kSPrintf(vcTempBuffer, "Window Event : %s", vpcEventString[stReceivedEvent.qwType]);
				kDrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

				
				// print Window Data
				kSPrintf(vcTempBuffer, "Data: X1 = %d, Y1 = %d, X2 = %d, Y2 = %d",
						 pstWindowEvent->stArea.iX1, pstWindowEvent->stArea.iY1, 
						 pstWindowEvent->stArea.iX2, pstWindowEvent->stArea.iY2);
				kDrawText(qwWindowID, 20, iY + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));


				// ******************************************
				// if CLOSE Event, delete Window, end Task
				// ******************************************
				
				if(stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
				{
					kDeleteWindow(qwWindowID);

					return;
				}

				break;


			default:
				
				// insert unknown Event handling code here
				
				// print unknown Event
				kSPrintf(vcTempBuffer, "Unknown Event: 0x%X", stReceivedEvent.qwType);
				kDrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

				
				// print Data
				kSPrintf(vcTempBuffer, "Data 0 = 0x%Q, Data 1 = 0x%Q, Data 2 = 0x%Q",
						 stReceivedEvent.vqwData[0], stReceivedEvent.vqwData[1], stReceivedEvent.vqwData[2]);
				kDrawText(qwWindowID, 20, iY + 40, RGB(0, 0 ,0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));


				break;
		}

		// update Window to screen
		kShowWindow(qwWindowID, TRUE);
	
	}	// infinite loop end
}



// ---------------------------------------
// System Monitor Task
// ---------------------------------------

void kSystemMonitorTask(void)
{
	QWORD qwWindowID;
	int i;
	int iWindowWidth;
	int iProcessorCount;
	DWORD vdwLastCPULoad[MAXPROCESSORCOUNT];
	int viLastTaskCount[MAXPROCESSORCOUNT];
	QWORD qwLastTickCount;
	EVENT stReceivedEvent;
	WINDOWEVENT* pstWindowEvent;
	BOOL bChanged;
	RECT stScreenArea;
	QWORD qwLastDynamicMemoryUsedSize;
	QWORD qwDynamicMemoryUsedSize;
	QWORD qwTemp;
	INTERRUPTMANAGER* pstInterruptManager;

	
	if(kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI mode\n");
		return;
	}


	// -------------------------------------------
	// create Window
	// -------------------------------------------
	
	// get Screen Area size
	kGetScreenArea(&stScreenArea);


	// calculate Window width by Processor count
	iProcessorCount = kGetProcessorCount();
	iWindowWidth = iProcessorCount * (SYSTEMMONITOR_PROCESSOR_WIDTH + SYSTEMMONITOR_PROCESSOR_MARGIN) + 
				   SYSTEMMONITOR_PROCESSOR_MARGIN;

	
	// create Window at the center of screen
	qwWindowID = kCreateWindow( (stScreenArea.iX2 - iWindowWidth) / 2, (stScreenArea.iY2 - SYSTEMMONITOR_WINDOW_HEIGHT) / 2, 
								iWindowWidth, SYSTEMMONITOR_WINDOW_HEIGHT, WINDOW_FLAGS_DEFAULT & -WINDOW_FLAGS_SHOW,
								"System Monitor" );

	if(qwWindowID == WINDOW_INVALIDID)
	{
		return;
	}


	// draw ------Processor Information---------------
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + 15, iWindowWidth - 5, WINDOW_TITLEBAR_HEIGHT + 15, RGB(0, 0, 0));
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + 16, iWindowWidth - 5, WINDOW_TITLEBAR_HEIGHT + 16, RGB(0, 0, 0));
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + 17, iWindowWidth - 5, WINDOW_TITLEBAR_HEIGHT + 17, RGB(0, 0, 0));
	
	kDrawText(qwWindowID, 9, WINDOW_TITLEBAR_HEIGHT +  8, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "Processor Information", 21);


	// draw ------Memory Inforamtion------------------
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 50, iWindowWidth - 5, 
			  WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 50, RGB(0, 0, 0));
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 51, iWindowWidth - 5, 
			  WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 51, RGB(0, 0, 0));
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 52, iWindowWidth - 5, 
			  WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 52, RGB(0, 0, 0));

	kDrawText(qwWindowID, 9, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 43, RGB(0, 0, 0), 
			  WINDOW_COLOR_BACKGROUND, "Memory Information", 18);


	// show Window
	kShowWindow(qwWindowID, TRUE);


	// monitor, print System Information while loop
	qwLastTickCount = 0;


	// init Last CPU Load, Last Task Count, Memory Usage
	kMemSet(vdwLastCPULoad, 0, sizeof(vdwLastCPULoad));
	kMemSet(viLastTaskCount, 0, sizeof(viLastTaskCount));
	qwLastDynamicMemoryUsedSize = 0;


	// ---------------------------------------------------
	// Event Handling Loop
	// ---------------------------------------------------
	
	while(1)
	{
		// get Event from Event Queue
		if(kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == TRUE)
		{
			switch(stReceivedEvent.qwType)
			{
				case EVENT_WINDOW_CLOSE:
					
					// delete Window
					kDeleteWindow(qwWindowID);
					
					return;

					break;

				default:

					break;
			}
		}


		// check System status every 0.5 sec
		if( (kGetTickCount() - qwLastTickCount) < 500 )
		{
			kSleep(1);

			continue;
		}


		// update Last Tick Count
		qwLastTickCount = kGetTickCount();


		// --------------------------------------
		// print Processor Info
		// --------------------------------------

		pstInterruptManager = kGetInterruptManager();


		iProcessorCount = 0;

		for(i = 0; i<MAXPROCESSORCOUNT; i++)		// FIXME : check all processor
		{
			if(pstInterruptManager->vvqwCoreInterruptCount[i][0] == 0)
			{
				continue;
			}
			
			bChanged = FALSE;

			
			// check Processor Load
			if(vdwLastCPULoad[i] != kGetProcessorLoad(i))
			{
				vdwLastCPULoad[i] = kGetProcessorLoad(i);

				bChanged = TRUE;
			}
			
			// check Task Count
			else if(viLastTaskCount[i] != kGetTaskCount(i))
			{
				viLastTaskCount[i] = kGetTaskCount(i);

				bChanged = TRUE;
			}


			// if status changed, update changes to screen
			if(bChanged == TRUE)
			{
				// draw current Processor Load
				kDrawProcessorInformation(qwWindowID, 
						iProcessorCount * SYSTEMMONITOR_PROCESSOR_WIDTH + (iProcessorCount + 1) * SYSTEMMONITOR_PROCESSOR_MARGIN,
						WINDOW_TITLEBAR_HEIGHT + 28, i);
			}

			iProcessorCount++;
		}	// print Processor Info loop end

			
		// --------------------------------------
		// print Memory Info
		// --------------------------------------

		// get Dynamic Memory Info
		kGetDynamicMemoryInformation(&qwTemp, &qwTemp, &qwTemp, &qwDynamicMemoryUsedSize);


		// if current DynamicMemoryUsedSize not match with previous one, update info to screen
		if(qwDynamicMemoryUsedSize != qwLastDynamicMemoryUsedSize)
		{
			qwLastDynamicMemoryUsedSize = qwDynamicMemoryUsedSize;


			// draw Memory Information
			kDrawMemoryInformation(qwWindowID, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 60, iWindowWidth);
		}

	}	// while(1)
}


static void kDrawProcessorInformation(QWORD qwWindowID, int iX, int iY, BYTE bAPICID)
{
	char vcBuffer[100];
	RECT stArea;
	QWORD qwProcessorLoad;
	QWORD iUsageBarHeight;
	int iMiddleX;


	// print Processor ID
	kSPrintf(vcBuffer, "Processor ID: %d", bAPICID);
	kDrawText(qwWindowID, iX + 10, iY, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));


	// print Processor Task Count
	kSPrintf(vcBuffer, "Task Count: %d", kGetTaskCount(bAPICID));
	kDrawText(qwWindowID, iX + 10, iY + 18, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));


	// ---------------------------------------------------
	// draw Processor Load Bar
	// ---------------------------------------------------
	
	qwProcessorLoad = kGetProcessorLoad(bAPICID);
	
	if(qwProcessorLoad > 100)
	{
		qwProcessorLoad = 100;
	}


	// draw Processor Load Bar Area
	kDrawRect(qwWindowID, iX, iY + 36, iX + SYSTEMMONITOR_PROCESSOR_WIDTH, iY + SYSTEMMONITOR_PROCESSOR_HEIGHT,
			  RGB(0, 0, 0), FALSE);


	// calculate Prcessor Load Bar Height (Bar Height * Processor Load / 100)
	iUsageBarHeight = (SYSTEMMONITOR_PROCESSOR_HEIGHT - 40) * qwProcessorLoad / 100;


	// draw inside of Processor Load Area
	// 		1. draw filled Bar, 2 pixel margin within Processor Load Area Line
	kDrawRect(qwWindowID, iX + 2, iY + (SYSTEMMONITOR_PROCESSOR_HEIGHT - iUsageBarHeight) - 2, 
			  iX + SYSTEMMONITOR_PROCESSOR_WIDTH - 2, iY + SYSTEMMONITOR_PROCESSOR_HEIGHT - 2,
			  SYSTEMMONITOR_BAR_COLOR, TRUE);

	//		2. draw empty Bar, 2 pixel margin within Processor Load Area Line
	kDrawRect(qwWindowID, iX + 2, iY + 38, iX + SYSTEMMONITOR_PROCESSOR_WIDTH - 2, 
			  iY + (SYSTEMMONITOR_PROCESSOR_HEIGHT - iUsageBarHeight) - 1, WINDOW_COLOR_BACKGROUND, TRUE);


	// draw Processor Load Text at the center of Bar
	kSPrintf(vcBuffer, "Usage: %d%%", qwProcessorLoad);
	
	iMiddleX = (SYSTEMMONITOR_PROCESSOR_WIDTH - (kStrLen(vcBuffer) * FONT_ENGLISHWIDTH)) / 2;
	kDrawText(qwWindowID, iX + iMiddleX, iY + 80, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));


	// update Processor Load Area
	kSetRectangleData(iX, iY, iX + SYSTEMMONITOR_PROCESSOR_WIDTH, iY + SYSTEMMONITOR_PROCESSOR_HEIGHT, &stArea);
	
	kUpdateScreenByWindowArea(qwWindowID, &stArea);
}


static void kDrawMemoryInformation(QWORD qwWindowID, int iY, int iWindowWidth)
{
	char vcBuffer[100];
	QWORD qwTotalRAMKbyteSize;
	QWORD qwDynamicMemoryStartAddress;
	QWORD qwDynamicMemoryUsedSize;
	QWORD qwUsedPercent;
	QWORD qwTemp;
	int iUsageBarWidth;
	RECT stArea;
	int iMiddleX;


	// get Total RAM Size (Kilobyte)
	qwTotalRAMKbyteSize = kGetTotalRAMSize() * 1024;


	// draw Memory Information Text
	kSPrintf(vcBuffer, "Total Size: %d KB		", qwTotalRAMKbyteSize);
	kDrawText(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 10, iY + 3, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, 
			  vcBuffer, kStrLen(vcBuffer));


	// get Dynamic Memory Info
	kGetDynamicMemoryInformation(&qwDynamicMemoryStartAddress, &qwTemp, &qwTemp, &qwDynamicMemoryUsedSize);

	kSPrintf(vcBuffer, "Used Size: %d KB		", (qwDynamicMemoryUsedSize + qwDynamicMemoryStartAddress) / 1024);
	kDrawText(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 10, iY + 21, RGB(0, 0, 0),
			  WINDOW_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));


	// --------------------------------
	// draw Memory Usage Bar
	// --------------------------------
	
	// draw Memory Usage Area Line
	kDrawRect(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN, iY + 40, iWindowWidth - SYSTEMMONITOR_PROCESSOR_MARGIN,
			  iY + SYSTEMMONITOR_MEMORY_HEIGHT - 32, RGB(0, 0, 0), FALSE);


	// calculate Memory Use Percentage
	qwUsedPercent = (qwDynamicMemoryStartAddress + qwDynamicMemoryUsedSize) * 100 / 1024 / qwTotalRAMKbyteSize;

	if(qwUsedPercent > 100)
	{
		qwUsedPercent = 100;
	}


	// Memory Usage Bar Length (Bar Length * Memory Usage / 100)
	iUsageBarWidth = (iWindowWidth - 2 * SYSTEMMONITOR_PROCESSOR_MARGIN) * qwUsedPercent / 100;


	// draw Bars
	// 		1. draw filled Bar, 2 pixel margin within Memory Usage Area Line
	kDrawRect(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 2, iY + 42, SYSTEMMONITOR_PROCESSOR_MARGIN + 2 + iUsageBarWidth,
			  iY + SYSTEMMONITOR_MEMORY_HEIGHT - 34, SYSTEMMONITOR_BAR_COLOR, TRUE);
	
	// 		2. draw empty Bar, 2 pixel margin within Memory Usage Area Line
	kDrawRect(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 2 + iUsageBarWidth, iY + 42,
			  iWindowWidth - SYSTEMMONITOR_PROCESSOR_MARGIN - 2, iY + SYSTEMMONITOR_MEMORY_HEIGHT - 34,
			  WINDOW_COLOR_BACKGROUND, TRUE);


	// draw Usage Text at the center of Bar
	kSPrintf(vcBuffer, "Usage: %d%%", qwUsedPercent);
	iMiddleX = (iWindowWidth - (kStrLen(vcBuffer) * FONT_ENGLISHWIDTH)) / 2;
	kDrawText(qwWindowID, iMiddleX, iY + 45, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));


	// update Memory Information Area
	kSetRectangleData(0, iY, iWindowWidth, iY + SYSTEMMONITOR_MEMORY_HEIGHT, &stArea);

	kUpdateScreenByWindowArea(qwWindowID, &stArea);
}



// ---------------------------------------
// GUI Console Shell Task
// ---------------------------------------

// previous screen Buffer area
static CHARACTER gs_vstPreviousScreenBuffer[CONSOLE_WIDTH * CONSOLE_HEIGHT];


// GUI Console Shell Task
void kGUIConsoleShellTask(void)
{
	static QWORD s_qwWindowID = WINDOW_INVALIDID;
	int iWindowWidth, iWindowHeight;
	EVENT stReceivedEvent;
	KEYEVENT* pstKeyEvent;
	RECT stScreenArea;
	KEYDATA stKeyData;
	TCB* pstConsoleShellTask;
	QWORD qwConsoleShellTaskID;

	
	// ------------------------------------------------
	// judge Graphic Mode
	// ------------------------------------------------
	
	if(kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI Mode\n");
		return;
	}

	
	// if GUI Console Shell Task running already, move Window to top
	if(s_qwWindowID != WINDOW_INVALIDID)
	{
		kMoveWindowToTop(s_qwWindowID);

		return;
	}

	
	// ------------------------------------------------
	// Create Window
	// ------------------------------------------------
	
	// get size of screen Area
	kGetScreenArea(&stScreenArea);


	// set Window size
	iWindowWidth = CONSOLE_WIDTH * FONT_ENGLISHWIDTH + 4;
	iWindowHeight = CONSOLE_HEIGHT * FONT_ENGLISHHEIGHT + WINDOW_TITLEBAR_HEIGHT + 2;

	
	// create Window at the center of screen
	s_qwWindowID = kCreateWindow((stScreenArea.iX2 - iWindowWidth) / 2, (stScreenArea.iY2 - iWindowHeight) / 2,
								 iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT, "Console Shell for GUI");

	if(s_qwWindowID == WINDOW_INVALIDID)
	{
		return;
	}


	// create Console Shell Task (handling Shell command)
	kSetConsoleShellExitFlag(FALSE);
	pstConsoleShellTask = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kStartConsoleShell, TASK_LOADBALANCINGID);

	if(pstConsoleShellTask == NULL)
	{
		// if failed to create Console Shell Task, delete Window
		kDeleteWindow(s_qwWindowID);

		return;
	}

	
	// save Console Shell Task ID
	qwConsoleShellTaskID = pstConsoleShellTask->stLink.qwID;


	// init Previous screen buffer
	kMemSet(gs_vstPreviousScreenBuffer, 0xFF, sizeof(gs_vstPreviousScreenBuffer));


	// ------------------------------------------------
	// GUI Task Event handling loop
	// ------------------------------------------------
	
	while(1)
	{
		// update changes to Window
		kProcessConsoleBuffer(s_qwWindowID);

		
		// receive Event from Event Queue
		if(kReceiveEventFromWindowQueue(s_qwWindowID, &stReceivedEvent) == FALSE)
		{
			kSleep(0);

			continue;
		}


		// handle Event by event type
		switch(stReceivedEvent.qwType)
		{
			// handle Key Event
			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:

				pstKeyEvent = &(stReceivedEvent.stKeyEvent);

				stKeyData.bASCIICode = pstKeyEvent->bASCIICode;
				stKeyData.bFlags 	 = pstKeyEvent->bFlags;
				stKeyData.bScanCode  = pstKeyEvent->bScanCode;


				// insert Key Data to GUI Key Queue as Shell Task input
				kPutKeyToGUIKeyQueue(&stKeyData);

				break;

			// handle Window Event
			case EVENT_WINDOW_CLOSE:

				// set Exit Flag
				kSetConsoleShellExitFlag(TRUE);

				while(kIsTaskExist(qwConsoleShellTaskID) == TRUE)
				{
					kSleep(1);
				}


				// delete Window, init Window ID
				kDeleteWindow(s_qwWindowID);
				s_qwWindowID = WINDOW_INVALIDID;
				
				return;

				break;

			default:

				break;
		}

	}	// Event handling infinite loop end
}


// update changes to GUI Console Shell Window
static void kProcessConsoleBuffer(QWORD qwWindowID)
{
	int i;
	int j;
	CONSOLEMANAGER* pstManager;
	CHARACTER* pstScreenBuffer;
	CHARACTER* pstPreviousScreenBuffer;
	RECT stLineArea;
	BOOL bChanged;
	static QWORD s_qwLastTickCount = 0;
	BOOL bFullRedraw;
	

	// get Console Manager to get Screen Buffer Address
	// 		also get Previous Screen Buffer
	pstManager = kGetConsoleManager();
	pstScreenBuffer = pstManager->pstScreenBuffer;
	pstPreviousScreenBuffer = gs_vstPreviousScreenBuffer;


	// redraw screen every 5 second
	if(kGetTickCount() - s_qwLastTickCount > 5000)
	{
		s_qwLastTickCount = kGetTickCount();
		bFullRedraw = TRUE;
	}
	else
	{
		bFullRedraw = FALSE;
	}


	for(j=0; j<CONSOLE_HEIGHT; j++)
	{
		bChanged = FALSE;

		
		// is there changes in current line?
		for(i=0; i<CONSOLE_WIDTH; i++)
		{
			// if the comparison is incorrect or fully redraw needed,
			// 		update changes to Previous Screen Buffer and set bChanged flag
			if( (pstScreenBuffer->bCharactor != pstPreviousScreenBuffer->bCharactor) || (bFullRedraw == TRUE) )
			{
				// print a text
				kDrawText(qwWindowID, i * FONT_ENGLISHWIDTH + 2, j * FONT_ENGLISHHEIGHT + WINDOW_TITLEBAR_HEIGHT,
						  RGB(0, 255, 0), RGB(0, 0, 0), &(pstScreenBuffer->bCharactor), 1);


				// move valut to Previous Screen Buffer, set bChanged flag
				kMemCpy(pstPreviousScreenBuffer, pstScreenBuffer, sizeof(CHARACTER));
				bChanged = TRUE;
			}

			pstScreenBuffer++;
			pstPreviousScreenBuffer++;
		}


		// if changes detected in current line, redraw current line only
		if(bChanged == TRUE)
		{
			// calculate current line area
			kSetRectangleData(2, j * FONT_ENGLISHHEIGHT + WINDOW_TITLEBAR_HEIGHT, 5 + FONT_ENGLISHWIDTH * CONSOLE_WIDTH,
							  (j + 1) * FONT_ENGLISHHEIGHT + WINDOW_TITLEBAR_HEIGHT - 1, &stLineArea);


			kUpdateScreenByWindowArea(qwWindowID, &stLineArea);
		}
	}
}



// ---------------------------------------
// Image Viewer
// ---------------------------------------

void kImageViewerTask(void)
{
	QWORD qwWindowID;
	int iMouseX, iMouseY;
	int iWindowWidth, iWindowHeight;
	int iEditBoxWidth;
	RECT stEditBoxArea;
	RECT stButtonArea;
	RECT stScreenArea;
	EVENT stReceivedEvent;
	EVENT stSendEvent;
	char vcFileName[FILESYSTEM_MAXFILENAMELENGTH + 1];
	int iFileNameLength;
	MOUSEEVENT* pstMouseEvent;
	KEYEVENT* pstKeyEvent;
	POINT stScreenXY;
	POINT stClientXY;


	// ---------- judge Graphic Mode ----------
	if(kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI Mode\n");
		return;
	}


	// ---------- create Window ----------
	
	// get Screen Area
	kGetScreenArea(&stScreenArea);


	// set Window size
	iWindowWidth = FONT_ENGLISHWIDTH * FILESYSTEM_MAXFILENAMELENGTH + 165;
	iWindowHeight = 35 + WINDOW_TITLEBAR_HEIGHT + 5;


	// create Window at the center of Screen
	qwWindowID = kCreateWindow((stScreenArea.iX2 - iWindowWidth) / 2, (stScreenArea.iY2 - iWindowHeight) / 2, 
							   iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT & -WINDOW_FLAGS_SHOW, "Image Viewer");

	if(qwWindowID == WINDOW_INVALIDID)
	{
		return;
	}


	// draw FILE NAME Text, Edit Box
	kDrawText(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + 6, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "FILE NAME", 9);

	iEditBoxWidth = FONT_ENGLISHWIDTH * FILESYSTEM_MAXFILENAMELENGTH + 4;
	kSetRectangleData(85, WINDOW_TITLEBAR_HEIGHT + 5, 85 + iEditBoxWidth, WINDOW_TITLEBAR_HEIGHT + 25, &stEditBoxArea);
	kDrawRect(qwWindowID, stEditBoxArea.iX1, stEditBoxArea.iY1, stEditBoxArea.iX2, stEditBoxArea.iY2, RGB(0, 0, 0), FALSE);


	// clear File Name Buffer, draw empty File Name to Edit Box
	iFileNameLength = 0;
	kMemSet(vcFileName, 0, sizeof(vcFileName));
	kDrawFileName(qwWindowID, &stEditBoxArea, vcFileName, iFileNameLength);


	// set and draw Button Area
	kSetRectangleData(stEditBoxArea.iX2 + 10, stEditBoxArea.iY1, stEditBoxArea.iX2 + 70, stEditBoxArea.iY2, &stButtonArea);
	kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "Show", RGB(0, 0, 0));


	// show Window
	kShowWindow(qwWindowID, TRUE);


	// ---------- Event Handling Loop ----------
	
	while(1)
	{
		// receive Event from Event Queue
		if(kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			kSleep(0);

			continue;
		}


		// handle Event by Event Type
		switch(stReceivedEvent.qwType)
		{
			// handle Mouse Event
			case EVENT_MOUSE_LBUTTONDOWN:
				
				pstMouseEvent = &(stReceivedEvent.stMouseEvent);


				// if LBUTTONDOWN on Button, show Image by using Image File Name
				if(kIsInRectangle(&stButtonArea, pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY) == TRUE)
				{
					// draw Button as pushed
					kDrawButton(qwWindowID, &stButtonArea, RGB(79, 204, 11), "Show", RGB(255, 255, 255));
					kUpdateScreenByWindowArea(qwWindowID, &(stButtonArea));

					
					// create Image Viewer Window, handling Event
					if(kCreateImageViewerWindowAndExecute(qwWindowID, vcFileName) == FALSE)
					{
						// if failed to create Window, wait 200ms to get effect Button pushed then not pushed
						kSleep(200);
					}


					// draw Button as not pushed
					kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "Show", RGB(0, 0, 0));
					kUpdateScreenByWindowArea(qwWindowID, &(stButtonArea));
				}

				break;

			// handle Key Event
			case EVENT_KEY_DOWN:

				pstKeyEvent = &(stReceivedEvent.stKeyEvent);


				// BackSpace Key deletes a character
				if( (pstKeyEvent->bASCIICode == KEY_BACKSPACE) && (iFileNameLength > 0) )
				{
					// remove the last character in buffer
					vcFileName[iFileNameLength] = '\0';
					iFileNameLength--;


					// draw File Name in Edit Box
					kDrawFileName(qwWindowID, &stEditBoxArea, vcFileName, iFileNameLength);
				}

				// Enter Key makes Button down
				else if( (pstKeyEvent->bASCIICode == KEY_ENTER) && (iFileNameLength > 0) )
				{
					// make Mouse Event and send
					stClientXY.iX = stButtonArea.iX1 + 1;
					stClientXY.iY = stButtonArea.iY1 + 1;
					kConvertPointClientToScreen(qwWindowID, &stClientXY, &stScreenXY);

					kSetMouseEvent(qwWindowID, EVENT_MOUSE_LBUTTONDOWN, stScreenXY.iX + 1, stScreenXY.iY + 1, 0, &stSendEvent);
					kSendEventToWindow(qwWindowID, &stSendEvent);
				}
				
				// ESC Key makes Window close
				else if( (pstKeyEvent->bASCIICode == KEY_ESC) && (iFileNameLength > 0) )
				{
					// make Window close Event and send
					kSetWindowEvent(qwWindowID, EVENT_WINDOW_CLOSE, &stSendEvent);
					kSendEventToWindow(qwWindowID, &stSendEvent);
				}
				
				// other Key into the Buffer (if Buffer has space)
				else if( (pstKeyEvent->bASCIICode <= 128) && 
						 (pstKeyEvent->bASCIICode != KEY_BACKSPACE) &&
						 (iFileNameLength < FILESYSTEM_MAXFILENAMELENGTH) )
				{
					// insert Key ASCII Code at iFileNameLength index of array
					vcFileName[iFileNameLength] = pstKeyEvent->bASCIICode;
					iFileNameLength++;


					// draw input Key to Edit Box
					kDrawFileName(qwWindowID, &stEditBoxArea, vcFileName, iFileNameLength);
				}

				break;

			// handle Window Event
			case EVENT_WINDOW_CLOSE:

				if(stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
				{
					kDeleteWindow(qwWindowID);

					return;
				}

				break;

			default:

				break;
		}
	}
}


static void kDrawFileName(QWORD qwWindowID, RECT* pstArea, char* pcFileName, int iNameLength)
{
	// erase previous contents
	kDrawRect(qwWindowID, pstArea->iX1 + 1, pstArea->iY1 + 1, pstArea->iX2 - 1, pstArea->iY2 - 1, WINDOW_COLOR_BACKGROUND, TRUE);


	// draw File Name
	kDrawText(qwWindowID, pstArea->iX1 + 2, pstArea->iY1 + 2, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, pcFileName, iNameLength);


	// draw Cursor
	if(iNameLength < FILESYSTEM_MAXFILENAMELENGTH)
	{
		kDrawText(qwWindowID, pstArea->iX1 + 2 + FONT_ENGLISHWIDTH * iNameLength, pstArea->iY1 + 2,
				  RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "_", 1);
	}


	// update(redraw) Edit Box area
	kUpdateScreenByWindowArea(qwWindowID, pstArea);
}


// read JPEG File and draw to Image Viewer Window, handle Event
static BOOL kCreateImageViewerWindowAndExecute(QWORD qwMainWindowID, const char* pcFileName)
{
	DIR* pstDirectory;
	struct dirent* pstEntry;
	DWORD dwFileSize;
	RECT stScreenArea;
	QWORD qwWindowID;
	WINDOW* pstWindow;
	BYTE* pbFileBuffer;
	COLOR* pstOutputBuffer;
	int iWindowWidth;
	FILE* fp;
	JPEG* pstJpeg;
	EVENT stReceivedEvent;
	KEYEVENT* pstKeyEvent;
	BOOL bExit;
	

	// initialize
	fp = NULL;
	pbFileBuffer = NULL;
	pstOutputBuffer = NULL;
	qwWindowID = WINDOW_INVALIDID;


	// ------------------------------------------------------
	// Open Root Directory to search a File
	// ------------------------------------------------------
	
	pstDirectory = opendir("/");
	dwFileSize = 0;

	
	// search target File in Directory
	while(1)
	{
		// read one Entry
		pstEntry = readdir(pstDirectory);

		if(pstEntry == NULL)		// file not found
		{
			break;
		}


		// check both file name and file name length
		if( (kStrLen(pstEntry->d_name) == kStrLen(pcFileName)) && 
			(kMemCmp(pstEntry->d_name, pcFileName, kStrLen(pcFileName)) == 0) )
		{
			dwFileSize = pstEntry->dwFileSize;

			break;
		}
	}


	// return Directory Handle (HANDLE MUST BE FREED)
	closedir(pstDirectory);


	// ------------------------------------------------------
	// read and decode found File
	// ------------------------------------------------------
	
	// read File
	fp = fopen(pcFileName, "rb");

	if(fp == NULL)
	{
		kPrintf("[ImageViewer] %s file open fail\n", pcFileName);

		return FALSE;
	}


	// allocate memory
	pbFileBuffer = (BYTE*)kAllocateMemory(dwFileSize);
	pstJpeg = (JPEG*)kAllocateMemory(sizeof(JPEG));

	if( (pbFileBuffer == NULL) || (pstJpeg == NULL) )
	{
		kPrintf("[ImageViewer] Buffer allocation fail\n");

		kFreeMemory(pbFileBuffer);
		kFreeMemory(pstJpeg);

		fclose(fp);

		return FALSE;
	}


	// is the File JPEG format?
	if( (fread(pbFileBuffer, 1, dwFileSize, fp) != dwFileSize) || (kJPEGInit(pstJpeg, pbFileBuffer, dwFileSize) == FALSE) )
	{
		kPrintf("[ImageViewer] Read fail or file is not JPEG format\n");

		kFreeMemory(pbFileBuffer);
		kFreeMemory(pstJpeg);

		fclose(fp);

		return FALSE;
	}


	// a Buffer for decoded result
	pstOutputBuffer = kAllocateMemory(pstJpeg->width * pstJpeg->height * sizeof(COLOR));


	// if decoding finished successfully, create Window
	if( (pstOutputBuffer != NULL) && (kJPEGDecode(pstJpeg, pstOutputBuffer) == TRUE) )
	{
		kGetScreenArea(&stScreenArea);

		
		// create Window, considering Image size, Title bar size
		qwWindowID = kCreateWindow( (stScreenArea.iX2 - pstJpeg->width) / 2, (stScreenArea.iY2 - pstJpeg->height) / 2,
									pstJpeg->width, pstJpeg->height + WINDOW_TITLEBAR_HEIGHT,
									WINDOW_FLAGS_DEFAULT & -WINDOW_FLAGS_SHOW | WINDOW_FLAGS_RESIZABLE, pcFileName );
	}

	
	// exit if failed to create Window or decode
	if( (qwWindowID == WINDOW_INVALIDID) || (pstOutputBuffer == NULL) )
	{
		kPrintf("[ImageViewer] Window create fail or output buffer allocation fail\n");

		kFreeMemory(pbFileBuffer);
		kFreeMemory(pstJpeg);
		kFreeMemory(pstOutputBuffer);

		kDeleteWindow(qwWindowID);


		return FALSE;
	}


	//////////////////////////////////////////////////////////////////////////////
	// copy decoded image to Image Viewer Window Buffer, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	//////////////////////////////////////////////////////////////////////////////

	if(pstWindow != NULL)
	{
		iWindowWidth = kGetRectangleWidth(&(pstWindow->stArea));

		kMemCpy(pstWindow->pstWindowBuffer + (WINDOW_TITLEBAR_HEIGHT * iWindowWidth), pstOutputBuffer,
				pstJpeg->width * pstJpeg->height * sizeof(COLOR));


		////////////////////////////////
		// CRITICAL SECTION END
		kUnlock(&(pstWindow->stLock));
		////////////////////////////////
		
	}


	// free File Buffer, update Screen
	kFreeMemory(pbFileBuffer);

	kShowWindow(qwWindowID, TRUE);



	// ----------------------------------------------------
	// simple Event handling loop
	// ----------------------------------------------------
	
	// hide File Name input Window
	kShowWindow(qwMainWindowID, FALSE);


	bExit = FALSE;
	
	while(bExit == FALSE)
	{
		if(kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			kSleep(0);

			continue;
		}


		// handle Event by Event Type
		switch(stReceivedEvent.qwType)
		{
			// handle Key Event
			case EVENT_KEY_DOWN:

				pstKeyEvent = &(stReceivedEvent.stKeyEvent);

				// ESC Key deletes Image Viewer Window, show File Name input Window
				if(pstKeyEvent->bASCIICode == KEY_ESC)
				{
					kDeleteWindow(qwWindowID);

					kShowWindow(qwMainWindowID, TRUE);


					return TRUE;
				}

				break;


			// handle Window Event
			
			// Window Resizing
			case EVENT_WINDOW_RESIZE:

				// send decoded image to resized Window
				kBitBlt(qwWindowID, 0, WINDOW_TITLEBAR_HEIGHT, pstOutputBuffer, pstJpeg->width, pstJpeg->height);

				// call kShowWindow() to update
				kShowWindow(qwWindowID, TRUE);

				break;

			// Window Close
			case EVENT_WINDOW_CLOSE:
				
				if(stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
				{
					kDeleteWindow(qwWindowID);

					kShowWindow(qwMainWindowID, TRUE);


					return TRUE;
				}

				break;
				
			default:
				
				break;
		}
	}


	// free buffer used for JPEG file decoding
	kFreeMemory(pstJpeg);
	kFreeMemory(pstOutputBuffer);


	return TRUE;
}



