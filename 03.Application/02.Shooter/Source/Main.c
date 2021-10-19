#include "MINTOSLibrary.h"
#include "Main.h"


// for game flow
GAMEINFO g_stGameInfo = {0, };


int Main(char* pcArgument)
{
	QWORD qwWindowID;
	EVENT stEvent;
	QWORD qwLastTickCount;
	char* pcStartMessage = "Please LButton Down To Start";
	POINT stMouseXY;
	RECT  stScreenArea;
	int   iX;
	int   iY;



	// --------------------------------------------------
	// make a Window with size 250*350 at the center
	// --------------------------------------------------
	
	GetScreenArea(&stScreenArea);

	iX = (GetRectangleWidth(&stScreenArea) - WINDOW_WIDTH) / 2;
	iY = (GetRectangleHeight(&stScreenArea) - WINDOW_HEIGHT) / 2;

	qwWindowID = CreateWindow(iX, iY, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_FLAGS_DEFAULT, "Bubble Shooter");

	if(qwWindowID == WINDOW_INVALIDID)
	{
		printf("Window create fail\n");
		
		return -1;
	}

	

	// --------------------------------------------------
	// initialize Game info, allocate buffers
	// --------------------------------------------------
	
	// set Mouse X,Y
	stMouseXY.iX = WINDOW_WIDTH / 2;
	stMouseXY.iY = WINDOW_HEIGHT / 2;
	

	// init Game info
	if(Initialize() == FALSE)
	{
		// if initialization failed, delete allocated Window
		DeleteWindow(qwWindowID);

		return -1;
	}

	
	// set random number initial value
	srand(GetTickCount());

	

	// -----------------------------------------------------------
	// draw Game info and Game area, print Game start message
	// -----------------------------------------------------------
	
	DrawInformation(qwWindowID);
	DrawGameArea(qwWindowID, &stMouseXY);
	DrawText(qwWindowID, 5, 150, RGB(255, 255, 255), RGB(0, 0, 0), pcStartMessage, strlen(pcStartMessage));
	
	// print Game start message (update Window)
	ShowWindow(qwWindowID, TRUE);



	// ------------------------------------------
	// handle Event, Game loop
	// ------------------------------------------
	
	qwLastTickCount = GetTickCount();

	while(1)
	{	
		// ------------------------------------------
		// handle received Event
		// ------------------------------------------
		
		if(ReceiveEventFromWindowQueue(qwWindowID, &stEvent) == TRUE)
		{
			// handle by Event type
			switch(stEvent.qwType)
			{
				case EVENT_MOUSE_LBUTTONDOWN:
			
					// if the click to play the game, game start
					if(g_stGameInfo.bGameStart == FALSE)
					{
						// init Game info
						Initialize();
				
						g_stGameInfo.bGameStart = TRUE;

						break;
					}


					// delete the bubble under the click point
					DeleteBubbleUnderMouse(&(stEvent.stMouseEvent.stPoint));


					// store Mouse position
					memcpy(&stMouseXY, &(stEvent.stMouseEvent.stPoint), sizeof(stMouseXY));

					break;

				case EVENT_MOUSE_MOVE:

					// store Mouse position
					memcpy(&stMouseXY, &(stEvent.stMouseEvent.stPoint), sizeof(stMouseXY));

					break;

				case EVENT_WINDOW_CLOSE:
					
					// delete Window, free allocated memory
					DeleteWindow(qwWindowID);
					free(g_stGameInfo.pstBubbleBuffer);

					return 0;

					break;
			}
		}	// Event received endif



		// ------------------------------------------
		// Game loop
		// ------------------------------------------

		// if the Game is started, move the bubble every 50 ms
		if( (g_stGameInfo.bGameStart == TRUE) && ( (GetTickCount() - qwLastTickCount) > 50 ) )
		{
			qwLastTickCount = GetTickCount();

			
			// make bubble
			if( (rand() % 7) == 1 )
			{
				CreateBubble();
			}

			// move bubbles
			MoveBubble();

			// redraw Game area
			DrawGameArea(qwWindowID, &stMouseXY);

			// redraw Game info
			DrawInformation(qwWindowID);


			// if Player life is 0, game over
			if(g_stGameInfo.iLife <= 0)
			{
				g_stGameInfo.bGameStart = FALSE;


				// draw Game over message
				DrawText(qwWindowID, 80, 130, RGB(255, 255, 255), RGB(0, 0, 0), "Game Over", 9);
				DrawText(qwWindowID, 5, 150, RGB(255, 255, 255), RGB(0, 0, 0), pcStartMessage, strlen(pcStartMessage));
			}


			// update re-drawn Window
			ShowWindow(qwWindowID, TRUE);
		}
		else
		{
			Sleep(0);
		}

	}	// while(1) end


	return 0;
}



