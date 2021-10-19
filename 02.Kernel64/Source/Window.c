#include "Window.h"
#include "VBE.h"
#include "Task.h"
#include "Font.h"
#include "DynamicMemory.h"
#include "Utility.h"
#include "JPEG.h"



// for GUI System
static WINDOWPOOLMANAGER gs_stWindowPoolManager;

// for Window Manager
static WINDOWMANAGER gs_stWindowManager;



// ==========================================================
// for Window Pool
// ==========================================================

static void kInitializeWindowPool(void)
{
	int i;
	void* pvWindowPoolAddress;


	kMemSet(&gs_stWindowPoolManager, 0, sizeof(gs_stWindowPoolManager));

	
	// allocate memory for Window Pool
	pvWindowPoolAddress = (void*)kAllocateMemory(sizeof(WINDOW) * WINDOW_MAXCOUNT);
	
	if(pvWindowPoolAddress == NULL)
	{
		kPrintf("Window Pool Allocate Fail\n");

		while(1);
	}


	// set Window Pool Address
	gs_stWindowPoolManager.pstStartAddress = (WINDOW*)pvWindowPoolAddress;
	kMemSet(pvWindowPoolAddress, 0, sizeof(WINDOW) * WINDOW_MAXCOUNT);


	// allocate Window ID in Window Pool
	for(i=0; i<WINDOW_MAXCOUNT; i++)
	{
		gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID = i;
	}


	// set Window Max Count, Allocation Count
	gs_stWindowPoolManager.iMaxCount = WINDOW_MAXCOUNT;
	gs_stWindowPoolManager.iAllocatedCount = 1;


	// init Mutex
	kInitializeMutex(&(gs_stWindowPoolManager.stLock));
}


static WINDOW* kAllocateWindow(void)
{
	WINDOW* pstEmptyWindow;
	int i;

	
	/////////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stWindowPoolManager.stLock));
	/////////////////////////////////////////
	

	// if all Window allocated, fail
	if(gs_stWindowPoolManager.iUseCount == gs_stWindowPoolManager.iMaxCount)
	{
		// CRITICAL SECTION END
		kUnlock(&gs_stWindowPoolManager.stLock);

		return NULL;
	}


	// search for empty Window (not allocated one)
	for(i=0; i<gs_stWindowPoolManager.iMaxCount; i++)
	{
		// upper 32 bit with 0 means not allocated (available)
		if((gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0)
		{
			pstEmptyWindow = &(gs_stWindowPoolManager.pstStartAddress[i]);
			
			break;
		}
	}


	// mark as allocated by making upper 32 bit non-zero
	pstEmptyWindow->stLink.qwID = ((QWORD)gs_stWindowPoolManager.iAllocatedCount << 32) | i;
	

	// increase Allocation Count, Use Count
	gs_stWindowPoolManager.iUseCount++;
	gs_stWindowPoolManager.iAllocatedCount++;

	if(gs_stWindowPoolManager.iAllocatedCount == 0)
	{
		gs_stWindowPoolManager.iAllocatedCount = 1;
	}


	/////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stWindowPoolManager.stLock));
	/////////////////////////////////////////
	

	// init Mutex of a Window
	kInitializeMutex(&(pstEmptyWindow->stLock));


	return pstEmptyWindow;
}


static void kFreeWindow(QWORD qwID)
{
	int i;

	
	// calculate Window Pool Offset
	// 		lower 32 bit means Index(Offset)
	i = GETWINDOWOFFSET(qwID);

	
	/////////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stWindowPoolManager.stLock));
	/////////////////////////////////////////
	
	
	// initialize WINDOW Data Structure, set ID
	kMemSet(&(gs_stWindowPoolManager.pstStartAddress[i]), 0, sizeof(WINDOW));
	gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID = i;


	// decrease Use Count
	gs_stWindowPoolManager.iUseCount--;

	
	/////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stWindowPoolManager.stLock));
	/////////////////////////////////////////
}



// ==========================================================
// for Window, Window Manager
// ==========================================================

void kInitializeGUISystem(void)
{
	VBEMODEINFOBLOCK* pstModeInfo;
	QWORD qwBackgroundWindowID;
	EVENT* pstEventBuffer;


	// init Window Pool
	kInitializeWindowPool();

	
	// get VBE Mode Info Block
	pstModeInfo = kGetVBEModeInfoBlock();


	// set Video Memory Address
	gs_stWindowManager.pstVideoMemory = (COLOR*)((QWORD)pstModeInfo->dwPhysicalBasePointer & 0xFFFFFFFF);


	// set Mouse Cursor position
	gs_stWindowManager.iMouseX = pstModeInfo->wXResolution / 2;
	gs_stWindowManager.iMouseY = pstModeInfo->wYResolution / 2;

	
	// set Screen Area range
	gs_stWindowManager.stScreenArea.iX1 = 0;
	gs_stWindowManager.stScreenArea.iY1 = 0;
	gs_stWindowManager.stScreenArea.iX2 = pstModeInfo->wXResolution - 1;
	gs_stWindowManager.stScreenArea.iY2 = pstModeInfo->wYResolution - 1;


	// init Mutex
	kInitializeMutex(&(gs_stWindowManager.stLock));

	
	// init Window List
	kInitializeList(&(gs_stWindowManager.stWindowList));


	// create EVENT Data Structure Pool
	pstEventBuffer = (EVENT*)kAllocateMemory(sizeof(EVENT) * EVENTQUEUE_WINDOWMANAGERMAXCOUNT);

	if(pstEventBuffer == NULL)
	{
		kPrintf("Window Manager Event Queue Allocation Fail\n");

		while(1);
	}

	
	// init Event Queue
	kInitializeQueue(&(gs_stWindowManager.stEventQueue), pstEventBuffer, EVENTQUEUE_WINDOWMANAGERMAXCOUNT, sizeof(EVENT));


	// create Bitmap Buffer (size is as much as whole Screen size)
	gs_stWindowManager.pbDrawBitmap = kAllocateMemory((pstModeInfo->wXResolution * pstModeInfo->wYResolution + 7) / 8);

	if(gs_stWindowManager.pbDrawBitmap == NULL)
	{
		kPrintf("Draw Bitmap Allocation Fail\n");

		while(1);
	}


	// init Mouse Button status, Window Move Flag
	gs_stWindowManager.bPreviousButtonStatus = 0;
	gs_stWindowManager.qwMovingWindowID 	 = WINDOW_INVALIDID;
	gs_stWindowManager.bWindowMoveMode  	 = FALSE;


	// init Resize Info
	gs_stWindowManager.bWindowResizeMode	 = FALSE;
	gs_stWindowManager.qwResizingWindowID	 = WINDOW_INVALIDID;
	kMemSet(&(gs_stWindowManager.stResizingWindowArea), 0, sizeof(RECT));


	// ----------------------------------
	// Create Background Window
	// ----------------------------------
	
	// set Flag as 0 not to draw Window
	// 		Background Window will be shown after Background Color filled
	qwBackgroundWindowID = kCreateWindow(0, 0, pstModeInfo->wXResolution, pstModeInfo->wYResolution, 
										 0, WINDOW_BACKGROUNDWINDOWTITLE);
	gs_stWindowManager.qwBackgroundWindowID = qwBackgroundWindowID;


	// fill Background Color in Background Window
	kDrawRect(qwBackgroundWindowID, 0, 0, pstModeInfo->wXResolution - 1, pstModeInfo->wYResolution - 1,
			  WINDOW_COLOR_SYSTEMBACKGROUND, TRUE);


	// draw Background Image (wallpaper)
	kDrawBackgroundImage();


	// show Background Window
	kShowWindow(qwBackgroundWindowID, TRUE);
}


WINDOWMANAGER* kGetWindowManager(void)
{
	return &gs_stWindowManager;
}


QWORD kGetBackgroundWindowID(void)
{
	return gs_stWindowManager.qwBackgroundWindowID;
}


void kGetScreenArea(RECT* pstScreenArea)
{
	kMemCpy(pstScreenArea, &(gs_stWindowManager.stScreenArea), sizeof(RECT));
}


// Create Window
// 		also sends SELECT, DESELECT Event
QWORD kCreateWindow(int iX, int iY, int iWidth, int iHeight, DWORD dwFlags, const char* pcTitle)
{
	WINDOW* pstWindow;
	TCB* pstTask;
	QWORD qwActiveWindowID;
	EVENT stEvent;


	// the Window with size 0 cannot be created
	if( (iWidth <= 0) || (iHeight <= 0) )
	{
		return WINDOW_INVALIDID;
	}

	
	// check Width, Height
	if(dwFlags & WINDOW_FLAGS_DRAWTITLE)
	{
		if(iWidth < WINDOW_WIDTH_MIN)
		{
			iWidth = WINDOW_WIDTH_MIN;
		}

		if(iHeight < WINDOW_HEIGHT_MIN)
		{
			iHeight = WINDOW_HEIGHT_MIN;
		}
	}


	// allocate WINDOW Data Structure
	pstWindow = kAllocateWindow();

	if(pstWindow == NULL)
	{
		return WINDOW_INVALIDID;
	}


	// set Window Area
	pstWindow->stArea.iX1 = iX;
	pstWindow->stArea.iY1 = iY;
	pstWindow->stArea.iX2 = iX + iWidth - 1;
	pstWindow->stArea.iY2 = iY + iHeight - 1;


	// set Window Title
	kMemCpy(pstWindow->vcWindowTitle, pcTitle, WINDOW_TITLEMAXLENGTH);
	pstWindow->vcWindowTitle[WINDOW_TITLEMAXLENGTH] = '\0';


	// allocate Window Buffer, Event Buffer
	pstWindow->pstWindowBuffer = (COLOR*)kAllocateMemory(iWidth * iHeight * sizeof(COLOR));
	pstWindow->pstEventBuffer = (EVENT*)kAllocateMemory(EVENTQUEUE_WINDOWMAXCOUNT * sizeof(EVENT));

	if( (pstWindow->pstWindowBuffer == NULL) || (pstWindow->pstEventBuffer == NULL) )
	{
		// free Window Buffer, Event Buffer
		kFreeMemory(pstWindow->pstWindowBuffer);
		kFreeMemory(pstWindow->pstEventBuffer);

		// if failed to allocate memory, free WINDOW Data Structure
		kFreeWindow(pstWindow->stLink.qwID);

		return WINDOW_INVALIDID;
	}


	// init Event Queue
	kInitializeQueue(&(pstWindow->stEventQueue), pstWindow->pstEventBuffer, EVENTQUEUE_WINDOWMAXCOUNT, sizeof(EVENT));


	// save Task ID which creates Window
	pstTask = kGetRunningTask(kGetAPICID());
	pstWindow->qwTaskID = pstTask->stLink.qwID;


	// set Window Attribute
	pstWindow->dwFlags = dwFlags;


	// draw Window Background
	kDrawWindowBackground(pstWindow->stLink.qwID);


	// draw Window Frame
	if(dwFlags & WINDOW_FLAGS_DRAWFRAME)
	{
		kDrawWindowFrame(pstWindow->stLink.qwID);
	}


	// draw Window Title Bar
	if(dwFlags & WINDOW_FLAGS_DRAWTITLE)
	{
		kDrawWindowTitle(pstWindow->stLink.qwID, pcTitle, TRUE);
	}

	
	///////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stWindowManager.stLock));	
	///////////////////////////////////////

	// get Top Window
	qwActiveWindowID = kGetTopWindowID();		// previous Top Window
	

	// add Window to header of List (set Window as Top Window)
	kAddListToHeader(&gs_stWindowManager.stWindowList, pstWindow);


	///////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stWindowManager.stLock));
	///////////////////////////////////////
	

	// ---------------------------------------------------------------
	// send Window Event
	// ---------------------------------------------------------------
	
	// update screen as much as Window area and send SELECT Event
	kUpdateScreenByID(pstWindow->stLink.qwID);

	kSetWindowEvent(pstWindow->stLink.qwID, EVENT_WINDOW_SELECT, &stEvent);
	kSendEventToWindow(pstWindow->stLink.qwID, &stEvent);

	// if previous Top Window not a Background Window..
	// 		make previous Top Window as deselected, send DESELECT Event
	if(qwActiveWindowID != gs_stWindowManager.qwBackgroundWindowID)
	{
		kUpdateWindowTitle(qwActiveWindowID, FALSE);

		kSetWindowEvent(qwActiveWindowID, EVENT_WINDOW_DESELECT, &stEvent);
		kSendEventToWindow(qwActiveWindowID, &stEvent);
	}


	/*
	// if 'show' option set, draw Window
	if(dwFlags & WINDOW_FLAGS_SHOW)
	{
		// update Window to screen
		kRedrawWindowByArea(&(pstWindow->stArea));
	}
	*/


	return pstWindow->stLink.qwID;
}


// Delete Window
// 		also sends SELECT Event
BOOL kDeleteWindow(QWORD qwWindowID)
{
	WINDOW* pstWindow;
	RECT stArea;
	QWORD qwActiveWindowID;
	BOOL bActiveWindow;
	EVENT stEvent;

	
	///////////////////////////////////////
	// CRITICAL SECTION START - Window Manager
	kLock(&(gs_stWindowManager.stLock));
	///////////////////////////////////////
	

	///////////////////////////////////////
	// search Window, CRITICAL SECTION START - Window
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	///////////////////////////////////////

	if(pstWindow == NULL)
	{
		// CRITICAL SECTION END - Window Manager
		kUnlock(&(gs_stWindowManager.stLock));

		return FALSE;
	}


	// save Area before deletion
	kMemCpy(&stArea, &(pstWindow->stArea), sizeof(RECT));


	// get Top Window
	qwActiveWindowID = kGetTopWindowID();


	// if the Target to delete is Top Window, set Flag
	if(qwActiveWindowID == qwWindowID)
	{
		bActiveWindow = TRUE;
	}
	else
	{
		bActiveWindow = FALSE;
	}


	// remove Window from Window List
	if(kRemoveList(&(gs_stWindowManager.stWindowList), qwWindowID) == NULL)
	{
		// CRITICAL SECTION END - Window, Window Manager
		kUnlock(&(pstWindow->stLock));
		kUnlock(&(gs_stWindowManager.stLock));

		return FALSE;
	}

	
	// ---------------------------------------------------------
	// free Window Buffer, Event Buffer
	// ---------------------------------------------------------
	
	// free Window Buffer
	kFreeMemory(pstWindow->pstWindowBuffer);
	pstWindow->pstWindowBuffer = NULL;


	// free Event Buffer
	kFreeMemory(pstWindow->pstEventBuffer);
	pstWindow->pstEventBuffer = NULL;


	///////////////////////////////////////
	// CRITICAL SECTION END - Window
	kUnlock(&(pstWindow->stLock));
	///////////////////////////////////////


	// free WINDOW Data Structure
	kFreeWindow(qwWindowID);


	///////////////////////////////////////
	// CRITICAL SECTION END - Window Manager
	kUnlock(&(gs_stWindowManager.stLock));
	///////////////////////////////////////


	// update area which previous Window existed
	kUpdateScreenByScreenArea(&stArea);


	// ---------------------------------------------------------
	// if Top Window erased, activate Top window of current List
	// and send SELECT Event
	// ---------------------------------------------------------
	
	if(bActiveWindow == TRUE)
	{
		// get Top Window
		qwActiveWindowID = kGetTopWindowID();


		// mark Title Bar of Top Window as activated (selected)
		if(qwActiveWindowID != WINDOW_INVALIDID)
		{
			kUpdateWindowTitle(qwActiveWindowID, TRUE);

			kSetWindowEvent(qwActiveWindowID, EVENT_WINDOW_SELECT, &stEvent);
			kSendEventToWindow(qwActiveWindowID, &stEvent);
		}
	}


	return TRUE;
}


BOOL kDeleteAllWindowInTaskID(QWORD qwTaskID)
{
	WINDOW* pstWindow;
	WINDOW* pstNextWindow;

	
	///////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stWindowManager.stLock));
	///////////////////////////////////////
	
	
	// get first Window from List
	pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));


	// linear search 
	while(pstWindow != NULL)
	{
		// get next Window
		pstNextWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList), pstWindow);


		// if No Background Window and Task ID match, delete Window
		if( (pstWindow->stLink.qwID != gs_stWindowManager.qwBackgroundWindowID) && (pstWindow->qwTaskID == qwTaskID) )
		{
			kDeleteWindow(pstWindow->stLink.qwID);
		}

		
		// move to next Window
		pstWindow = pstNextWindow;
	}


	///////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stWindowManager.stLock));
	///////////////////////////////////////
}


