#include "ApplicationPanelTask.h"
#include "RTC.h"
#include "Task.h"
#include "GUITask.h"



// Application Table
APPLICATIONENTRY gs_vstApplicationTable[] =
{
	{"Base GUI Task"			, kBaseGUITask			},
	{"Hello World GUI Task"		, kHelloWorldGUITask	},
	{"System Monitor Task"		, kSystemMonitorTask	},
	{"Console Shell for GUI"	, kGUIConsoleShellTask	},
	{"Image Viewer Task"		, kImageViewerTask		},
};


// for Application Panel Data
APPLICATIONPANELDATA gs_stApplicationPanelData;



// Application Panel Task
void kApplicationPanelGUITask(void)
{
	EVENT stReceivedEvent;
	BOOL bApplicationPanelEventResult;
	BOOL bApplicationListEventResult;

	
	// is Graphic Mode now?
	if(kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI Mode\n");

		return;
	}


	// ------------------------------------------------------------------------
	// Create Window - Application Panel Window, Application List Window
	// ------------------------------------------------------------------------
	
	if( (kCreateApplicationPanelWindow() == FALSE) || (kCreateApplicationListWindow() == FALSE) )
	{
		return;
	}


	// ------------------------------------------------------------------------
	// GUI Task Event Handling Loop
	// ------------------------------------------------------------------------

	while(1)
	{
		// handling Window Event
		bApplicationPanelEventResult = kProcessApplicationPanelWindowEvent();
		bApplicationListEventResult  = kProcessApplicationListWindowEvent();


		if( (bApplicationPanelEventResult == FALSE) && (bApplicationListEventResult == FALSE) )
		{
			kSleep(0);
		}
	}
}



static BOOL kCreateApplicationPanelWindow(void)
{
	WINDOWMANAGER* pstWindowManager;
	QWORD qwWindowID;

	
	// get Window Manager
	pstWindowManager = kGetWindowManager();


	// create a Application Panel Window at the top of screen
	qwWindowID = kCreateWindow(0, 0, pstWindowManager->stScreenArea.iX2 + 1, APPLICATIONPANEL_HEIGHT, NULL, APPLICATIONPANEL_TITLE);

	if(qwWindowID == WINDOW_INVALIDID)
	{
		return FALSE;
	}


	// draw Application Panel
	kDrawRect(qwWindowID, 0, 0, pstWindowManager->stScreenArea.iX2	  , APPLICATIONPANEL_HEIGHT - 1,
			  APPLICATIONPANEL_COLOR_OUTERLINE , FALSE);
	kDrawRect(qwWindowID, 1, 1, pstWindowManager->stScreenArea.iX2 - 1, APPLICATIONPANEL_HEIGHT - 2,
			  APPLICATIONPANEL_COLOR_MIDDLELINE, FALSE);
	kDrawRect(qwWindowID, 2, 2, pstWindowManager->stScreenArea.iX2 - 2, APPLICATIONPANEL_HEIGHT - 3,
			  APPLICATIONPANEL_COLOR_INNERLINE , FALSE);
	kDrawRect(qwWindowID, 3, 3, pstWindowManager->stScreenArea.iX2 - 3, APPLICATIONPANEL_HEIGHT - 4,
			  APPLICATIONPANEL_COLOR_BACKGROUND, TRUE);


	// draw Application Button at the left of Application Panel
	kSetRectangleData(5, 5, 120, 25, &(gs_stApplicationPanelData.stButtonArea));

	kDrawButton(qwWindowID, &(gs_stApplicationPanelData.stButtonArea), APPLICATIONPANEL_COLOR_ACTIVE, 
				"Application", RGB(255, 255, 255));


	// draw Digital Clock at the right of Application Panel
	kDrawDigitalClock(qwWindowID);


	// show Application Panel
	kShowWindow(qwWindowID, TRUE);


	// store Window ID to Application Panel Data Structure
	gs_stApplicationPanelData.qwApplicationPanelID = qwWindowID;


	return TRUE;
}


