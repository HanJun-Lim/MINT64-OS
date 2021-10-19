#include "MINTOSLibrary.h"


// define User-defined event type
#define EVENT_USER_TESTMESSAGE			0x80000001


// Entry Point of Application Program
// 		named as "Main" to distinguish with original main() - int Main(char *pcArgument), int main(int argc, char* argv[])
int Main(char* pcArgument)
{
	QWORD qwWindowID;
	int iMouseX, iMouseY;
	int iWindowWidth, iWindowHeight;
	EVENT stReceivedEvent;
	MOUSEEVENT* pstMouseEvent;
	KEYEVENT* pstKeyEvent;
	WINDOWEVENT* pstWindowEvent;
	int iY;
	char vcTempBuffer[1024];
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
	if(IsGraphicMode() == FALSE)
	{
		printf("This task can run only GUI Mode\n");

		return 0;
	}


	// ========== create Window ==========
	
	GetCursorPosition(&iMouseX, &iMouseY);

	
	// set Window size, Window title
	iWindowWidth = 500;
	iWindowHeight = 200;


	// call CreateWindow(), Window location based on Mouse Cursor 
	// 		give each Window unique title
	sprintf(vcTempBuffer, "Hello World Window %d", s_iWindowCount++);

	qwWindowID = CreateWindow(iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2, 
							   iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT, vcTempBuffer);

	if(qwWindowID == WINDOW_INVALIDID)
	{
		return 0;
	}


	// ========== draw the part of printing Window Event (Window Manager -> Window) ==========
	
	// Event Information position to print
	iY = WINDOW_TITLEBAR_HEIGHT + 10;


	// draw line of Event Information area, Window ID
	DrawRect(qwWindowID, 10, iY + 8, iWindowWidth - 10, iY + 70, RGB(0, 0, 0), FALSE);

	sprintf(vcTempBuffer, "GUI Event Information [Window ID: 0x%Q, User Mode:%s]", qwWindowID, pcArgument);
	DrawText(qwWindowID, 20, iY, RGB(0, 0, 0), RGB(255, 255, 255), vcTempBuffer, strlen(vcTempBuffer));

	
	// ========== draw Event Send Button ==========
	
	// set Button Area
	SetRectangleData(10, iY + 80, iWindowWidth - 10, iWindowHeight - 10, &stButtonArea);

	// draw Button
	DrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "User Message Send Button (Up)", RGB(0, 0, 0));
	
	// show Window
	ShowWindow(qwWindowID, TRUE);


	// ========== Event Handling loop ==========
	
	while(1)
	{
		// get received Event in Event Queue
		if(ReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			Sleep(0);

			continue;
		}


		// erase previous Window Event Information by filling area with Background color
		DrawRect(qwWindowID, 11, iY + 20, iWindowWidth - 11, iY + 69, WINDOW_COLOR_BACKGROUND, TRUE);


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
				sprintf(vcTempBuffer, "Mouse Event : %s", vpcEventString[stReceivedEvent.qwType]);
				DrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen(vcTempBuffer));

				
				// print Mouse Data
				sprintf(vcTempBuffer, "Data: X = %d, Y = %d, Button = %X",
						 pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY, pstMouseEvent->bButtonStatus);
				DrawText(qwWindowID, 20, iY + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen(vcTempBuffer));


				// --------------------------------------------------
				// if Mouse Up or Down Event, redraw Button
				// --------------------------------------------------

				if(stReceivedEvent.qwType == EVENT_MOUSE_LBUTTONDOWN)
				{
					// is Button Up or Down Event on the Button?
					if(IsInRectangle(&stButtonArea, pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY) == TRUE)
					{
						// set as "down" by fill Button color with bright green
						DrawButton(qwWindowID, &stButtonArea, RGB(79, 204, 11), 
									"User Message Send Button (Down)", RGB(255, 255, 255));
						
						UpdateScreenByID(qwWindowID);


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
							sprintf(vcTempBuffer, "Hello World Window %d", i);
							qwFindWindowID = FindWindowByTitle(vcTempBuffer);


							if((qwFindWindowID != WINDOW_INVALIDID) && (qwFindWindowID != qwWindowID))
							{
								// send Event to other Window
								SendEventToWindow(qwFindWindowID, &stSendEvent);
							}
						}

					}	// if LButton Down on the Button

				}	// if LButtonDown on Window
				
				else if(stReceivedEvent.qwType == EVENT_MOUSE_LBUTTONUP)
				{
					// set as "up" by fill Button color with white
					DrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "User Message Send Button (Up)",
								RGB(0, 0, 0));
				}

				break;


			// handle Key Event
			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:

				// insert Key Event handling code here
				pstKeyEvent = &(stReceivedEvent.stKeyEvent);


				// print Key Event Type
				sprintf(vcTempBuffer, "Key Event: %s", vpcEventString[stReceivedEvent.qwType]);
				DrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen(vcTempBuffer));


				// print Key Data
				sprintf(vcTempBuffer, "Data: Key = %c, Flag = %X", pstKeyEvent->bASCIICode, pstKeyEvent->bFlags);
				DrawText(qwWindowID, 20, iY + 40, RGB(0, 0 ,0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen(vcTempBuffer));

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
				sprintf(vcTempBuffer, "Window Event : %s", vpcEventString[stReceivedEvent.qwType]);
				DrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen(vcTempBuffer));

				
				// print Window Data
				sprintf(vcTempBuffer, "Data: X1 = %d, Y1 = %d, X2 = %d, Y2 = %d",
						 pstWindowEvent->stArea.iX1, pstWindowEvent->stArea.iY1, 
						 pstWindowEvent->stArea.iX2, pstWindowEvent->stArea.iY2);
				DrawText(qwWindowID, 20, iY + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen(vcTempBuffer));


				// ******************************************
				// if CLOSE Event, delete Window, end Task
				// ******************************************
				
				if(stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
				{
					DeleteWindow(qwWindowID);

					return 0;
				}

				break;


			default:
				
				// insert unknown Event handling code here
				
				// print unknown Event
				sprintf(vcTempBuffer, "Unknown Event: 0x%X", stReceivedEvent.qwType);
				DrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen(vcTempBuffer));

				
				// print Data
				sprintf(vcTempBuffer, "Data 0 = 0x%Q, Data 1 = 0x%Q, Data 2 = 0x%Q",
						 stReceivedEvent.vqwData[0], stReceivedEvent.vqwData[1], stReceivedEvent.vqwData[2]);
				DrawText(qwWindowID, 20, iY + 40, RGB(0, 0 ,0), WINDOW_COLOR_BACKGROUND, vcTempBuffer, strlen(vcTempBuffer));


				break;
		}

		// update Window to screen
		ShowWindow(qwWindowID, TRUE);
	
	}	// infinite loop end
	
	return 0;
}