WINDOW* kGetWindow(QWORD qwWindowID)
{
	WINDOW* pstWindow;


	// is Window ID valid?
	if(GETWINDOWOFFSET(qwWindowID) >= WINDOW_MAXCOUNT)
	{
		return NULL;
	}


	// search WINDOW pointer with ID
	pstWindow = &gs_stWindowPoolManager.pstStartAddress[GETWINDOWOFFSET(qwWindowID)];

	if(pstWindow->stLink.qwID == qwWindowID)
	{
		return pstWindow;
	}


	return NULL;
}


// used in case that needed to modify Window Information (Window position, modify Window Buffer...)
//
// WARNING : this function should not be called when other Window's Mutex locked.
// 			 Window Manager Mutex called first before Window Mutex calling
WINDOW* kGetWindowWithWindowLock(QWORD qwWindowID)
{
	WINDOW* pstWindow;
	BOOL bResult;


	// search for Window
	pstWindow = kGetWindow(qwWindowID);

	if(pstWindow == NULL)
	{
		return NULL;
	}

	
	///////////////////////////////////////
	// CRITICAL SECTION START, search for Window again
	kLock(&(pstWindow->stLock));
	///////////////////////////////////////
	
	
	// if Window cannot be found after synchronization, that means Window has been changed
	pstWindow = kGetWindow(qwWindowID);

	if(pstWindow == NULL)
	{
		// CRITICAL SECTION END
		kUnlock(&(pstWindow->stLock));

		return NULL;
	}


	return pstWindow;
}


BOOL kShowWindow(QWORD qwWindowID, BOOL bShow)
{
	WINDOW* pstWindow;
	RECT stWindowArea;

	
	//////////////////////////////////////////////
	// search Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	//////////////////////////////////////////////

	if(pstWindow == NULL)
	{
		return FALSE;
	}


	// set Window Attribute
	if(bShow == TRUE)
	{
		pstWindow->dwFlags |= WINDOW_FLAGS_SHOW;
	}
	else
	{
		pstWindow->dwFlags &= ~WINDOW_FLAGS_SHOW;
	}


	//////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(pstWindow->stLock));
	//////////////////////////////////////////////
	

	// show/hide Window by update previous Window Area
	if(bShow == TRUE)
	{
		kUpdateScreenByID(qwWindowID);
	}
	else
	{
		kGetWindowArea(qwWindowID, &stWindowArea);
		kUpdateScreenByScreenArea(&stWindowArea);
	}


	return TRUE;
}