static void kDrawDigitalClock(QWORD qwApplicationPanelID)
{
	RECT stWindowArea;
	RECT stUpdateArea;
	static BYTE s_bPreviousHour, s_bPreviousMinute, s_bPreviousSecond;
	BYTE bHour, bMinute, bSecond;
	char vcBuffer[10] = "00:00 AM";


	// get current time from RTC (Real Time Clock)
	kReadRTCTime(&bHour, &bMinute, &bSecond);

	
	// if no change with previous one, no need to draw
	if( (s_bPreviousHour == bHour) && (s_bPreviousMinute == bMinute) && (s_bPreviousSecond == bSecond) )
	{
		return;
	}
	
	
	// update Hour, Minute, Second for next comparison 
	s_bPreviousHour   = bHour;
	s_bPreviousMinute = bMinute;
	s_bPreviousSecond = bSecond;


	// if bHour is over 12, change to PM
	if(bHour >= 12)
	{
		if(bHour > 12)
		{
			bHour -= 12;
		}

		vcBuffer[6] = 'P';
	}


	// set Hour
	vcBuffer[0] = '0' + bHour / 10;
	vcBuffer[1] = '0' + bHour % 10;

	// set Minute
	vcBuffer[3] = '0' + bMinute / 10;
	vcBuffer[4] = '0' + bMinute % 10;

	// blink ':'
	if( (bSecond % 2) == 1 )
	{
		vcBuffer[2] = ' ';
	}
	else
	{
		vcBuffer[2] = ':';
	}


	// get Application Panel Window location
	kGetWindowArea(qwApplicationPanelID, &stWindowArea);


	// draw Clock Area Line
	kSetRectangleData(stWindowArea.iX2 - APPLICATIONPANEL_CLOCKWIDTH - 13, 5, stWindowArea.iX2 - 5, 25, &stUpdateArea);

	kDrawRect(qwApplicationPanelID, stUpdateArea.iX1, stUpdateArea.iY1, stUpdateArea.iX2, stUpdateArea.iY2,
			  APPLICATIONPANEL_COLOR_INNERLINE, FALSE);


	// draw Clock
	kDrawText(qwApplicationPanelID, stUpdateArea.iX1 + 4, stUpdateArea.iY1 + 3, RGB(255, 255, 255),
			  APPLICATIONPANEL_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));


	// update Clock Area
	kUpdateScreenByWindowArea(qwApplicationPanelID, &stUpdateArea);	
}


static BOOL kProcessApplicationPanelWindowEvent(void)
{
	EVENT stReceivedEvent;
	MOUSEEVENT* pstMouseEvent;
	BOOL bProcessResult;
	QWORD qwApplicationPanelID;
	QWORD qwApplicationListID;

	
	// store Window ID
	qwApplicationPanelID = gs_stApplicationPanelData.qwApplicationPanelID;
	qwApplicationListID  = gs_stApplicationPanelData.qwApplicationListID;
	bProcessResult = FALSE;


	// Event handling loop
	while(1)
	{
		// draw Clock at the right of Application Panel Window
		kDrawDigitalClock(gs_stApplicationPanelData.qwApplicationPanelID);

		
		// receive Event from Event Queue
		if(kReceiveEventFromWindowQueue(qwApplicationPanelID, &stReceivedEvent) == FALSE)
		{
			break;
		}
		
		bProcessResult = TRUE;


		// handle received Event by Event Type
		switch(stReceivedEvent.qwType)
		{
			// handle LBUTTONDOWN
			case EVENT_MOUSE_LBUTTONDOWN:
				
				pstMouseEvent = &(stReceivedEvent.stMouseEvent);


				// if LBUTTONDOWN at Application Panel Button Area, show Application List Window
				if(kIsInRectangle(&(gs_stApplicationPanelData.stButtonArea), 
								  pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY) == FALSE)
				{
					break;
				}


				// if LBUTTONDOWN when Visible option disabled (Button not pushed)
				if(gs_stApplicationPanelData.bApplicationWindowVisible == FALSE)
				{
					// draw Button as pushed
					kDrawButton(qwApplicationPanelID, &(gs_stApplicationPanelData.stButtonArea),
								APPLICATIONPANEL_COLOR_BACKGROUND, "Application", RGB(255, 255, 255));

					
					// update Button Area
					kUpdateScreenByWindowArea(qwApplicationPanelID, &(gs_stApplicationPanelData.stButtonArea));


					// init Application List Window
					// 		1. as no item selected
					// 		2. make it to highest Window
					if(gs_stApplicationPanelData.iPreviousMouseOverIndex != -1)
					{
						kDrawApplicationListItem(gs_stApplicationPanelData.iPreviousMouseOverIndex, FALSE);

						gs_stApplicationPanelData.iPreviousMouseOverIndex = -1;
					}


					kMoveWindowToTop(gs_stApplicationPanelData.qwApplicationListID);
					kShowWindow(gs_stApplicationPanelData.qwApplicationListID, TRUE);


					// set Visible Flag on
					gs_stApplicationPanelData.bApplicationWindowVisible = TRUE;
				}
				// if LBUTTONDOWN when Visible option enabled (Button pushed)
				else
				{
					// draw Button as not pushed
					kDrawButton(qwApplicationPanelID, &(gs_stApplicationPanelData.stButtonArea),
								APPLICATIONPANEL_COLOR_ACTIVE, "Application", RGB(255, 255, 255));
					
					
					// update Button Area
					kUpdateScreenByWindowArea(qwApplicationPanelID, &(gs_stApplicationPanelData.stButtonArea));


					// hide Application List Window
					kShowWindow(qwApplicationListID, FALSE);


					// set Visible Flag off
					gs_stApplicationPanelData.bApplicationWindowVisible = FALSE;
				}

				break;


			default:

				break;
		}
	}	// Event Handling loop end


	return bProcessResult;
}