BOOL Initialize(void)
{
	// allocate memory for the maximum count of the bubble
	if(g_stGameInfo.pstBubbleBuffer == NULL)
	{
		g_stGameInfo.pstBubbleBuffer = malloc(sizeof(BUBBLE) * MAXBUBBLECOUNT);

		if(g_stGameInfo.pstBubbleBuffer == NULL)
		{
			printf("Memory allocate fail\n");

			return FALSE;
		}
	}


	// initialize information of each bubble
	memset(g_stGameInfo.pstBubbleBuffer, 0, sizeof(BUBBLE) * MAXBUBBLECOUNT);

	g_stGameInfo.iAliveBubbleCount = 0;


	// Game Info initialize - Game Start, Score, Life
	g_stGameInfo.bGameStart = FALSE;
	g_stGameInfo.qwScore = 0;
	g_stGameInfo.iLife = MAXLIFE;


	return TRUE;
}


BOOL CreateBubble(void)
{
	BUBBLE* pstTarget;
	int i;


	// the bubble can be created for MAXBUBBLECOUNT
	if(g_stGameInfo.iAliveBubbleCount >= MAXBUBBLECOUNT)
	{
		return FALSE;
	}


	// find empty BUBBLE
	for(i=0; i<MAXBUBBLECOUNT; i++)
	{
		if(g_stGameInfo.pstBubbleBuffer[i].bAlive == FALSE)
		{
			// selected BUBBLE
			pstTarget = &(g_stGameInfo.pstBubbleBuffer[i]);


			// make target BUBBLE as alive, initialize BUBBLE speed
			pstTarget->bAlive = TRUE;
			pstTarget->qwSpeed = (rand() % 8) + DEFAULTSPEED;


			// make (X, Y) of BUBBLE to be located into Game area
			pstTarget->qwX = rand() % (WINDOW_WIDTH - 2 * RADIUS) + RADIUS;
			pstTarget->qwY = INFORMATION_HEIGHT + WINDOW_TITLEBAR_HEIGHT + RADIUS + 1;


			// set the colore of BUBBLE
			pstTarget->stColor = RGB( rand() % 256, rand() % 256, rand() % 256 );


			// increase alive bubble count
			g_stGameInfo.iAliveBubbleCount++;


			return TRUE;
		}
	}


	return FALSE;
}


void MoveBubble(void)
{
	BUBBLE* pstTarget;
	int i;


	// move all alive Bubbles
	for(i=0; i<MAXBUBBLECOUNT; i++)
	{
		// if the Bubble alive..
		if(g_stGameInfo.pstBubbleBuffer[i].bAlive == TRUE)
		{
			pstTarget = &(g_stGameInfo.pstBubbleBuffer[i]);

			
			// add Y coord of Bubble to speed
			pstTarget->qwY += pstTarget->qwSpeed;


			// if the Bubble touches the bottom of Game area, decrease Player Life
			if( (pstTarget->qwY + RADIUS) >= WINDOW_HEIGHT )
			{
				pstTarget->bAlive = FALSE;

				g_stGameInfo.iAliveBubbleCount--;

				if(g_stGameInfo.iLife > 0)
				{
					g_stGameInfo.iLife--;
				}
			}
		}
	}	// alive Bubble loop end

}