// Draw all Window containing certain area
// 		- called by Window Manager, User calls functions kind of kUpdateScreen() instead
BOOL kRedrawWindowByArea(const RECT* pstArea, QWORD qwDrawWindowID)
{
	WINDOW* pstWindow;
	WINDOW* pstTargetWindow = NULL;
	RECT stOverlappedArea;
	RECT stCursorArea;
	DRAWBITMAP stDrawBitmap;
	RECT stTempOverlappedArea;
	RECT vstLargestOverlappedArea[WINDOW_OVERLAPPEDAREALOGMAXCOUNT];
	int viLargestOverlappedAreaSize[WINDOW_OVERLAPPEDAREALOGMAXCOUNT];
	int iTempOverlappedAreaSize;
	int iMinAreaSize;
	int iMinAreaIndex;
	int i;

	
	// if overlapped Area with Screen not exist (out of Screen), no need to draw
	if(kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), pstArea, &stOverlappedArea) == FALSE)
	{
		return FALSE;
	}

	
	// ---------------------------------------------------------------------------------
	// search for Window which overlapped with updating Area
	// by looping from Window List Header to Tail (from hightest Z to lowest Z)
	// ---------------------------------------------------------------------------------
	
	// initialize space which saves Screen Update log
	kMemSet(viLargestOverlappedAreaSize, 0, sizeof(viLargestOverlappedAreaSize));
	kMemSet(vstLargestOverlappedArea, 0, sizeof(vstLargestOverlappedArea));
	

	//////////////////////////////////////
	// CRITICAL SECTION START - Window Manager
	kLock(&(gs_stWindowManager.stLock));
	//////////////////////////////////////
	
	// create Bitmap which saves Area to update
	kCreateDrawBitmap(&stOverlappedArea, &stDrawBitmap);


	// Window List is sorted in Z order (latest Window is the lowest)
	pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));

	while(pstWindow != NULL)
	{
		// send overlapped area to screen when Show option set, and overlapped area between Window area and Update area exists
		if( (pstWindow->dwFlags & WINDOW_FLAGS_SHOW) && 
			(kGetOverlappedRectangle(&(pstWindow->stArea), &stOverlappedArea, &stTempOverlappedArea) == TRUE) )
		{
			// get overlapped area size
			iTempOverlappedAreaSize = kGetRectangleWidth(&stTempOverlappedArea) * kGetRectangleHeight(&stTempOverlappedArea);
			
			
			// check if included in previous area by searching log
			for(i=0; i<WINDOW_OVERLAPPEDAREALOGMAXCOUNT; i++)
			{
				// stop updating if Previous update area covers Current update area
				if( (iTempOverlappedAreaSize <= viLargestOverlappedAreaSize[i]) &&
					(kIsInRectangle(&(vstLargestOverlappedArea[i]), stTempOverlappedArea.iX1, stTempOverlappedArea.iY1) == TRUE) &&
					(kIsInRectangle(&(vstLargestOverlappedArea[i]), stTempOverlappedArea.iX2, stTempOverlappedArea.iY2) == TRUE) )
				{
					break;
				}
			}


			// if match update area found, that means be updated before
			// so, move to next Window
			if(i < WINDOW_OVERLAPPEDAREALOGMAXCOUNT)
			{
				pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList), pstWindow);

				continue;
			}


			// if current area is not completely included with the largest one of previously updated area,
			// 		get minimum area by comparing size
			iMinAreaSize = 0xFFFFFF;
			iMinAreaIndex = 0;

			for(i=0; i<WINDOW_OVERLAPPEDAREALOGMAXCOUNT; i++)
			{
				if(viLargestOverlappedAreaSize[i] < iMinAreaSize)
				{
					iMinAreaSize = viLargestOverlappedAreaSize[i];
					iMinAreaIndex = i;
				}
			}

			
			// if Current overlapped area size > stored minimum size, replace
			if(iMinAreaSize < iTempOverlappedAreaSize)
			{
				kMemCpy(&(vstLargestOverlappedArea[iMinAreaIndex]), &stTempOverlappedArea, sizeof(RECT));

				viLargestOverlappedAreaSize[iMinAreaIndex] = iTempOverlappedAreaSize;
			}


			////////////////////////////////////////
			// CRITICAL SECTION START - Window
			kLock(&(pstWindow->stLock));
			////////////////////////////////////////


			// if Window ID vaild, make Bitmap as updated (no drawing)
			if( (qwDrawWindowID != WINDOW_INVALIDID) && (qwDrawWindowID != pstWindow->stLink.qwID) )
			{
				kFillDrawBitmap(&stDrawBitmap, &(pstWindow->stArea), FALSE);		// 0 means updated
			}
			else
			{
				// send Window Buffer to Video Memory
				kCopyWindowBufferToFrameBuffer(pstWindow, &stDrawBitmap);
			}


			////////////////////////////////////////
			// CRITICAL SECTION END - Window
			kUnlock(&(pstWindow->stLock));
			////////////////////////////////////////
		}

		// if all Area updated, no need to draw
		if(kIsDrawBitmapAllOff(&stDrawBitmap) == TRUE)
		{
			break;
		}
		

		// move to next Window
		pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList), pstWindow);
	}

	
	/////////////////////////////////////
	// CRITICAL SECTION END - Window Manager
	kUnlock(&(gs_stWindowManager.stLock));
	/////////////////////////////////////
	
	
	// ---------------------------------------------------------------------------------
	// if Mouse Cursor Area included, draw Mouse Cursor also
	// ---------------------------------------------------------------------------------
	
	// set Mouse Area to RECT Data Structure
	kSetRectangleData(gs_stWindowManager.iMouseX, gs_stWindowManager.iMouseY,
					  gs_stWindowManager.iMouseX + MOUSE_CURSOR_WIDTH, gs_stWindowManager.iMouseY + MOUSE_CURSOR_HEIGHT,
					  &stCursorArea);


	// check if Mouse Area overlapped
	if(kIsRectangleOverlapped(&stOverlappedArea, &stCursorArea) == TRUE)
	{
		kDrawCursor(gs_stWindowManager.iMouseX, gs_stWindowManager.iMouseY);
	}
}


// copy some part or all of Window Buffer to Frame Buffer
// 		- send overlapped area only
static void kCopyWindowBufferToFrameBuffer(const WINDOW* pstWindow, DRAWBITMAP* pstDrawBitmap)
{
	RECT stTempArea;
	RECT stOverlappedArea;
	int iOverlappedWidth;
	int iOverlappedHeight;
	int iScreenWidth;
	int iWindowWidth;
	int i;
	COLOR* pstCurrentVideoMemoryAddress;
	COLOR* pstCurrentWindowBufferAddress;
	BYTE bTempBitmap;
	int iByteOffset;
	int iBitOffset;
	int iOffsetX;
	int iOffsetY;
	int iLastBitOffset;
	int iBulkCount;

	
	// get overlapped part between screen and area to send
	if(kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), &(pstDrawBitmap->stArea), &stTempArea) == FALSE)
	{
		return;
	}


	// get overlapped part between Window area and temporary area
	// 		if No overlapped part found, no need to send to Video Memory
	if(kGetOverlappedRectangle(&stTempArea, &(pstWindow->stArea), &stOverlappedArea) == FALSE)
	{
		return;
	}


	// get width, height of each area
	iScreenWidth = kGetRectangleWidth(&(gs_stWindowManager.stScreenArea));
	iWindowWidth = kGetRectangleWidth(&(pstWindow->stArea));
	iOverlappedWidth = kGetRectangleWidth(&stOverlappedArea);
	iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);

	
	// --------------------------------------------------------------------------
	// printing loop for height of overlapped area
	// --------------------------------------------------------------------------

	for(iOffsetY = 0; iOffsetY < iOverlappedHeight; iOffsetY++)
	{
		// get position of overlapped area in Bitmap
		if(kGetStartPositionInDrawBitmap(pstDrawBitmap, stOverlappedArea.iX1, stOverlappedArea.iY1 + iOffsetY,
										 &iByteOffset, &iBitOffset) == FALSE)
		{
			break;
		}

		
		// calculate the address of Video Memory to send, and of Window Buffer
		// 		convert Screen-based coord of overlapped area to Window-based
		pstCurrentVideoMemoryAddress = gs_stWindowManager.pstVideoMemory + (stOverlappedArea.iY1 + iOffsetY) * iScreenWidth + 
									   stOverlappedArea.iX1;

		pstCurrentWindowBufferAddress = pstWindow->pstWindowBuffer + (stOverlappedArea.iY1 - pstWindow->stArea.iY1 + iOffsetY) * 
										iWindowWidth + (stOverlappedArea.iX1 - pstWindow->stArea.iX1);

		
		// --------------------------------------------------------------------------
		// printing loop for width of overlapped area
		// --------------------------------------------------------------------------
		
		for(iOffsetX = 0; iOffsetX < iOverlappedWidth; )
		{
			// update 8 bit at once
			if( (pstDrawBitmap->pbBitmap[iByteOffset] == 0xFF) && (iBitOffset == 0x00) &&
				((iOverlappedWidth - iOffsetX) >= 8) )
			{
				for(iBulkCount = 0; (iBulkCount < ((iOverlappedWidth - iOffsetX) >> 3)); iBulkCount++)
				{
					if(pstDrawBitmap->pbBitmap[iByteOffset + iBulkCount] != 0xFF)
					{
						break;
					}
				}


				// handle 8 Pixel
				kMemCpy(pstCurrentVideoMemoryAddress, pstCurrentWindowBufferAddress, (sizeof(COLOR) * iBulkCount) << 3);


				// update Memory Address, Bitmap info (8 pixel handled at once)
				pstCurrentVideoMemoryAddress += iBulkCount << 3;
				pstCurrentWindowBufferAddress += iBulkCount << 3;
				kMemSet(pstDrawBitmap->pbBitmap + iByteOffset, 0x00, iBulkCount);


				// update X offset
				iOffsetX += iBulkCount << 3;


				// change Bitmap Offset
				iByteOffset += iBulkCount;
				iBitOffset = 0;
			}

			// jump 8 pixel at once (in case of 8 pixel already updated)
			else if( (pstDrawBitmap->pbBitmap[iByteOffset] == 0x00) && (iBitOffset == 0x00) &&
					 ((iOverlappedWidth - iOffsetX) >= 8) )
			{
				for(iBulkCount = 0; (iBulkCount < ((iOverlappedWidth - iOffsetX) >> 3)); iBulkCount++)
				{
					if(pstDrawBitmap->pbBitmap[iByteOffset + iBulkCount] != 0x00)
					{
						break;
					}
				}

				// update Memory Address, Bitmap info (8 pixel handled at once)
				pstCurrentVideoMemoryAddress  += iBulkCount << 3;
				pstCurrentWindowBufferAddress += iBulkCount << 3;


				// update X offset
				iOffsetX += iBulkCount << 3;


				// change bitmap offset
				iByteOffset += iBulkCount;
				iBitOffset = 0;
			}
			else
			{
				// Bitmap to update
				bTempBitmap = pstDrawBitmap->pbBitmap[iByteOffset];


				iLastBitOffset = MIN(8, iOverlappedWidth - iOffsetX + iBitOffset);


				for(i = iBitOffset; i < iLastBitOffset; i++)
				{
					// if bitmap set, copy Window Buffer to Video Memory (print to screen), then clear bit
					if(bTempBitmap & (0x01 << i))
					{
						*pstCurrentVideoMemoryAddress = *pstCurrentWindowBufferAddress;


						// clear bit (updated)
						bTempBitmap &= ~(0x01 << i);
					}

					pstCurrentVideoMemoryAddress++;
					pstCurrentWindowBufferAddress++;
				}

				
				// iOffsetX += send count
				iOffsetX += (iLastBitOffset - iBitOffset);

				
				// update Bitmap info
				pstDrawBitmap->pbBitmap[iByteOffset] = bTempBitmap;
				iByteOffset++;
				iBitOffset = 0;
			}
		}	// Offset X loop end
	}	// Offset Y loop end
}



// Get Top Window at Mouse Point (X, Y)
QWORD kFindWindowByPoint(int iX, int iY)
{
	QWORD qwWindowID;
	WINDOW* pstWindow;


	// Mouse cannot be out of screen. so, the base is Background Window
	qwWindowID = gs_stWindowManager.qwBackgroundWindowID;


	////////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stWindowManager.stLock));
	////////////////////////////////////////
	
	
	// search from latest Window
	pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));

	do
	{
		// update Window ID when Window includes (iX, iY) and has SHOW option
		if( (pstWindow != NULL) && (pstWindow->dwFlags & WINDOW_FLAGS_SHOW) && 
			(kIsInRectangle(&(pstWindow->stArea), iX, iY) == TRUE) )
		{
			qwWindowID = pstWindow->stLink.qwID;
			
			break;
		}


		// get next (older) Window
		pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList), pstWindow);

	} while(pstWindow != NULL);


	////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stWindowManager.stLock));
	////////////////////////////////////////
	

	return qwWindowID;
}


