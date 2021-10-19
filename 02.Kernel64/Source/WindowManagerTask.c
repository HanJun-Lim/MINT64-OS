#include "Types.h"
#include "Window.h"
#include "WindowManagerTask.h"
#include "VBE.h"
#include "Mouse.h"
#include "Task.h"
#include "GUITask.h"
#include "Font.h"
#include "ApplicationPanelTask.h"


// Window Manager Task
void kStartWindowManager(void)
{
	int iMouseX, iMouseY;
	BOOL bMouseDataResult;
	BOOL bKeyDataResult;
	BOOL bEventQueueResult;
	WINDOWMANAGER* pstWindowManager;


	/************** for LPS (Loop Per Second) measuring ****************
	QWORD qwLastTickCount;
	QWORD qwPreviousLoopExecutionCount;
	QWORD qwLoopExecutionCount;
	QWORD qwMinLoopExecutionCount;
	char  vcTemp[40];
	RECT  stLoopCountArea;
	QWORD qwBackgroundWindowID;
	// *******************************************************************/
	

	// init GUI System (init Window Manager)
	kInitializeGUISystem();
	
	
	// draw Cursor at current Mouse position
	kGetCursorPosition(&iMouseX, &iMouseY);
	kMoveCursor(iMouseX, iMouseY);


	// execute Application Panel Task
	kCreateTask(TASK_FLAGS_SYSTEM | TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, (QWORD)kApplicationPanelGUITask, TASK_LOADBALANCINGID);


	// get Window Manager
	pstWindowManager = kGetWindowManager();


	/************** for LPS (Loop Per Second) measuring ****************
	
	// initializing
	qwLastTickCount = kGetTickCount();
	qwPreviousLoopExecutionCount = 0;
	qwLoopExecutionCount = 0;
	qwMinLoopExecutionCount = 0xFFFFFFFFFFFFFFFF;
	qwBackgroundWindowID = kGetBackgroundWindowID();
	// *******************************************************************/
	

	// Window Manager Task loop
	while(1)
	{
		/*************** for LPS (Loop Per Second) measuring ****************
		
		// updated every second
		if(kGetTickCount() - qwLastTickCount > 1000)
		{
			qwLastTickCount = kGetTickCount();


			// compare and update qwMinLoopExecutionCount
			if( (qwLoopExecutionCount - qwPreviousLoopExecutionCount) < qwMinLoopExecutionCount )
			{
				qwMinLoopExecutionCount = qwLoopExecutionCount - qwPreviousLoopExecutionCount;
			}
	
			qwPreviousLoopExecutionCount = qwLoopExecutionCount;


			// print qwMinLoopExecutionCount to screen
			kSPrintf(vcTemp, "MIN Loop Execution Count: %d   ", qwMinLoopExecutionCount);
			kDrawText(qwBackgroundWindowID, 0, 0, RGB(0, 0, 0), RGB(255, 255, 255), vcTemp, kStrLen(vcTemp));


			// redraw part of qwMinLoopExecutionCount only
			kSetRectangleData(0, 0, kStrLen(vcTemp) * FONT_ENGLISHWIDTH, FONT_ENGLISHHEIGHT, &stLoopCountArea);
			kRedrawWindowByArea(&stLoopCountArea, qwBackgroundWindowID);
		}

		qwLoopExecutionCount++;
		*******************************************************************/


		// handle Mouse data
		// 		returns TRUE when processed Data exists
		bMouseDataResult = kProcessMouseData();


		// handle Key Data
		//		returns TRUE when processed Data exists
		bKeyDataResult = kProcessKeyData();
	
		
		// handle Data in Event Queue
		// 		Data in Event Queue must be processed quickly, ex) updating screen
		bEventQueueResult = FALSE;

		while(kProcessEventQueueData() == TRUE)
		{
			bEventQueueResult = TRUE;
		}


		// redraw Window Resize Marker
		// 		Window Resize Marker may be erased when Screen Update occurs
		if( (bEventQueueResult == TRUE) && (pstWindowManager->bWindowResizeMode == TRUE) )
		{
			// draw Window Resize Marker at the new position
			kDrawResizeMarker(&(pstWindowManager->stResizingWindowArea), TRUE);
		}
		
		
		// if there's no processed Data, call kSleep() for scheduling
		if( (bMouseDataResult == FALSE) && (bKeyDataResult == FALSE) && (bEventQueueResult == FALSE) )
		{
			kSleep(0);
		}

	}	// Window Manager Task loop 

}