static BOOL kCreateApplicationListWindow(void)
{
	int i;
	int iCount;
	int iMaxNameLength;
	int iNameLength;
	QWORD qwWindowID;
	int iX;
	int iY;
	int iWindowWidth;


	// get the longest name
	iMaxNameLength = 0;
	iCount = sizeof(gs_vstApplicationTable) / sizeof(APPLICATIONENTRY);


	for(i=0; i<iCount; i++)
	{
		iNameLength = kStrLen(gs_vstApplicationTable[i].pcApplicationName);

		if(iMaxNameLength < iNameLength)
		{
			iMaxNameLength = iNameLength;
		}
	}


	// calculate width, free space 20 pixel
	iWindowWidth = iMaxNameLength * FONT_ENGLISHWIDTH + 20;


	// Window location is at below of Application Panel Button
	iX = gs_stApplicationPanelData.stButtonArea.iX1;
	iY = gs_stApplicationPanelData.stButtonArea.iY2 + 5;


	// create Application List Window, Title bar not drawn
	qwWindowID = kCreateWindow(iX, iY, iWindowWidth, iCount * APPLICATIONPANEL_LISTITEMHEIGHT + 1, NULL, APPLICATIONPANEL_LISTTITLE);

	if(qwWindowID == WINDOW_INVALIDID)
	{
		return FALSE;
	}


	// store List Window Width to Application Panal Data Structure
	gs_stApplicationPanelData.iApplicationListWidth = iWindowWidth;


	// hide Application List at the start
	gs_stApplicationPanelData.bApplicationWindowVisible = FALSE;


	// store List Window ID to Application Panel Data Structure, no item previously mouse over
	gs_stApplicationPanelData.qwApplicationListID = qwWindowID;
	gs_stApplicationPanelData.iPreviousMouseOverIndex = -1;


	// draw Program Name, limit
	for(i=0; i<iCount; i++)
	{
		kDrawApplicationListItem(i, FALSE);
	}


	kMoveWindow(qwWindowID, gs_stApplicationPanelData.stButtonArea.iX1, gs_stApplicationPanelData.stButtonArea.iY2 + 5);


	return TRUE;
}


static void kDrawApplicationListItem(int iIndex, BOOL bSelected)
{
	QWORD qwWindowID;
	int iWindowWidth;
	COLOR stColor;
	RECT stItemArea;


	// ID, width of Application List Window
	qwWindowID = gs_stApplicationPanelData.qwApplicationListID;
	iWindowWidth = gs_stApplicationPanelData.iApplicationListWidth;


	if(bSelected == TRUE)
	{
		stColor = APPLICATIONPANEL_COLOR_ACTIVE;
	}
	else
	{
		stColor = APPLICATIONPANEL_COLOR_BACKGROUND;
	}

	
	// draw Item line
	kSetRectangleData(0, iIndex * APPLICATIONPANEL_LISTITEMHEIGHT, 
					  iWindowWidth - 1, (iIndex + 1) * APPLICATIONPANEL_LISTITEMHEIGHT, &stItemArea);
	kDrawRect(qwWindowID, stItemArea.iX1, stItemArea.iY1, stItemArea.iX2, stItemArea.iY2, 
			  APPLICATIONPANEL_COLOR_INNERLINE, FALSE);


	// fill internal of Item
	kDrawRect(qwWindowID, stItemArea.iX1 + 1, stItemArea.iY1 + 1, stItemArea.iX2 - 1, stItemArea.iY2 - 1,
			  stColor, TRUE);


	// draw GUI Task Name
	kDrawText(qwWindowID, stItemArea.iX1 + 10, stItemArea.iY1 + 2, RGB(255, 255, 255), stColor,
			  gs_vstApplicationTable[iIndex].pcApplicationName, kStrLen(gs_vstApplicationTable[iIndex].pcApplicationName));


	// update screen
	kUpdateScreenByWindowArea(qwWindowID, &stItemArea);
}