QWORD kFindWindowByTitle(const char* pcTitle)
{
	QWORD qwWindowID;
	WINDOW* pstWindow;
	int iTitleLength;


	qwWindowID = WINDOW_INVALIDID;
	iTitleLength = kStrLen(pcTitle);


	////////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stWindowManager.stLock));
	////////////////////////////////////////
	

	// search from Background Window
	pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));

	while(pstWindow != NULL)
	{
		if( (kStrLen(pstWindow->vcWindowTitle) == iTitleLength) && 
			(kMemCmp(pstWindow->vcWindowTitle, pcTitle, iTitleLength) == 0) )
		{
			qwWindowID = pstWindow->stLink.qwID;

			break;
		}


		// move to next Window
		pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList), pstWindow);
	}


	////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stWindowManager.stLock));
	////////////////////////////////////////
	

	return qwWindowID;
}


BOOL kIsWindowExist(QWORD qwWindowID)
{
	// if search result is NULL, Window not exist
	if(kGetWindow(qwWindowID) == NULL)
	{
		return FALSE;
	}


	return TRUE;
}


QWORD kGetTopWindowID(void)
{
	WINDOW* pstActiveWindow;
	QWORD qwActiveWindowID;


	////////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stWindowManager.stLock));
	////////////////////////////////////////
	

	// get latest Window from Window List
	pstActiveWindow = (WINDOW*)kGetHeaderFromList(&(gs_stWindowManager.stWindowList));

	if(pstActiveWindow != NULL)
	{
		qwActiveWindowID = pstActiveWindow->stLink.qwID;
	}
	else
	{
		qwActiveWindowID = WINDOW_INVALIDID;
	}


	////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stWindowManager.stLock));
	////////////////////////////////////////
	

	return qwActiveWindowID;
}


// Move Window to Top
// 		also send SELECT, DESELECT Event
BOOL kMoveWindowToTop(QWORD qwWindowID)
{
	WINDOW* pstWindow;
	RECT stArea;
	DWORD dwFlags;
	QWORD qwTopWindowID;
	EVENT stEvent;


	// get current Top Window
	qwTopWindowID = kGetTopWindowID();

	// if qwWindowID is the Top, no need to process
	if(qwTopWindowID == qwWindowID)
	{
		return TRUE;
	}


	////////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stWindowManager.stLock));
	////////////////////////////////////////
	

	// remove from Window List and move it to the tail of List
	pstWindow = kRemoveList(&(gs_stWindowManager.stWindowList), qwWindowID);

	if(pstWindow != NULL)
	{
		kAddListToHeader(&(gs_stWindowManager.stWindowList), pstWindow);


		// convert Screen-based coordinate to Window-based
		// 		used to update Window area
		kConvertRectScreenToClient(qwWindowID, &(pstWindow->stArea), &stArea);

		dwFlags = pstWindow->dwFlags;
	}


	////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stWindowManager.stLock));
	////////////////////////////////////////

	
	// if success to move Window to the Top,
	// update Title Bar, send SELECT/DESELECT Event
	if(pstWindow != NULL)
	{
		// ----------------------------------------------
		// update selected Window's screen, send Event
		// ----------------------------------------------
		
		// send SELECT Event
		kSetWindowEvent(qwWindowID, EVENT_WINDOW_SELECT, &stEvent);
		kSendEventToWindow(qwWindowID, &stEvent);


		// if DRAWTITLE option set, make Title Bar as selected and update
		if(dwFlags & WINDOW_FLAGS_DRAWTITLE)
		{
			// update Window Title Bar as selected
			kUpdateWindowTitle(qwWindowID, TRUE);

			// update rest of area
			stArea.iY1 += WINDOW_TITLEBAR_HEIGHT;
			kUpdateScreenByWindowArea(qwWindowID, &stArea);
		}
		// if Title Bar no exist, update whole area
		else
		{
			kUpdateScreenByID(qwWindowID);
		}


		// ----------------------------------------------
		// update deselected Window's screen, send Event
		// ----------------------------------------------

		// send DESELECT Event
		kSetWindowEvent(qwTopWindowID, EVENT_WINDOW_DESELECT, &stEvent);
		kSendEventToWindow(qwTopWindowID, &stEvent);

		// update Window Title Bar as deselected
		kUpdateWindowTitle(qwTopWindowID, FALSE);
		

		return TRUE;
	}

	return FALSE;
}


BOOL kIsInTitleBar(QWORD qwWindowID, int iX, int iY)
{
	WINDOW* pstWindow;


	// find WINDOW
	pstWindow = kGetWindow(qwWindowID);

	if( (pstWindow == NULL) || ((pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE) == 0) )
	{
		return FALSE;
	}


	// is (X, Y) in Title Bar?
	if( (pstWindow->stArea.iX1 <= iX) && (iX <= pstWindow->stArea.iX2) &&
		(pstWindow->stArea.iY1 <= iY) && (iY <= pstWindow->stArea.iY1 + WINDOW_TITLEBAR_HEIGHT) )
	{
		return TRUE;
	}
		

	return FALSE;
}


BOOL kIsInCloseButton(QWORD qwWindowID, int iX, int iY)
{
	WINDOW* pstWindow;

	
	// find Window
	pstWindow = kGetWindow(qwWindowID);

	if( (pstWindow == NULL) || ((pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE) == 0) )
	{
		return FALSE;
	}

	
	// is (X, Y) in Close Button?
	if( ((pstWindow->stArea.iX2 - WINDOW_XBUTTON_SIZE - 1) <= iX) && 
		(iX <= (pstWindow->stArea.iX2 - 1)) && ((pstWindow->stArea.iY1 + 1) <= iY) &&
		(iY <= (pstWindow->stArea.iY1 + 1 + WINDOW_XBUTTON_SIZE)) )
	{
		return TRUE;
	}


	return FALSE;
}


// Move Window at (X, Y) by updating Area Information
BOOL kMoveWindow(QWORD qwWindowID, int iX, int iY)
{
	WINDOW* pstWindow;
	RECT stPreviousArea;
	int iWidth;
	int iHeight;
	EVENT stEvent;


	////////////////////////////////////////////
	// find Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}


	// save previous Area
	kMemCpy(&stPreviousArea, &(pstWindow->stArea), sizeof(RECT));


	// calc width, height to move Window
	iWidth = kGetRectangleWidth(&stPreviousArea);
	iHeight = kGetRectangleHeight(&stPreviousArea);
	kSetRectangleData(iX, iY, iX + iWidth - 1, iY + iHeight - 1, &(pstWindow->stArea));


	////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(pstWindow->stLock));
	////////////////////////////////////////////

	
	// update screen that previous Area was on
	kUpdateScreenByScreenArea(&stPreviousArea);

	
	// update current Window Area
	kUpdateScreenByID(qwWindowID);


	// send MOVE Event
	kSetWindowEvent(qwWindowID, EVENT_WINDOW_MOVE, &stEvent);
	kSendEventToWindow(qwWindowID, &stEvent);


	return TRUE;
}


static BOOL kUpdateWindowTitle(QWORD qwWindowID, BOOL bSelectedTitle)
{
	WINDOW* pstWindow;
	RECT stTitleBarArea;

	
	// find Window
	pstWindow = kGetWindow(qwWindowID);


	// if Window exists, redraw Window Title Bar
	if( (pstWindow != NULL) && (pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE) )
	{
		kDrawWindowTitle(pstWindow->stLink.qwID, pstWindow->vcWindowTitle, bSelectedTitle);

		// save Window Title Bar location (Window-based)
		stTitleBarArea.iX1 = 0;
		stTitleBarArea.iY1 = 0;
		stTitleBarArea.iX2 = kGetRectangleWidth(&(pstWindow->stArea)) - 1;
		stTitleBarArea.iY2 = WINDOW_TITLEBAR_HEIGHT;

		kUpdateScreenByWindowArea(qwWindowID, &stTitleBarArea);


		return TRUE;
	}


	return FALSE;
}


BOOL kResizeWindow(QWORD qwWindowID, int iX, int iY, int iWidth, int iHeight)
{
	WINDOW* pstWindow;
	COLOR* pstNewWindowBuffer;
	COLOR* pstOldWindowBuffer;
	RECT stPreviousArea;


	////////////////////////////////////////
	// CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}

	
	// check the size
	if(pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE)
	{
		if(iWidth < WINDOW_WIDTH_MIN)
		{
			iWidth = WINDOW_WIDTH_MIN;
		}

		if(iHeight < WINDOW_HEIGHT_MIN)
		{
			iHeight = WINDOW_HEIGHT_MIN;
		}
	}


	// allocate new Window Buffer
	pstNewWindowBuffer = (COLOR*)kAllocateMemory(iWidth * iHeight * sizeof(COLOR));

	if(pstNewWindowBuffer == NULL)
	{
		// CRITICAL SECTION END
		kUnlock(&(pstWindow->stLock));

		return FALSE;
	}


	// change Window Buffer to new one, free old Window Buffer
	pstOldWindowBuffer = pstWindow->pstWindowBuffer;
	pstWindow->pstWindowBuffer = pstNewWindowBuffer;
	kFreeMemory(pstOldWindowBuffer);


	// update Window Size info
	kMemCpy(&stPreviousArea, &(pstWindow->stArea), sizeof(RECT));
	
	pstWindow->stArea.iX1 = iX;
	pstWindow->stArea.iY1 = iY;
	pstWindow->stArea.iX2 = iX + iWidth - 1;
	pstWindow->stArea.iY2 = iY + iHeight - 1;


	// draw Window Background
	kDrawWindowBackground(qwWindowID);


	// draw Window Frame
	if(pstWindow->dwFlags & WINDOW_FLAGS_DRAWFRAME)
	{
		kDrawWindowFrame(qwWindowID);
	}


	// draw Window Title Bar
	if(pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE)
	{
		kDrawWindowTitle(qwWindowID, pstWindow->vcWindowTitle, TRUE);
	}


	////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(pstWindow->stLock));
	////////////////////////////////////////
	

	// update Window to Screen
	if(pstWindow->dwFlags & WINDOW_FLAGS_SHOW)
	{
		kUpdateScreenByScreenArea(&stPreviousArea);

		// redraw new Area
		kShowWindow(qwWindowID, TRUE);
	}


	return TRUE;
}