BOOL kProcessMouseData(void)
{
	QWORD qwWindowIDUnderMouse;
	BYTE  bButtonStatus;
	int   iRelativeX, iRelativeY;
	int   iMouseX, iMouseY;
	int   iPreviousMouseX, iPreviousMouseY;
	BYTE  bChangedButton;
	RECT  stWindowArea;
	EVENT stEvent;
	WINDOWMANAGER* pstWindowManager;
	char  vcTempTitle[WINDOW_TITLEMAXLENGTH];
	int   i;
	int   iWidth, iHeight;


	// get Window Manager
	pstWindowManager = kGetWindowManager();


	//////////////////////////////////////////////////
	// Mouse Event integration
	//////////////////////////////////////////////////
	

	for(i=0; i<WINDOWMANAGER_DATAACCUMULATECOUNT; i++)
	{
		// wait for Mouse Data
		if(kGetMouseDataFromMouseQueue(&bButtonStatus, &iRelativeX, &iRelativeY) == FALSE)
		{
			// if no data at first check, exit
			if(i == 0)
			{
				return FALSE;
			}

			// handle integrated event
			else
			{
				break;
			}
		}

		
		// get current cursor pos
		kGetCursorPosition(&iMouseX, &iMouseY);
	

		// if first data, save current coordinate
		if(i == 0)
		{
			iPreviousMouseX = iMouseX;
			iPreviousMouseY = iMouseY;
		}


		// calculate current Mouse coordinate
		iMouseX += iRelativeX;
		iMouseY += iRelativeY;

		
		// move Mouse cursor to new position, then get cursor position
		// 		to prevent unexpected problem by cursor out of screen
		kMoveCursor(iMouseX, iMouseY);
		kGetCursorPosition(&iMouseX, &iMouseY);


		// check Button Status by XOR
		bChangedButton = pstWindowManager->bPreviousButtonStatus ^ bButtonStatus;


		// if Button status changed, process Event immediately
		if(bChangedButton != 0)
		{
			break;
		}

	}	// event integration loop end



	//////////////////////////////////////////////////
	// Mouse Event processing
	//////////////////////////////////////////////////


	// search for hightest Window under Mouse Cursor 
	qwWindowIDUnderMouse = kFindWindowByPoint(iMouseX, iMouseY);


	// ================================================================================
	// check Button status, send Mouse Message, Window Message by Button status
	// ================================================================================
	
	// check changed status (flag) by XOR operation with previous Button status
	bChangedButton = pstWindowManager->bPreviousButtonStatus ^ bButtonStatus;

	
	/*
	 ****************************************************
	 *		  process when LButton status changed
	 *		- is Left Button of Mouse down or up? -
	 ****************************************************
	 */
	if(bChangedButton & MOUSE_LBUTTONDOWN)
	{

		// =========== handle the case of LButton down ==========

		if(bButtonStatus & MOUSE_LBUTTONDOWN)
		{
			// This is the case of selecting a Window
			// 		if selected Window not a Background Window, move selected one to highest
			if(qwWindowIDUnderMouse != pstWindowManager->qwBackgroundWindowID)
			{
				// move selected Window to highest Z level
				// 		also send SELECT, DESELECT Event
				kMoveWindowToTop(qwWindowIDUnderMouse);
			}

			
			// if the location of LButton down is in Title Bar,
			// check if the location is on Title or X Button
			if(kIsInTitleBar(qwWindowIDUnderMouse, iMouseX, iMouseY) == TRUE)
			{
				// if LButton location is on the X Button, send CLOSE event
				if(kIsInCloseButton(qwWindowIDUnderMouse, iMouseX, iMouseY) == TRUE)
				{
					kSetWindowEvent(qwWindowIDUnderMouse, EVENT_WINDOW_CLOSE, &stEvent);
					kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
				}

				// if LButton location is on the Resize Button, set Resize Mode, draw Resize Marker
				else if(kIsInResizeButton(qwWindowIDUnderMouse, iMouseX, iMouseY) == TRUE)
				{
					pstWindowManager->bWindowResizeMode = TRUE;
					pstWindowManager->qwResizingWindowID = qwWindowIDUnderMouse;

					kGetWindowArea(qwWindowIDUnderMouse, &(pstWindowManager->stResizingWindowArea));
					kDrawResizeMarker(&(pstWindowManager->stResizingWindowArea), TRUE);
				}

				// if LButton location is on the Title Bar, activate Window Move Mode
				else
				{
					// set Window Move Mode
					pstWindowManager->bWindowMoveMode = TRUE;

					// set qwMovingWindowID as currently moving Window's ID
					pstWindowManager->qwMovingWindowID = qwWindowIDUnderMouse;
				}
			}
			
			// if LButton location is inside of Window, send LBUTTONDOWN Event
			else
			{
				kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_LBUTTONDOWN, iMouseX, iMouseY, bButtonStatus, &stEvent);
				kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
			}
		}

		// ========== handle the case of LButton up ==========

		else
		{
			// if a Window moving, release Move Mode
			if(pstWindowManager->bWindowMoveMode == TRUE)
			{
				// clear Move Mode Flag
				pstWindowManager->bWindowMoveMode = FALSE;
				pstWindowManager->qwMovingWindowID = WINDOW_INVALIDID;
			}

			// if a Window resizing, resize target Window
			else if(pstWindowManager->bWindowResizeMode == TRUE)
			{
				// resize target Window
				iWidth = kGetRectangleWidth(&(pstWindowManager->stResizingWindowArea));
				iHeight = kGetRectangleHeight(&(pstWindowManager->stResizingWindowArea));

				kResizeWindow(pstWindowManager->qwResizingWindowID, 
							  pstWindowManager->stResizingWindowArea.iX1, pstWindowManager->stResizingWindowArea.iY1,
							  iWidth, iHeight);


				// erase Window Resize Marker
				kDrawResizeMarker(&(pstWindowManager->stResizingWindowArea), FALSE);


				// send Resize Event to target Window
				kSetWindowEvent(pstWindowManager->qwResizingWindowID, EVENT_WINDOW_RESIZE, &stEvent);
				kSendEventToWindow(pstWindowManager->qwResizingWindowID, &stEvent);


				// clear Resize Mode Flag
				pstWindowManager->bWindowResizeMode = FALSE;
				pstWindowManager->qwResizingWindowID = WINDOW_INVALIDID;
			}

			// if no moving, send LBUTTONUP Event
			else
			{
				kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_LBUTTONUP, iMouseX, iMouseY, bButtonStatus, &stEvent);
				kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
			}
		}
	}

	/*
	 ****************************************************
	 *		  process when RButton status changed
	 *		- is Right Button of Mouse down or up? -
	 ****************************************************
	 */
	else if(bChangedButton & MOUSE_RBUTTONDOWN)
	{
		// ========== handle the case of RButton down ==========
		
		if(bButtonStatus & MOUSE_RBUTTONDOWN)
		{
			// send RBUTTONDOWN Event
			kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_RBUTTONDOWN, iMouseX, iMouseY, bButtonStatus, &stEvent);
			kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
		}

		// ========== handle the case of RButton up ==========
		
		else
		{
			// send RBUTTONUP Event
			kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_RBUTTONUP, iMouseX, iMouseY, bButtonStatus, &stEvent);
			kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
		}
	}

	/*
	 ****************************************************
	 *		  process when MButton status changed
	 *		- is Middle Button of Mouse down or up? -
	 ****************************************************
	 */
	else if(bChangedButton & MOUSE_MBUTTONDOWN)
	{
		// ========== handle the case of MButton down ==========
		
		if(bButtonStatus & MOUSE_MBUTTONDOWN)
		{
			// send MBUTTONDOWN Event
			kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MBUTTONDOWN, iMouseX, iMouseY, bButtonStatus, &stEvent);
			kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
		}

		// ========== handle the case of MButton up ==========

		else
		{
			// send MBUTTONUP Event
			kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MBUTTONUP, iMouseX, iMouseY, bButtonStatus, &stEvent);
			kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
		}
	}

	/*
	 ****************************************************
	 *		  if Mouse Button status not changed,
	 *		  process Mouse Movement
	 ****************************************************
	 */
	else
	{
		// send MOVE Event
		kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MOVE, iMouseX, iMouseY, bButtonStatus, &stEvent);
		kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
	}



	// -------------------------------------------------------
	// handle Window Move, Window Resize
	// -------------------------------------------------------

	// if moving Window exist, process Window movement
	if(pstWindowManager->bWindowMoveMode == TRUE)
	{
		// get Window location
		if(kGetWindowArea(pstWindowManager->qwMovingWindowID, &stWindowArea) == TRUE)
		{
			// move Window as much as Mouse moving distance
			//		kMoveWindow() sends Event
			kMoveWindow(pstWindowManager->qwMovingWindowID, 
						stWindowArea.iX1 + iMouseX - iPreviousMouseX,
						stWindowArea.iY1 + iMouseY - iPreviousMouseY);
		}

		// To fail to get Window location means Window not exist
		// 		so, release Window Move Mode
		else
		{
			// release Move Flag
			pstWindowManager->bWindowMoveMode = FALSE;
			pstWindowManager->qwMovingWindowID = WINDOW_INVALIDID;
		}
	}

	// if resizing Window exist, process Window resizing
	else if(pstWindowManager->bWindowResizeMode == TRUE)
	{
		// erase previous Window Resize Marker
		kDrawResizeMarker(&(pstWindowManager->stResizingWindowArea), FALSE);


		// calculate new Window Size by Mouse movement distance
		pstWindowManager->stResizingWindowArea.iX2 += iMouseX - iPreviousMouseX;
		pstWindowManager->stResizingWindowArea.iY1 += iMouseY - iPreviousMouseY;
		

		// if resized Window area is under minimum size, set the size to minimum

		//		adjust width
		if( (pstWindowManager->stResizingWindowArea.iX2 < pstWindowManager->stResizingWindowArea.iX1) ||
			(kGetRectangleWidth(&(pstWindowManager->stResizingWindowArea)) < WINDOW_WIDTH_MIN) )
		{
			pstWindowManager->stResizingWindowArea.iX2 = pstWindowManager->stResizingWindowArea.iX1 + WINDOW_WIDTH_MIN - 1;
		}

		//		adjust height
		if( (pstWindowManager->stResizingWindowArea.iY2 < pstWindowManager->stResizingWindowArea.iY1) ||
			(kGetRectangleHeight(&(pstWindowManager->stResizingWindowArea)) < WINDOW_HEIGHT_MIN) )
		{
			pstWindowManager->stResizingWindowArea.iY1 = pstWindowManager->stResizingWindowArea.iY2 - WINDOW_HEIGHT_MIN - 1;
		}


		// draw Window Resize Marker at the new position
		kDrawResizeMarker(&(pstWindowManager->stResizingWindowArea), TRUE);
	}


	// save current Button status
	pstWindowManager->bPreviousButtonStatus = bButtonStatus;


	return TRUE;
}