void DeleteBubbleUnderMouse(POINT* pstMouseXY)
{
	BUBBLE* pstTarget;
	int i;
	QWORD qwDistance;


	// search all alive Bubble to remove (from right to left)
	for(i = MAXBUBBLECOUNT - 1; i >= 0; i--)
	{
		// if the Bubble alive..
		if(g_stGameInfo.pstBubbleBuffer[i].bAlive == TRUE)
		{
			pstTarget = &(g_stGameInfo.pstBubbleBuffer[i]);

			
			// distance between Mouse(X,Y), Bubble(X,Y) = root((Xmouse - Xbubble)^2 + (Ymouse - Ybubble)^2)
			qwDistance = ( (pstMouseXY->iX - pstTarget->qwX) * (pstMouseXY->iX - pstTarget->qwX) ) +
						 ( (pstMouseXY->iY - pstTarget->qwY) * (pstMouseXY->iY - pstTarget->qwY) );


			// if click position is into the bubble..
			if( qwDistance < (RADIUS * RADIUS) )
			{
				pstTarget->bAlive = FALSE;

				
				// decrase alive Bubble count, increse User score
				g_stGameInfo.iAliveBubbleCount--;
				g_stGameInfo.qwScore++;

				break;
			}
		}
	}	// search all bubble loop end

}


void DrawInformation(QWORD qwWindowID)
{
	char vcBuffer[200];
	int iLength;

	
	// draw Game info area
	DrawRect(qwWindowID, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_WIDTH - 2, WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT,
			 RGB(55, 215, 47), TRUE);


	// store Game info into temporary buffer
	sprintf(vcBuffer, "Life: %d, Score: %d\n", g_stGameInfo.iLife, g_stGameInfo.qwScore);
	iLength = strlen(vcBuffer);


	// draw Game info at the center of Game info area
	DrawText(qwWindowID, (WINDOW_WIDTH - iLength * FONT_ENGLISHWIDTH) / 2, WINDOW_TITLEBAR_HEIGHT + 2,
			 RGB(255, 255, 255), RGB(55, 215, 47), vcBuffer, strlen(vcBuffer));
}


void DrawGameArea(QWORD qwWindowID, POINT* pstMouseXY)
{
	BUBBLE* pstTarget;
	int i;


	// draw Game info area background
	DrawRect(qwWindowID, 0, WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1,
			 RGB(0, 0, 0), TRUE);


	// draw alive Bubbles
	for(i=0; i<MAXBUBBLECOUNT; i++)
	{
		if(g_stGameInfo.pstBubbleBuffer[i].bAlive == TRUE)
		{
			pstTarget = &(g_stGameInfo.pstBubbleBuffer[i]);

			
			// draw one Bubble
			DrawCircle(qwWindowID, pstTarget->qwX, pstTarget->qwY, RADIUS, pstTarget->stColor, TRUE);
			DrawCircle(qwWindowID, pstTarget->qwX, pstTarget->qwY, RADIUS, ~pstTarget->stColor, FALSE);
		}
	}

	
	// draw aim line (+)
	if(pstMouseXY->iY < (WINDOW_TITLEBAR_HEIGHT + RADIUS))
	{
		pstMouseXY->iY = WINDOW_TITLEBAR_HEIGHT + RADIUS;
	}

	DrawLine(qwWindowID, pstMouseXY->iX			, pstMouseXY->iY - RADIUS, 
						 pstMouseXY->iX			, pstMouseXY->iY + RADIUS, RGB(255, 0, 0));
	DrawLine(qwWindowID, pstMouseXY->iX - RADIUS, pstMouseXY->iY		 ,
			 			 pstMouseXY->iX + RADIUS, pstMouseXY->iY		 , RGB(255, 0, 0));


	// draw Game info area frame
	DrawRect(qwWindowID, 0, WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1,
			 RGB(0, 255, 0), FALSE);
}