// is (iX, iY) in Resize Button?
BOOL kIsInResizeButton(QWORD qwWindowID, int iX, int iY)
{
	WINDOW* pstWindow;
	

	// search Window
	pstWindow = kGetWindow(qwWindowID);

	if( (pstWindow == NULL) ||
		((pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE) == 0) ||
		((pstWindow->dwFlags & WINDOW_FLAGS_RESIZABLE) == 0) )
	{
		return FALSE;
	}


	// is (iX, iY) in Resize Button?
	if( ((pstWindow->stArea.iX2 - (WINDOW_XBUTTON_SIZE * 2) - 2) <= iX) &&
		(iX <= (pstWindow->stArea.iX2 - (WINDOW_XBUTTON_SIZE * 1) - 2)) &&
		((pstWindow->stArea.iY1 + 1) <= iY) &&
		(iY <= (pstWindow->stArea.iY1 + 1 + WINDOW_XBUTTON_SIZE)) )
	{
		return TRUE;
	}


	return FALSE;
}



// ==========================================================
// for converting coordinate (Screen coord <-> Window coord)
// ==========================================================

BOOL kGetWindowArea(QWORD qwWindowID, RECT* pstArea)
{
	WINDOW* pstWindow;

	
	////////////////////////////////////////////
	// find Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}


	// copy stArea in WINDOW
	kMemCpy(pstArea, &(pstWindow->stArea), sizeof(RECT));


	////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(pstWindow->stLock));
	////////////////////////////////////////////


	return TRUE;
}


// Convert Screen-based (X, Y) to Window-based
BOOL kConvertPointScreenToClient(QWORD qwWindowID, const POINT* pstXY, POINT* pstXYInWindow)
{
	RECT stArea;

	
	// get Window Area
	if(kGetWindowArea(qwWindowID, &stArea) == FALSE)
	{
		return FALSE;
	}


	pstXYInWindow->iX = pstXY->iX - stArea.iX1;		// Screen-based iX - Starting Point iX
	pstXYInWindow->iY = pstXY->iY - stArea.iY1;		// Screen-based iY - Starting Point iY

	
	return TRUE;
}


// Convert Window-based (X, Y) to Screen-based
BOOL kConvertPointClientToScreen(QWORD qwWindowID, const POINT* pstXY, POINT* pstXYInScreen)
{
	RECT stArea;

	
	// get Window Area
	if(kGetWindowArea(qwWindowID, &stArea) == FALSE)
	{
		return FALSE;
	}

	
	pstXYInScreen->iX = pstXY->iX + stArea.iX1;		// Window-based iX + Starting Point iX
	pstXYInScreen->iY = pstXY->iY + stArea.iY1;		// Window-based iY + Starting Point iY

	
	return TRUE;
}


// Convert Screen-based RECT to Window-based
BOOL kConvertRectScreenToClient(QWORD qwWindowID, const RECT* pstArea, RECT* pstAreaInWindow)
{
	RECT stWindowArea;


	// get Window Area
	if(kGetWindowArea(qwWindowID, &stWindowArea) == FALSE)
	{
		return FALSE;
	}

	
	pstAreaInWindow->iX1 = pstArea->iX1 - stWindowArea.iX1;		// Screen-based iX1 - Starting Point iX1;
	pstAreaInWindow->iY1 = pstArea->iY1 - stWindowArea.iY1;		// Screen-based iY1 - Starting Point iY1;
	pstAreaInWindow->iX2 = pstArea->iX2 - stWindowArea.iX1;		// Screen-based iX2 - Starting Point iX1;
	pstAreaInWindow->iY2 = pstArea->iY2 - stWindowArea.iY1;		// Screen-based iY2 - Starting Point iY1;


	return TRUE;
}


// Convert Window-based RECT to Screen-based
BOOL kConvertRectClientToScreen(QWORD qwWindowID, const RECT* pstArea, RECT* pstAreaInScreen)
{
	RECT stWindowArea;


	// get Window Area
	if(kGetWindowArea(qwWindowID, &stWindowArea) == FALSE)
	{
		return FALSE;
	}

	
	pstAreaInScreen->iX1 = pstArea->iX1 + stWindowArea.iX1;
	pstAreaInScreen->iY1 = pstArea->iY1 + stWindowArea.iY1;
	pstAreaInScreen->iX2 = pstArea->iX2 + stWindowArea.iX1;
	pstAreaInScreen->iY2 = pstArea->iY2 + stWindowArea.iY1;


	return TRUE;
}



// ==========================================================
// for updating screen (used by Task)
// ==========================================================


// Update Window to Screen
BOOL kUpdateScreenByID(QWORD qwWindowID)
{
	EVENT stEvent;
	WINDOW* pstWindow;


	// find Window
	pstWindow = kGetWindow(qwWindowID);

	if( (pstWindow == NULL) || ((pstWindow->dwFlags & WINDOW_FLAGS_SHOW) == 0) )
	{
		return FALSE;
	}


	// fill EVENT structure to send, save Window ID
	stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYID;
	stEvent.stWindowEvent.qwWindowID = qwWindowID;


	return kSendEventToWindowManager(&stEvent);
}


// Update some part of Window to Screen
BOOL kUpdateScreenByWindowArea(QWORD qwWindowID, const RECT* pstArea)
{
	EVENT stEvent;
	WINDOW* pstWindow;


	// find Window
	pstWindow = kGetWindow(qwWindowID);

	if( (pstWindow == NULL) || ((pstWindow->dwFlags & WINDOW_FLAGS_SHOW) == 0) )
	{
		return FALSE;
	}


	// fill EVENT structure to send, save Window ID, internal area of Window
	stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA;
	stEvent.stWindowEvent.qwWindowID = qwWindowID;
	kMemCpy(&(stEvent.stWindowEvent.stArea), pstArea, sizeof(RECT));


	return kSendEventToWindowManager(&stEvent);
}


BOOL kUpdateScreenByScreenArea(const RECT* pstArea)
{
	EVENT stEvent;


	// fill EVENT structure to send, save Window ID, Screen Area
	stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA;
	stEvent.stWindowEvent.qwWindowID = WINDOW_INVALIDID;
	kMemCpy(&(stEvent.stWindowEvent.stArea), pstArea, sizeof(RECT));


	return kSendEventToWindowManager(&stEvent);
}



// ==========================================================
// for Event Queue
// ==========================================================

BOOL kSendEventToWindow(QWORD qwWindowID, const EVENT* pstEvent)
{
	WINDOW* pstWindow;
	BOOL bResult;


	///////////////////////////////////////////
	// find Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	///////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}

	
	bResult = kPutQueue(&(pstWindow->stEventQueue), pstEvent);


	///////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(pstWindow->stLock));
	///////////////////////////////////////////
	

	return bResult;
}


BOOL kReceiveEventFromWindowQueue(QWORD qwWindowID, EVENT* pstEvent)
{
	WINDOW* pstWindow;
	BOOL bResult;

	
	///////////////////////////////////////////
	// find Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	///////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}


	bResult = kGetQueue(&(pstWindow->stEventQueue), pstEvent);

	
	///////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(pstWindow->stLock));
	///////////////////////////////////////////
	

	return bResult;
}


BOOL kSendEventToWindowManager(const EVENT* pstEvent)
{
	BOOL bResult = FALSE;

	
	if(kIsQueueFull(&(gs_stWindowManager.stEventQueue)) == FALSE)
	{
		///////////////////////////////////////////
		// CRITICAL SECTION START
		kLock(&(gs_stWindowManager.stLock));
		///////////////////////////////////////////

		bResult = kPutQueue(&(gs_stWindowManager.stEventQueue), pstEvent);

		///////////////////////////////////////////
		// CRITICAL SECTION END
		kUnlock(&(gs_stWindowManager.stLock));
		///////////////////////////////////////////
	}


	return bResult;
}


BOOL kReceiveEventFromWindowManagerQueue(EVENT* pstEvent)
{
	BOOL bResult = FALSE;
	

	if(kIsQueueEmpty(&(gs_stWindowManager.stEventQueue)) == FALSE)
	{	
		///////////////////////////////////////////
		// CRITICAL SECTION START
		kLock(&(gs_stWindowManager.stLock));
		///////////////////////////////////////////

		bResult = kGetQueue(&(gs_stWindowManager.stEventQueue), pstEvent);
		
		///////////////////////////////////////////
		// CRITICAL SECTION END
		kUnlock(&(gs_stWindowManager.stLock));
		///////////////////////////////////////////
	}


	return bResult;
}


BOOL kSetMouseEvent(QWORD qwWindowID, QWORD qwEventType, int iMouseX, int iMouseY, BYTE bButtonStatus, EVENT* pstEvent)
{
	POINT stMouseXYInWindow;
	POINT stMouseXY;

	
	switch(qwEventType)
	{
		case EVENT_MOUSE_MOVE:
		case EVENT_MOUSE_LBUTTONDOWN:
		case EVENT_MOUSE_LBUTTONUP:
		case EVENT_MOUSE_RBUTTONDOWN:
		case EVENT_MOUSE_RBUTTONUP:
		case EVENT_MOUSE_MBUTTONDOWN:
		case EVENT_MOUSE_MBUTTONUP:
			
			// set Mouse (X, Y) coordinate
			stMouseXY.iX = iMouseX;
			stMouseXY.iY = iMouseY;

			
			// convert Mouse (X,Y) to Window-based
			if(kConvertPointScreenToClient(qwWindowID, &stMouseXY, &stMouseXYInWindow) == FALSE)
			{
				return FALSE;
			}


			// set Event Type
			pstEvent->qwType = qwEventType;

			// set Window ID
			pstEvent->stMouseEvent.qwWindowID = qwWindowID;
			
			// set Mouse Button Status
			pstEvent->stMouseEvent.bButtonStatus = bButtonStatus;
			
			// set converted value (Screen-based -> Window-based)
			kMemCpy(&(pstEvent->stMouseEvent.stPoint), &stMouseXYInWindow, sizeof(POINT));


			break;

		default:
			
			return FALSE;
			break;
	}

	return TRUE;
}