BOOL kProcessKeyData(void)
{
	KEYDATA stKeyData;
	EVENT stEvent;
	QWORD qwActiveWindowID;


	// receive Keyboard Data
	if(kGetKeyFromKeyQueue(&stKeyData) == FALSE)
	{
		return FALSE;
	}
	
	
	// send Message to selected Window (hightest Window)
	qwActiveWindowID = kGetTopWindowID();
	kSetKeyEvent(qwActiveWindowID, &stKeyData, &stEvent);		// create Key Event and store

	
	return kSendEventToWindow(qwActiveWindowID, &stEvent);
}


BOOL kProcessEventQueueData(void)
{
	EVENT vstEvent[WINDOWMANAGER_DATAACCUMULATECOUNT];
	int iEventCount;
	WINDOWEVENT* pstWindowEvent;
	WINDOWEVENT* pstNextWindowEvent;
	QWORD qwWindowID;
	RECT stArea;
	RECT stOverlappedArea;
	int i;
	int j;


	///////////////////////////////////////////////////
	// Window Manager Task Event integration
	///////////////////////////////////////////////////

	
	// Window Event gathering
	for(i=0; i<WINDOWMANAGER_DATAACCUMULATECOUNT; i++)
	{
		// wait for Event
		if(kReceiveEventFromWindowManagerQueue(&(vstEvent[i])) == FALSE)
		{
			// if no data, exit
			if(i == 0)
			{
				return FALSE;
			}
			else
			{
				break;
			}
		}

		pstWindowEvent = &(vstEvent[i].stWindowEvent);


		// if UPDATESCREENBYID Event received, insert Window Area into Event Data
		if(vstEvent[i].qwType == EVENT_WINDOWMANAGER_UPDATESCREENBYID)
		{
			// insert Window Area into Event
			if(kGetWindowArea(pstWindowEvent->qwWindowID, &stArea) == FALSE)
			{
				kSetRectangleData(0, 0, 0, 0, &(pstWindowEvent->stArea));
			}
			else
			{
				kSetRectangleData(0, 0, kGetRectangleWidth(&stArea) - 1, kGetRectangleHeight(&stArea) - 1, 
								  &(pstWindowEvent->stArea));
			}
		}

	}	// Window Manager Task Event integration loop end


	// inspect stored Event to integrate data as much as possible
	iEventCount = i;


	// Event distinguish, integration loop
	for(j=0; j<iEventCount; j++)
	{
		pstWindowEvent = &(vstEvent[j].stWindowEvent);

		if( (vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYID) &&
			(vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA) && 
			(vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA) )
		{
			continue;
		}


		for(i = j+1; i < iEventCount; i++)
		{
			pstNextWindowEvent = &(vstEvent[i].stWindowEvent);		// for comparison with previous Event

			
			if( ((vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYID) && 
				 (vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA) &&
				 (vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA)) || 
				(pstWindowEvent->qwWindowID != pstNextWindowEvent->qwWindowID) )	
				// is Window ID in next Event same with the current one?
			{
				continue;
			}

			
			// does the one cover another one?
			if(kGetOverlappedRectangle(&(pstWindowEvent->stArea), &(pstNextWindowEvent->stArea), &stOverlappedArea) == FALSE)
			{
				continue;
			}


			// integrate Event when the one covers another one or same (overlapped area exist)
			if(kMemCmp(&(pstWindowEvent->stArea), &stOverlappedArea, sizeof(RECT)) == 0)
			{
				// copy next Window Area to current Window Area
				kMemCpy(&(pstWindowEvent->stArea), &(pstNextWindowEvent->stArea), sizeof(RECT));


				// erase next Window
				vstEvent[i].qwType = EVENT_UNKNOWN;
			}
			else if(kMemCmp(&(pstNextWindowEvent->stArea), &stOverlappedArea, sizeof(RECT)) == 0)
			{
				vstEvent[i].qwType = EVENT_UNKNOWN;
			}
		}
	}

	
	///////////////////////////////////////////////////
	// process integrated Event
	///////////////////////////////////////////////////
	
	for(i=0; i<iEventCount; i++)
	{
		pstWindowEvent = &(vstEvent[i].stWindowEvent);


		// handle by type
		switch(vstEvent[i].qwType)
		{
			// update current Window Area to screen - using Window ID
			case EVENT_WINDOWMANAGER_UPDATESCREENBYID:
			// update the inside of Window to screen - using Window-based coordinate
			case EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA:

				// update by converting Window-based coordinate to Screen-based coordinate
				if(kConvertRectClientToScreen(pstWindowEvent->qwWindowID, &(pstWindowEvent->stArea), &stArea) == TRUE)
				{
					kRedrawWindowByArea(&stArea, pstWindowEvent->qwWindowID);
				}

				break;
		
			// update received Area to screen - using Screen-based coordinate
			case EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA:
				
				// WINDOW_INVALIDID : update all Window including specific Area (stArea)
				kRedrawWindowByArea(&(pstWindowEvent->stArea), WINDOW_INVALIDID);
		
				break;

			default:

				break;
		}

	}	// process integrated Event loop end


	return TRUE;
}