static BOOL kProcessApplicationListWindowEvent(void)
{
	EVENT stReceivedEvent;
	MOUSEEVENT* pstMouseEvent;
	BOOL bProcessResult;
	QWORD qwApplicationPanelID;
	QWORD qwApplicationListID;
	int iMouseOverIndex;
	EVENT stEvent;


	// store Window ID
	qwApplicationPanelID = gs_stApplicationPanelData.qwApplicationPanelID;
	qwApplicationListID  = gs_stApplicationPanelData.qwApplicationListID;
	bProcessResult = FALSE;

	
	// Event handling loop
	while(1)
	{
		// receive Event from Event Queue
		if(kReceiveEventFromWindowQueue(qwApplicationListID, &stReceivedEvent) == FALSE)
		{
			break;
		}

		bProcessResult = TRUE;


		// handle Event by Event Type
		switch(stReceivedEvent.qwType)
		{
			// handle Mouse Move Event
			case EVENT_MOUSE_MOVE:
				
				pstMouseEvent = &(stReceivedEvent.stMouseEvent);


				// get Item Index under the Mouse
				iMouseOverIndex = kGetMouseOverItemIndex(pstMouseEvent->stPoint.iY);


				// execute code only (previous Mouse over Index != current Mouse over Index)
				if( (iMouseOverIndex == gs_stApplicationPanelData.iPreviousMouseOverIndex) ||
					(iMouseOverIndex == -1) )
				{
					break;
				}


				// redraw previous Mouse over Item area as default
				if(gs_stApplicationPanelData.iPreviousMouseOverIndex != -1)
				{
					kDrawApplicationListItem(gs_stApplicationPanelData.iPreviousMouseOverIndex, FALSE);
				}


				// redraw current Mouse over Item area as selected
				kDrawApplicationListItem(iMouseOverIndex, TRUE);


				// save currently Mouse over Item index
				gs_stApplicationPanelData.iPreviousMouseOverIndex = iMouseOverIndex;

				break;


			// handle Mouse LButton down Event
			case EVENT_MOUSE_LBUTTONDOWN:

				pstMouseEvent = &(stReceivedEvent.stMouseEvent);

				
				// get current Mouse over Item index
				iMouseOverIndex = kGetMouseOverItemIndex(pstMouseEvent->stPoint.iY);

				if(iMouseOverIndex == -1)
				{
					break;
				}


				// execute selected Item
				kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, 
							(QWORD)gs_vstApplicationTable[iMouseOverIndex].pvEntryPoint, TASK_LOADBALANCINGID);


				// send LBUTTONDOWN Event to Application Panel
				kSetMouseEvent(qwApplicationPanelID, EVENT_MOUSE_LBUTTONDOWN, 
							   gs_stApplicationPanelData.stButtonArea.iX1 + 1, gs_stApplicationPanelData.stButtonArea.iY1 + 1,
							   NULL, &stEvent);
				kSendEventToWindow(qwApplicationPanelID, &stEvent);

				break;


			default:

				break;
		}
	}

	return bProcessResult;
}


static int kGetMouseOverItemIndex(int iMouseY)
{
	int iCount;
	int iItemIndex;


	// get total Item count of Application List
	iCount = sizeof(gs_vstApplicationTable) / sizeof(APPLICATIONENTRY);


	// calculate Item Index by Mouse Y coord
	iItemIndex = iMouseY / APPLICATIONPANEL_LISTITEMHEIGHT;


	// if out of range, return -1
	if( (iItemIndex < 0) || (iItemIndex >= iCount) )
	{
		return -1;
	}


	return iItemIndex;
}