BOOL kSetWindowEvent(QWORD qwWindowID, QWORD qwEventType, EVENT* pstEvent)
{
	RECT stArea;


	switch(qwEventType)
	{
		case EVENT_WINDOW_SELECT:
		case EVENT_WINDOW_DESELECT:
		case EVENT_WINDOW_MOVE:
		case EVENT_WINDOW_RESIZE:
		case EVENT_WINDOW_CLOSE:

			// set Event Type
			pstEvent->qwType = qwEventType;

			// set Window ID
			pstEvent->stWindowEvent.qwWindowID = qwWindowID;

			// get Window Area
			if(kGetWindowArea(qwWindowID, &stArea) == FALSE)
			{
				return FALSE;
			}

			// set current Window coordinate
			kMemCpy(&(pstEvent->stWindowEvent.stArea), &stArea, sizeof(RECT));


			break;

		default:

			return FALSE;
			break;
	}

	return TRUE;
}


void kSetKeyEvent(QWORD qwWindow, const KEYDATA* pstKeyData, EVENT* pstEvent)
{
	if(pstKeyData->bFlags & KEY_FLAGS_DOWN)
	{
		pstEvent->qwType = EVENT_KEY_DOWN;
	}
	else
	{
		pstEvent->qwType = EVENT_KEY_UP;
	}


	// set Key Information
	pstEvent->stKeyEvent.bASCIICode = pstKeyData->bASCIICode;
	pstEvent->stKeyEvent.bScanCode  = pstKeyData->bScanCode;
	pstEvent->stKeyEvent.bFlags 	= pstKeyData->bFlags;
}



// ==========================================================
// Drawing functions for a Window, others for Mouse Cursor
// ==========================================================

BOOL kDrawWindowFrame(QWORD qwWindowID)
{
	WINDOW* pstWindow;
	RECT stArea;
	int iWidth;
	int iHeight;


	/////////////////////////////////////////////////////
	// search Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	/////////////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}


	// get Window width, height
	iWidth = kGetRectangleWidth(&(pstWindow->stArea));
	iHeight = kGetRectangleHeight(&(pstWindow->stArea));


	// clipping Area
	kSetRectangleData(0, 0, iWidth - 1, iHeight - 1, &stArea);


	// draw Window Frame with size 2 pixel
	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer, 0, 0, iWidth - 1, iHeight - 1, WINDOW_COLOR_FRAME, FALSE);
	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer, 1, 1, iWidth - 2, iHeight - 2, WINDOW_COLOR_FRAME, FALSE);


	/////////////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(pstWindow->stLock));
	/////////////////////////////////////////////////////
	

	return TRUE;
}


BOOL kDrawWindowBackground(QWORD qwWindowID)
{
	WINDOW* pstWindow;
	int iWidth;
	int iHeight;
	RECT stArea;
	int iX;
	int iY;


	/////////////////////////////////////////////////////
	// search Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	/////////////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}

	
	// get Window width, height
	iWidth = kGetRectangleWidth(&(pstWindow->stArea));
	iHeight = kGetRectangleHeight(&(pstWindow->stArea));


	// clipping area
	kSetRectangleData(0, 0, iWidth - 1, iHeight - 1, &stArea);


	// if Title Bar exist, draw from below Title Bar
	if(pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE)
	{
		iY = WINDOW_TITLEBAR_HEIGHT;
	}
	else
	{
		iY = 0;
	}


	// if Window Frame drawn, draw except Frame Area
	if(pstWindow->dwFlags & WINDOW_FLAGS_DRAWFRAME)
	{
		iX = 2;
	}
	else
	{
		iX = 0;
	}


	// fill Window Background Color
	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer, iX, iY, iWidth - 1 - iX, iHeight - 1 - iX,
					  WINDOW_COLOR_BACKGROUND, TRUE);

	
	/////////////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(pstWindow->stLock));
	/////////////////////////////////////////////////////
	

	return TRUE;
}


BOOL kDrawWindowTitle(QWORD qwWindowID, const char* pcTitle, BOOL bSelectedTitle)
{
	WINDOW* pstWindow;
	int iWidth;
	int iHeight;
	int iX;
	int iY;
	RECT stArea;
	RECT stButtonArea;
	COLOR stTitleBarColor;


	/////////////////////////////////////////////////////
	// search Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	/////////////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}


	// get Window width, height
	iWidth = kGetRectangleWidth(&(pstWindow->stArea));
	iHeight = kGetRectangleHeight(&(pstWindow->stArea));


	// clipping area
	kSetRectangleData(0, 0, iWidth - 1, iHeight - 1, &stArea);


	// ------------------------------------------------------
	// draw Title Bar
	// ------------------------------------------------------
	
	if(bSelectedTitle == TRUE)
	{
		stTitleBarColor = WINDOW_COLOR_TITLEBARACTIVEBACKGROUND;
	}
	else
	{
		stTitleBarColor = WINDOW_COLOR_TITLEBARINACTIVEBACKGROUND;
	}

	
	// fill Title Bar
	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer, 0, 3, iWidth - 1, WINDOW_TITLEBAR_HEIGHT - 1,
					  stTitleBarColor, TRUE);


	// draw Title Bar Text
	kInternalDrawText(&stArea, pstWindow->pstWindowBuffer, 6, 3, 
					  WINDOW_COLOR_TITLEBARTEXT, stTitleBarColor, pcTitle, kStrLen(pcTitle));


	// draw Title Bar effect with size 2 pixel
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 1, 1, iWidth - 1, 1, WINDOW_COLOR_TITLEBARBRIGHT1);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 1, 2, iWidth - 1, 2, WINDOW_COLOR_TITLEBARBRIGHT2);

	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 1, 2, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT1);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 2, 2, 2, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT2);
	

	// draw a line below Title Bar
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 
					  2, WINDOW_TITLEBAR_HEIGHT - 2, iWidth - 2, WINDOW_TITLEBAR_HEIGHT - 2, 
					  WINDOW_COLOR_TITLEBARUNDERLINE);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
					  2, WINDOW_TITLEBAR_HEIGHT - 1, iWidth - 2, WINDOW_TITLEBAR_HEIGHT - 1,
					  WINDOW_COLOR_TITLEBARUNDERLINE);


	/////////////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(pstWindow->stLock));
	/////////////////////////////////////////////////////
	
	

	// ----------------------------------------------------------
	// draw "Exit" Button
	// ----------------------------------------------------------
	
	// draw "Exit" Button at right top
	stButtonArea.iX1 = iWidth - WINDOW_XBUTTON_SIZE - 1;
	stButtonArea.iY1 = 1;
	stButtonArea.iX2 = iWidth - 2;
	stButtonArea.iY2 = WINDOW_XBUTTON_SIZE - 1;
	kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "", WINDOW_COLOR_BACKGROUND);


	/////////////////////////////////////////////////////
	// search Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	/////////////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}


	// draw "X" with size 3 pixel
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 
					  iWidth - 2 - 18 + 4, 1 + 4, iWidth - 2 - 4, WINDOW_TITLEBAR_HEIGHT - 6, WINDOW_COLOR_XBUTTONLINECOLOR);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 
					  iWidth - 2 - 18 + 5, 1 + 4, iWidth - 2 - 4, WINDOW_TITLEBAR_HEIGHT - 7, WINDOW_COLOR_XBUTTONLINECOLOR);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 
					  iWidth - 2 - 18 + 4, 1 + 5, iWidth - 2 - 5, WINDOW_TITLEBAR_HEIGHT - 6, WINDOW_COLOR_XBUTTONLINECOLOR);
	
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 
					  iWidth - 2 - 18 + 4, 19 - 4, iWidth - 2 - 4, 1 + 4, WINDOW_COLOR_XBUTTONLINECOLOR);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 
					  iWidth - 2 - 18 + 5, 19 - 4, iWidth - 2 - 4, 1 + 5, WINDOW_COLOR_XBUTTONLINECOLOR);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 
					  iWidth - 2 - 18 + 4, 19 - 5, iWidth - 2 - 5, 1 + 4, WINDOW_COLOR_XBUTTONLINECOLOR);
	

	/////////////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(pstWindow->stLock));
	/////////////////////////////////////////////////////



	// ----------------------------------------------------------
	// draw "Resize" Button
	// ----------------------------------------------------------
	
	if(pstWindow->dwFlags & WINDOW_FLAGS_RESIZABLE)
	{
		// draw Resize Button at beside to Exit Button
		stButtonArea.iX1 = iWidth - (WINDOW_XBUTTON_SIZE * 2) - 2;
		stButtonArea.iY1 = 1;
		stButtonArea.iX2 = iWidth - WINDOW_XBUTTON_SIZE - 2;
		stButtonArea.iY2 = WINDOW_XBUTTON_SIZE - 1;

		kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "", WINDOW_COLOR_BACKGROUND);

		
		/////////////////////////////////////////////
		// search Window, CRITICAL SECTION START
		pstWindow = kGetWindowWithWindowLock(qwWindowID);
		/////////////////////////////////////////////

		if(pstWindow == NULL)
		{
			return FALSE;
		}


		// draw Line
		kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, stButtonArea.iX1 + 4, stButtonArea.iY2 - 4,
						  stButtonArea.iX2 - 5, stButtonArea.iY1 + 3, WINDOW_COLOR_XBUTTONLINECOLOR);	
		kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, stButtonArea.iX1 + 4, stButtonArea.iY2 - 3,
						  stButtonArea.iX2 - 4, stButtonArea.iY1 + 3, WINDOW_COLOR_XBUTTONLINECOLOR);
		kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, stButtonArea.iX1 + 5, stButtonArea.iY2 - 3,
						  stButtonArea.iX2 - 4, stButtonArea.iY1 + 4, WINDOW_COLOR_XBUTTONLINECOLOR);

		// draw an Arrow at Right Top
		kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, stButtonArea.iX1 + 9, stButtonArea.iY1 + 3,
						  stButtonArea.iX2 - 4, stButtonArea.iY1 + 3, WINDOW_COLOR_XBUTTONLINECOLOR);
		kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, stButtonArea.iX1 + 9, stButtonArea.iY1 + 4,
						  stButtonArea.iX2 - 4, stButtonArea.iY1 + 4, WINDOW_COLOR_XBUTTONLINECOLOR);
		kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, stButtonArea.iX2 - 4, stButtonArea.iY1 + 5,
						  stButtonArea.iX2 - 4, stButtonArea.iY1 + 9, WINDOW_COLOR_XBUTTONLINECOLOR);
		kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, stButtonArea.iX2 - 5, stButtonArea.iY1 + 5,
						  stButtonArea.iX2 - 5, stButtonArea.iY1 + 9, WINDOW_COLOR_XBUTTONLINECOLOR);

		// draw an Arraw at Left Bottom
		kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, stButtonArea.iX1 + 4, stButtonArea.iY1 + 8,
						  stButtonArea.iX1 + 4, stButtonArea.iY2 - 3, WINDOW_COLOR_XBUTTONLINECOLOR);
		kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, stButtonArea.iX1 + 5, stButtonArea.iY1 + 8,
						  stButtonArea.iX1 + 5, stButtonArea.iY2 - 3, WINDOW_COLOR_XBUTTONLINECOLOR);
		kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, stButtonArea.iX1 + 6, stButtonArea.iY2 - 4,
						  stButtonArea.iX1 + 10, stButtonArea.iY2 - 4, WINDOW_COLOR_XBUTTONLINECOLOR);
		kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, stButtonArea.iX1 + 6, stButtonArea.iY2 - 3,
						  stButtonArea.iX1 + 10, stButtonArea.iY2 - 3, WINDOW_COLOR_XBUTTONLINECOLOR);


		/////////////////////////////////////////////////////
		// CRITICAL SECTION END
		kUnlock(&(pstWindow->stLock));
		/////////////////////////////////////////////////////
	}


	return TRUE;
}