// draw/erase Window Resize Marker to Video Memory
void kDrawResizeMarker(const RECT* pstArea, BOOL bShowMarker)
{
	RECT stMarkerArea;
	WINDOWMANAGER* pstWindowManager;


	// get Window Manager
	pstWindowManager = kGetWindowManager();


	// drawing Window Resize Marker
	if(bShowMarker == TRUE)
	{
		// Left Top Marker
		kSetRectangleData(pstArea->iX1, pstArea->iY1, 
						  pstArea->iX1 + WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iY1 + WINDOWMANAGER_RESIZEMARKERSIZE,
						  &stMarkerArea);
		kInternalDrawRect(&(pstWindowManager->stScreenArea), pstWindowManager->pstVideoMemory,
						  stMarkerArea.iX1, stMarkerArea.iY1, stMarkerArea.iX2, stMarkerArea.iY1 + WINDOWMANAGER_THICK_RESIZEMARKER,
						  WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);
		kInternalDrawRect(&(pstWindowManager->stScreenArea), pstWindowManager->pstVideoMemory,
						  stMarkerArea.iX1, stMarkerArea.iY1, stMarkerArea.iX1 + WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iY2,
						  WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);

		// Right Top Marker
		kSetRectangleData(pstArea->iX2 - WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iY1,
						  pstArea->iX2, pstArea->iY1 + WINDOWMANAGER_RESIZEMARKERSIZE,
						  &stMarkerArea);
		kInternalDrawRect(&(pstWindowManager->stScreenArea), pstWindowManager->pstVideoMemory,
						  stMarkerArea.iX1, stMarkerArea.iY1, stMarkerArea.iX2, stMarkerArea.iY1 + WINDOWMANAGER_THICK_RESIZEMARKER,
						  WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);
		kInternalDrawRect(&(pstWindowManager->stScreenArea), pstWindowManager->pstVideoMemory,
						  stMarkerArea.iX2 - WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iY1, stMarkerArea.iX2, stMarkerArea.iY2,
						  WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);
		
		// Left Bottom Marker
		kSetRectangleData(pstArea->iX1, pstArea->iY2 - WINDOWMANAGER_RESIZEMARKERSIZE, 
						  pstArea->iX1 + WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iY2,
						  &stMarkerArea);
		kInternalDrawRect(&(pstWindowManager->stScreenArea), pstWindowManager->pstVideoMemory,
						  stMarkerArea.iX1, stMarkerArea.iY2 - WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iX2, stMarkerArea.iY2,
						  WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);
		kInternalDrawRect(&(pstWindowManager->stScreenArea), pstWindowManager->pstVideoMemory,
						  stMarkerArea.iX1, stMarkerArea.iY1, stMarkerArea.iX1 + WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iY2,
						  WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);

		// Right Bottom Marker
		kSetRectangleData(pstArea->iX2 - WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iY2 - WINDOWMANAGER_RESIZEMARKERSIZE, 
						  pstArea->iX2, pstArea->iY2,
						  &stMarkerArea);
		kInternalDrawRect(&(pstWindowManager->stScreenArea), pstWindowManager->pstVideoMemory,
						  stMarkerArea.iX1, stMarkerArea.iY2 - WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iX2, stMarkerArea.iY2,
						  WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);
		kInternalDrawRect(&(pstWindowManager->stScreenArea), pstWindowManager->pstVideoMemory,
						  stMarkerArea.iX2 - WINDOWMANAGER_THICK_RESIZEMARKER, stMarkerArea.iY1, stMarkerArea.iX2, stMarkerArea.iY2,
						  WINDOWMANAGER_COLOR_RESIZEMARKER, TRUE);
	}

	// erasing Window Resize Marker
	else
	{
		// update Marker Area only, Window Resize Marker at the each corner of area

		// Left Top Marker
		kSetRectangleData(pstArea->iX1, pstArea->iY1,
						  pstArea->iX1 + WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iY1 + WINDOWMANAGER_RESIZEMARKERSIZE,
						  &stMarkerArea);
		kRedrawWindowByArea(&stMarkerArea, WINDOW_INVALIDID);

		// Right Top Marker
		kSetRectangleData(pstArea->iX2 - WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iY1,
						  pstArea->iX2, pstArea->iY1 + WINDOWMANAGER_RESIZEMARKERSIZE,
						  &stMarkerArea);
		kRedrawWindowByArea(&stMarkerArea, WINDOW_INVALIDID);

		// Left Bottom Marker
		kSetRectangleData(pstArea->iX1, pstArea->iY2 - WINDOWMANAGER_RESIZEMARKERSIZE,
						  pstArea->iX1 + WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iY2,
						  &stMarkerArea);
		kRedrawWindowByArea(&stMarkerArea, WINDOW_INVALIDID);

		// Right Bottom Marker
		kSetRectangleData(pstArea->iX2 - WINDOWMANAGER_RESIZEMARKERSIZE, pstArea->iY2 - WINDOWMANAGER_RESIZEMARKERSIZE,
						  pstArea->iX2, pstArea->iY2,
						  &stMarkerArea);
		kRedrawWindowByArea(&stMarkerArea, WINDOW_INVALIDID);
	}
}