BOOL kDrawButton(QWORD qwWindowID, RECT* pstButtonArea, COLOR stBackgroundColor, const char* pcText, COLOR stTextColor)
{
	WINDOW* pstWindow;
	RECT stArea;
	int iWindowWidth;
	int iWindowHeight;
	int iTextLength;
	int iTextWidth;
	int iButtonWidth;
	int iButtonHeight;
	int iTextX;
	int iTextY;

	
	/////////////////////////////////////////////////////
	// search Window, CRICITAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	/////////////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}
	
	
	// get Window width, height
	iWindowWidth = kGetRectangleWidth(&(pstWindow->stArea));
	iWindowHeight = kGetRectangleHeight(&(pstWindow->stArea));


	// clipping area
	kSetRectangleData(0, 0, iWindowWidth - 1, iWindowHeight - 1, &stArea);


	// draw Button Background Color
	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
					  pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX2, pstButtonArea->iY2, 
					  stBackgroundColor, TRUE);


	// get Button width, height and Text width, height
	iButtonWidth = kGetRectangleWidth(pstButtonArea);
	iButtonHeight = kGetRectangleHeight(pstButtonArea);
	iTextLength = kStrLen(pcText);
	iTextWidth = iTextLength * FONT_ENGLISHWIDTH;


	// print Text on the center of the Button
	iTextX = (pstButtonArea->iX1 + iButtonWidth / 2) - iTextWidth / 2;
	iTextY = (pstButtonArea->iY1 + iButtonHeight / 2) - FONT_ENGLISHHEIGHT / 2;
	kInternalDrawText(&stArea, pstWindow->pstWindowBuffer, iTextX, iTextY, stTextColor, stBackgroundColor, pcText, iTextLength);

	
	// draw Button effect, 2 pixel size
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, 
			pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX2, pstButtonArea->iY1, WINDOW_COLOR_BUTTONBRIGHT);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1, pstButtonArea->iY1 + 1, pstButtonArea->iX2 - 1, pstButtonArea->iY1 + 1, WINDOW_COLOR_BUTTONBRIGHT);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX1, pstButtonArea->iY2, WINDOW_COLOR_BUTTONBRIGHT);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1 + 1, pstButtonArea->iY1, pstButtonArea->iX1 + 1, pstButtonArea->iY2 - 1, WINDOW_COLOR_BUTTONBRIGHT);


	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1 + 1, pstButtonArea->iY2, pstButtonArea->iX2, pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1 + 2, pstButtonArea->iY2 - 1, pstButtonArea->iX2, pstButtonArea->iY2 - 1, WINDOW_COLOR_BUTTONDARK);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX2, pstButtonArea->iY1 + 1, pstButtonArea->iX2, pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX2 - 1, pstButtonArea->iY1 + 2, pstButtonArea->iX2 - 1, pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK);


	/////////////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(pstWindow->stLock));
	/////////////////////////////////////////////////////
	

	return TRUE;
}


// Mouse Cursor Image Data
static BYTE gs_vwMouseBuffer[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT] =
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


static void kDrawCursor(int iX, int iY)
{
	int i;
	int j;
	BYTE* pbCurrentPos;


	// Cursor Data Start Pos
	pbCurrentPos= gs_vwMouseBuffer;


	// print Cursor on Screen
	for(j=0; j<MOUSE_CURSOR_HEIGHT; j++)
	{
		for(i=0; i<MOUSE_CURSOR_WIDTH; i++)
		{
			switch(*pbCurrentPos)
			{
				case 0:
					// do nothing
					break;

				case 1:
					// draw Cursor outer line : black
					kInternalDrawPixel(&(gs_stWindowManager.stScreenArea), gs_stWindowManager.pstVideoMemory, 
							i + iX, j + iY, MOUSE_CURSOR_OUTERLINE);

					break;

				case 2:
					// draw outer of Cursor : dark green
					kInternalDrawPixel(&(gs_stWindowManager.stScreenArea), gs_stWindowManager.pstVideoMemory,
							i + iX, j + iY, MOUSE_CURSOR_OUTER);

					break;

				case 3:
					// draw innner of Cursor : bright green
					kInternalDrawPixel(&(gs_stWindowManager.stScreenArea), gs_stWindowManager.pstVideoMemory,
							i + iX , j + iY, MOUSE_CURSOR_INNER);

					break;
			}


			// move to next Cursor Data
			pbCurrentPos++;
		}
	}
}


void kMoveCursor(int iX, int iY)
{
	RECT stPreviousArea;


	// correct Mouse Position : make Mouse not to be out of Screen
	if(iX < gs_stWindowManager.stScreenArea.iX1)
	{
		iX = gs_stWindowManager.stScreenArea.iX1;
	}
	else if(iX > gs_stWindowManager.stScreenArea.iX2)
	{
		iX = gs_stWindowManager.stScreenArea.iX2;
	}

	if(iY < gs_stWindowManager.stScreenArea.iY1)
	{
		iY = gs_stWindowManager.stScreenArea.iY1;
	}
	else if(iY > gs_stWindowManager.stScreenArea.iY2)
	{
		iY = gs_stWindowManager.stScreenArea.iY2;
	}


	/////////////////////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stWindowManager.stLock));
	/////////////////////////////////////////////////////
	

	// save previous Mouse Cursor pos
	stPreviousArea.iX1 = gs_stWindowManager.iMouseX;
	stPreviousArea.iY1 = gs_stWindowManager.iMouseY;
	stPreviousArea.iX2 = gs_stWindowManager.iMouseX + MOUSE_CURSOR_WIDTH - 1;
	stPreviousArea.iY2 = gs_stWindowManager.iMouseY + MOUSE_CURSOR_HEIGHT - 1;


	// save new Mouse Cursor pos
	gs_stWindowManager.iMouseX = iX;
	gs_stWindowManager.iMouseY = iY;


	/////////////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stWindowManager.stLock));
	/////////////////////////////////////////////////////
	
	
	// redraw previous Mouse Area
	kRedrawWindowByArea(&stPreviousArea, WINDOW_INVALIDID);


	// print Mouse Cursor at the new position
	kDrawCursor(iX, iY);
}


void kGetCursorPosition(int* piX, int* piY)
{
	*piX = gs_stWindowManager.iMouseX;
	*piY = gs_stWindowManager.iMouseY;
}


BOOL kDrawPixel(QWORD qwWindowID, int iX, int iY, COLOR stColor)
{
	WINDOW* pstWindow;
	RECT stArea;

	
	/////////////////////////////////////////////////////
	// search Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	/////////////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}

	
	// convert Window start coordinate based on (0, 0)
	kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1, 
					  pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);

	
	// call internal draw function
	kInternalDrawPixel(&stArea, pstWindow->pstWindowBuffer, iX, iY, stColor);


	/////////////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&pstWindow->stLock);
	/////////////////////////////////////////////////////


	return TRUE;
}


BOOL kDrawLine(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor)
{
	WINDOW* pstWindow;
	RECT stArea;


	/////////////////////////////////////////////////////
	// search Window, CRITICALS SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	/////////////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}


	// convert Window start coordinate based on (0, 0)
	kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
					  pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);


	// call internal draw function
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, iX1, iY1, iX2, iY2, stColor);

	
	/////////////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&pstWindow->stLock);
	/////////////////////////////////////////////////////


	return TRUE;
}


BOOL kDrawRect(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor, BOOL bFill)
{
	WINDOW* pstWindow;
	RECT stArea;


	/////////////////////////////////////////////////////
	// search Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	/////////////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}
	

	// convert Window Start coordinate based on (0, 0)
	kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
					  pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);


	// call internal draw function
	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer, iX1, iY1, iX2, iY2, stColor, bFill);


	/////////////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&pstWindow->stLock);
	/////////////////////////////////////////////////////
	

	return TRUE;
}


BOOL kDrawCircle(QWORD qwWindowID, int iX, int iY, int iRadius, COLOR stColor, BOOL bFill)
{
	WINDOW* pstWindow;
	RECT stArea;


	/////////////////////////////////////////////////////
	// search Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	/////////////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}
	

	// convert Window Start coordinate based on (0, 0)
	kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
					  pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);


	// call internal draw function
	kInternalDrawCircle(&stArea, pstWindow->pstWindowBuffer, iX, iY, iRadius, stColor, bFill);


	/////////////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&pstWindow->stLock);
	/////////////////////////////////////////////////////
	

	return TRUE;
}


BOOL kDrawText(QWORD qwWindowID, int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor, 
			   const char* pcString, int iLength)
{
	WINDOW* pstWindow;
	RECT stArea;


	/////////////////////////////////////////////////////
	// search Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	/////////////////////////////////////////////////////
	
	if(pstWindow == NULL)
	{
		return FALSE;
	}
	

	// convert Window Start coordinate based on (0, 0)
	kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
					  pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);


	// call internal draw function
	kInternalDrawText(&stArea, pstWindow->pstWindowBuffer, iX, iY, stTextColor, stBackgroundColor, pcString, iLength);


	/////////////////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&pstWindow->stLock);
	/////////////////////////////////////////////////////
	

	return TRUE;
}


// Bit Block Transfer - Send Buffer contents to Window Buffer at once
// 		(X, Y) based on Window Buffer
BOOL kBitBlt(QWORD qwWindowID, int iX, int iY, COLOR* pstBuffer, int iWidth, int iHeight)
{
	WINDOW* pstWindow;
	RECT stWindowArea;
	RECT stBufferArea;
	RECT stOverlappedArea;
	int iWindowWidth;
	int iOverlappedWidth;
	int iOverlappedHeight;
	int i;
	int j;
	int iWindowPosition;
	int iBufferPosition;
	int iStartX;
	int iStartY;


	///////////////////////////////////////////
	// get Window, CRITICAL SECTION START
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	///////////////////////////////////////////

	if(pstWindow == NULL)
	{
		return FALSE;
	}


	// Window-based coordinate
	kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1, pstWindow->stArea.iY2 - pstWindow->stArea.iY1, 
					  &stWindowArea);


	// Buffer coordinate
	kSetRectangleData(iX, iY, iX + iWidth - 1, iY + iHeight - 1, &stBufferArea);


	// get overlapped area within Window area and Buffer area (set Mask)
	if(kGetOverlappedRectangle(&stWindowArea, &stBufferArea, &stOverlappedArea) == FALSE)
	{
		// CRITICAL SECTION END
		kUnlock(&pstWindow->stLock);

		return FALSE;
	}


	// get width, height of overlapped area and Window area
	iWindowWidth = kGetRectangleWidth(&stWindowArea);

	iOverlappedWidth = kGetRectangleWidth(&stOverlappedArea);
	iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);


	// set start position to print image
	// 		if start position is negative, buffer data (image) clipped as much as negative
	if(iX < 0)
	{
		iStartX = iX;
	}
	else
	{
		iStartX = 0;
	}

	if(iY < 0)
	{
		iStartY = iY;
	}
	else
	{
		iStartY = 0;
	}


	// calculate width, height
	for(j = 0; j < iOverlappedHeight; j++)
	{
		// calcultate start offset of each buffer
		iWindowPosition = (iWindowWidth * (stOverlappedArea.iY1 + j)) + stOverlappedArea.iX1;
		iBufferPosition = (iWidth * j + iStartY) + iStartX;


		// copy one line
		kMemCpy(pstWindow->pstWindowBuffer + iWindowPosition, pstBuffer + iBufferPosition, iOverlappedWidth * sizeof(COLOR));
	}
	
	
	///////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&pstWindow->stLock);
	///////////////////////////////////////////
	

	return TRUE;
}


// Buffer, Buffer size of Wallpaper Image File
extern unsigned char g_vbWallPaper[0];
extern unsigned int  size_g_vbWallPaper;


// draw Wallpaper to Background Window
void kDrawBackgroundImage(void)
{
	JPEG* pstJpeg;
	COLOR* pstOutputBuffer;
	WINDOWMANAGER* pstWindowManager;
	int i;
	int j;
	int iMiddleX;
	int iMiddleY;
	int iScreenWidth;
	int iScreenHeight;


	// get Window Manager
	pstWindowManager = kGetWindowManager();


	// allocate memory for JPEG Manager
	pstJpeg = (JPEG*)kAllocateMemory(sizeof(JPEG));


	// initialize JPEG
	if(kJPEGInit(pstJpeg, g_vbWallPaper, size_g_vbWallPaper) == FALSE)
	{
		return;
	}


	// allocate memory for decoded Image
	pstOutputBuffer = (COLOR*)kAllocateMemory(pstJpeg->width * pstJpeg->height * sizeof(COLOR));

	if(pstOutputBuffer == NULL)
	{
		kFreeMemory(pstJpeg);

		return;
	}


	// process decoding
	if(kJPEGDecode(pstJpeg, pstOutputBuffer) == FALSE)
	{
		kFreeMemory(pstOutputBuffer);
		kFreeMemory(pstJpeg);

		return;
	}


	// print decoded Image to the center of Background Window
	iScreenWidth = kGetRectangleWidth(&(pstWindowManager->stScreenArea));
	iScreenHeight = kGetRectangleHeight(&(pstWindowManager->stScreenArea));

	iMiddleX = (iScreenWidth - pstJpeg->width) / 2;
	iMiddleY = (iScreenHeight - pstJpeg->height) / 2;


	// copy memory
	kBitBlt(pstWindowManager->qwBackgroundWindowID, iMiddleX, iMiddleY, pstOutputBuffer, pstJpeg->width, pstJpeg->height);


	// return allocated memory
	kFreeMemory(pstOutputBuffer);
	kFreeMemory(pstJpeg);
}



// ==================================================================
// for Bitmap used for Screen Update
// ==================================================================

// create Screen Update Bitmap - Window-based coordinate used
BOOL kCreateDrawBitmap(const RECT* pstArea, DRAWBITMAP* pstDrawBitmap)
{
	// if no overlapped Area with screen (out of screen), no need to create
	if(kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), pstArea, &(pstDrawBitmap->stArea)) == FALSE)
	{
		return FALSE;
	}

	
	// get and set Screen Update Bitmap Buffer in Window Manager
	pstDrawBitmap->pbBitmap = gs_stWindowManager.pbDrawBitmap;


	return kFillDrawBitmap(pstDrawBitmap, &(pstDrawBitmap->stArea), TRUE);
}


static BOOL kFillDrawBitmap(DRAWBITMAP* pstDrawBitmap, RECT* pstArea, BOOL bFill)
{
	RECT stOverlappedArea;
	int iByteOffset;
	int iBitOffset;
	int iAreaSize;
	int iOverlappedWidth;
	int iOverlappedHeight;
	BYTE bTempBitmap;
	int i;
	int iOffsetX;
	int iOffsetY;
	int iBulkCount;
	int iLastBitOffset;


	// if no overlapped area with updating area, no need to fill data in Bitmap Buffer
	if(kGetOverlappedRectangle(&(pstDrawBitmap->stArea), pstArea, &stOverlappedArea) == FALSE)
	{
		return FALSE;
	}


	// get width, height of overlapped area
	iOverlappedWidth  = kGetRectangleWidth(&stOverlappedArea);
	iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);
	
	
	// ---------------------------------------------------------
	// loop for height of overlapped area
	// ---------------------------------------------------------
	
	for(iOffsetY = 0; iOffsetY < iOverlappedHeight; iOffsetY++)
	{
		// get Start position in Bitmap Buffer
		if(kGetStartPositionInDrawBitmap(pstDrawBitmap, stOverlappedArea.iX1, stOverlappedArea.iY1 + iOffsetY,
										 &iByteOffset, &iBitOffset) == FALSE)
		{
			break;
		}

		
		// ---------------------------------------------------------
		// loop for width of overlapped area
		// ---------------------------------------------------------
		
		for(iOffsetX = 0; iOffsetX < iOverlappedWidth; )
		{
			// is the size hanldled by 8 bit at once? (executed if iOverlappedWidth >= 8)
			if( (iBitOffset == 0x00) && ((iOverlappedWidth - iOffsetX) >= 8) )
			{
				// calculate max size handled by 8 Pixel at once
				iBulkCount = (iOverlappedWidth - iOffsetX) >> 3;


				// handle 8 Pixel at once
				if(bFill == TRUE)
				{
					kMemSet(pstDrawBitmap->pbBitmap + iByteOffset, 0xFF, iBulkCount);
				}
				else
				{
					kMemSet(pstDrawBitmap->pbBitmap + iByteOffset, 0x00, iBulkCount);
				}

				
				// change value as much as set Bitmap
				iOffsetX += iBulkCount << 3;

				
				// change Bitmap Offset
				iByteOffset += iBulkCount;
				iBitOffset = 0;
			}
			else
			{
				// calculate bit offset of last Pixel
				iLastBitOffset = MIN(8, iOverlappedWidth - iOffsetX + iBitOffset);

				
				// create Bitmap
				bTempBitmap = 0;

				for(i = iBitOffset; i < iLastBitOffset; i++)
				{
					bTempBitmap |= (0x01 << i);
				}


				// change value as much as handled by 8 bit
				iOffsetX += (iLastBitOffset - iBitOffset);


				// set Bitmap info as updated
				if(bFill == TRUE)
				{
					pstDrawBitmap->pbBitmap[iByteOffset] |= bTempBitmap;
				}
				else
				{
					pstDrawBitmap->pbBitmap[iByteOffset] &= ~(bTempBitmap);
				}

				iByteOffset++;
				iBitOffset = 0;
			}
		}	// loop for width end
	}	// loop for height end

	return TRUE;
}


// Return Byte Offset, Bit Offset starting from Update Bitmap
// 		- uses Screen-based coordinate (iX, iY)
BOOL kGetStartPositionInDrawBitmap(const DRAWBITMAP* pstDrawBitmap, int iX, int iY, int* piByteOffset, int* piBitOffset)
{
	int iWidth;
	int iOffsetX;
	int iOffsetY;

	
	// if (X1, Y1) not included in Update Bitmap, no need to get Start Position
	if(kIsInRectangle(&(pstDrawBitmap->stArea), iX, iY) == FALSE)
	{
		return FALSE;
	}


	// calculate internal offset of updating area
	iOffsetX = iX - pstDrawBitmap->stArea.iX1;
	iOffsetY = iY - pstDrawBitmap->stArea.iY1;


	// get width of updating area
	iWidth = kGetRectangleWidth(&(pstDrawBitmap->stArea));


	// Byte Offset : divide drawing area by 8
	*piByteOffset = (iOffsetY * iWidth + iOffsetX) >> 3;
	*piBitOffset  = (iOffsetY * iWidth + iOffsetX) & 0x7;


	return TRUE;
}


// return TRUE means there's no pixel to update
BOOL kIsDrawBitmapAllOff(const DRAWBITMAP* pstDrawBitmap)
{
	int iByteCount;
	int iLastBitIndex;
	int iWidth;
	int iHeight;
	int i;
	BYTE* pbTempPosition;
	int iSize;


	// calculate width, height of updating area
	iWidth = kGetRectangleWidth(&(pstDrawBitmap->stArea));
	iHeight = kGetRectangleHeight(&(pstDrawBitmap->stArea));


	// get Byte count of Bitmap
	iSize = iWidth * iHeight;
	iByteCount = iSize >> 3;

	
	// compare 8 byte at once
	pbTempPosition = pstDrawBitmap->pbBitmap;		// start from the begin

	for(i=0; i<(iByteCount >> 3); i++)
	{
		if(*(QWORD*)(pbTempPosition) != 0)
		{
			return FALSE;
		}

		pbTempPosition += 8;
	}

	
	// compare remaining byte
	for(i=0; i<(iByteCount & 0x7); i++)
	{
		if(*pbTempPosition != 0)
		{
			return FALSE;
		}

		pbTempPosition++;
	}

	
	// if Total size(iSize) not completely divided by 8 (if has the rest),
	// check the last byte having surplus bit
	iLastBitIndex = iSize & 0x7;		// iSize % 8

	for(i=0; i<iLastBitIndex; i++)
	{
		if(*pbTempPosition & (0x01 << i) )
		{
			return FALSE;
		}
	}


	return TRUE;
}




