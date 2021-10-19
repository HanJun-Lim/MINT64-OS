#include "MINTOSLibrary.h"
#include "Main.h"


GAMEINFO g_stGameInfo = {0, };


int Main(char* pcArgument)
{
	QWORD qwWindowID;
	EVENT stEvent;
	KEYEVENT* pstKeyEvent;
	QWORD qwLastTickCount;
	char* pcStartMessage = "Please LButton Down To Start";
	RECT stScreenArea;
	int iX;
	int iY;
	BYTE bBlockKind;


	// -------------------------------------------------
	// create a Window at the center
	// -------------------------------------------------
	
	GetScreenArea(&stScreenArea);
	iX = (GetRectangleWidth(&stScreenArea) - WINDOW_WIDTH) / 2;
	iY = (GetRectangleHeight(&stScreenArea) - WINDOW_HEIGHT) / 2;
	qwWindowID = CreateWindow(iX, iY, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_FLAGS_DEFAULT, "Hexa");

	if(qwWindowID == WINDOW_INVALIDID)
	{
		printf("Window create fail\n");

		return -1;
	}


	// -------------------------------------------------
	// initialize Game info, allocate memory to use
	// -------------------------------------------------
	
	// init Game info
	Initialize();

	// set initial random value
	srand(GetTickCount());


	// ---------------------------------------------------------------
	// draw Game info area, Game area, and print Game start message
	// ---------------------------------------------------------------

	DrawInformation(qwWindowID);
	DrawGameArea(qwWindowID);
	DrawText(qwWindowID, 7, 200, RGB(255, 255, 255), RGB(0, 0, 0), pcStartMessage, strlen(pcStartMessage));

	// draw updated window to screen
	ShowWindow(qwWindowID, TRUE);


	// -------------------------------------------------
	// Event handling
	// -------------------------------------------------
	qwLastTickCount = GetTickCount();

	while(1)
	{
		if(ReceiveEventFromWindowQueue(qwWindowID, &stEvent) == TRUE)
		{
			switch(stEvent.qwType)
			{
				// handle Mouse LButton down
				case EVENT_MOUSE_LBUTTONDOWN:

					if(g_stGameInfo.bGameStart == FALSE)
					{
						// initialize Game Info
						Initialize();
						
						// set Game Start flag
						g_stGameInfo.bGameStart = TRUE;
						
						break;
					}
					
					break;


				// handle Key down
				case EVENT_KEY_DOWN:

					pstKeyEvent = &(stEvent.stKeyEvent);

					if(g_stGameInfo.bGameStart == FALSE)
					{
						break;
					}


					// handle by ASCII Code
					switch(pstKeyEvent->bASCIICode)
					{
						case KEY_LEFT:

							// move left
							if(IsMovePossible(g_stGameInfo.iBlockX - 1, g_stGameInfo.iBlockY) == TRUE)
							{
								g_stGameInfo.iBlockX -= 1;

								DrawGameArea(qwWindowID);
							}

							break;

						case KEY_RIGHT:

							// move right
							if(IsMovePossible(g_stGameInfo.iBlockX + 1, g_stGameInfo.iBlockY) == TRUE)
							{
								g_stGameInfo.iBlockX += 1;

								DrawGameArea(qwWindowID);
							}

							break;

						case KEY_UP:
							
							// change block order
							bBlockKind = g_stGameInfo.vbBlock[0];

							memcpy(&(g_stGameInfo.vbBlock), &(g_stGameInfo.vbBlock[1]), BLOCKCOUNT - 1);

							g_stGameInfo.vbBlock[BLOCKCOUNT - 1] = bBlockKind;

							break;

						case KEY_DOWN:

							// move down
							if(IsMovePossible(g_stGameInfo.iBlockX, g_stGameInfo.iBlockY + 1) == TRUE)
							{
								g_stGameInfo.iBlockY += 1;
							}

							DrawGameArea(qwWindowID);

							break;

						case ' ':

							// move to the bottom
							while(IsMovePossible(g_stGameInfo.iBlockX, g_stGameInfo.iBlockY + 1) == TRUE)
							{
								g_stGameInfo.iBlockY += 1;
							}

							DrawGameArea(qwWindowID);

							break;
					}


					// draw updated window to screen
					ShowWindow(qwWindowID, TRUE);

					break;


				// handle Window close
				case EVENT_WINDOW_CLOSE:

					DeleteWindow(qwWindowID);

					return 0;

					break;
			}

		}	// event received endif


        //----------------------------------------------------------------------
        // Game loop
        //----------------------------------------------------------------------
        
		// wait for a certain time
        if( (g_stGameInfo.bGameStart == TRUE) && 
				( (GetTickCount() - qwLastTickCount) > (300 - (g_stGameInfo.qwLevel * 10)) ) )
        {
            qwLastTickCount = GetTickCount();

            // move the block down
            if(IsMovePossible(g_stGameInfo.iBlockX, g_stGameInfo.iBlockY + 1) == FALSE)
            {
                // if the block can't be fixed, game over
                if(FreezeBlock(g_stGameInfo.iBlockX, g_stGameInfo.iBlockY) == FALSE)
                {
                    g_stGameInfo.bGameStart = FALSE;

                    // draw game over message
                    DrawText(qwWindowID, 82, 230, RGB(255, 255, 255), RGB(0, 0, 0), "Game Over", 9);
                    DrawText(qwWindowID, 7, 250, RGB(255, 255, 255), RGB(0, 0, 0), pcStartMessage, strlen(pcStartMessage));
                }

                // erase continuous blocks on board
                EraseAllContinuousBlockOnBoard( qwWindowID );

                // create new block
                CreateBlock();
            }

			// the block can be moved
            else
            {
                g_stGameInfo.iBlockY++;

                // redraw game area
                DrawGameArea( qwWindowID );
            }

            // update redrawn area to screen
            ShowWindow( qwWindowID, TRUE );
        }
        else
        {
            Sleep(0);
        }
    }


	return 0;
}



void Initialize(void)
{
	// init all entries of Game Info
	memset(&g_stGameInfo, 0, sizeof(g_stGameInfo));


	// set block color
	g_stGameInfo.vstBlockColor[1] = RGB(230,   0,   0);
	g_stGameInfo.vstBlockColor[2] = RGB(  0, 230,   0);
	g_stGameInfo.vstBlockColor[3] = RGB(230,   0, 230);
	g_stGameInfo.vstBlockColor[4] = RGB(230, 230,   0);
	g_stGameInfo.vstBlockColor[5] = RGB(  0, 230, 230);
	g_stGameInfo.vstEdgeColor[1]  = RGB(150,   0,   0);
	g_stGameInfo.vstEdgeColor[2]  = RGB(  0, 150,   0);
	g_stGameInfo.vstEdgeColor[3]  = RGB(150,   0, 150);
	g_stGameInfo.vstEdgeColor[4]  = RGB(150, 150,   0);
	g_stGameInfo.vstEdgeColor[5]  = RGB(  0, 150, 150);


	// set the position of moving block
	g_stGameInfo.iBlockX = -1;
	//g_stGameInfo.iBlockY = -1;
}


void CreateBlock(void)
{
	int i;


	// the block starts 
	g_stGameInfo.iBlockX = BOARDWIDTH / 2;
	g_stGameInfo.iBlockY = -BLOCKCOUNT;


	// determine the kind of blocks
	for(i=0; i<BLOCKCOUNT; i++)
	{
		g_stGameInfo.vbBlock[i] = (rand() % BLOCKKIND) + 1;
	}
}


BOOL IsMovePossible(int iBlockX, int iBlockY)
{
	// is block (X, Y) in Game area?
	if( (iBlockX < 0) || (iBlockX >= BOARDWIDTH) || ((iBlockY + BLOCKCOUNT) > BOARDHEIGHT) )
	{
		return FALSE;
	}


	// if the target position contains any block, false
	if(g_stGameInfo.vvbBoard[iBlockY + BLOCKCOUNT - 1][iBlockX] != EMPTYBLOCK)
	{
		return FALSE;
	}


	return TRUE;
}


BOOL FreezeBlock(int iBlockX, int iBlockY)
{
	int i;


	// if the position to fix is under 0, it means that current Y position filled with blocks
	if(iBlockY < 0)
	{
		return FALSE;
	}


	// fix the moving block to game board
	for(i=0; i<BLOCKCOUNT; i++)
	{
		g_stGameInfo.vvbBoard[iBlockY + i][iBlockX] = g_stGameInfo.vbBlock[i];
	}

	
	// set X coord of moving block to -1
	g_stGameInfo.iBlockX = -1;


	return TRUE;
}


BOOL MarkContinuousHorizonBlockOnBoard(void)
{
	int iMatchCount;
	BYTE bBlockKind;
	int i;
	int j;
	int k;
	BOOL bMarked;

	
	bMarked = FALSE;


	// search whole game board to find over 3 continuous horizon blocks
	for(j=0; j<BOARDHEIGHT; j++)
	{
		iMatchCount = 0;
		bBlockKind = 0xFF;


		for(i=0; i<BOARDWIDTH; i++)
		{
			// if the first one, save block kind
			if( (iMatchCount == 0) && (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK) )
			{
				bBlockKind = g_stGameInfo.vvbBoard[j][i];
				iMatchCount++;
			}
			else
			{
				// if block kind match, increase match block count
				if(g_stGameInfo.vvbBoard[j][i] == bBlockKind)
				{
					iMatchCount++;

					
					// if the match count is 3, mark the target block as to erase
					if(iMatchCount == BLOCKCOUNT)
					{
						for(k=0; k<iMatchCount; k++)
						{
							g_stGameInfo.vvbEraseBlock[j][i-k] = ERASEBLOCK;
						}

						bMarked = TRUE;
					}

					// if the match count over 3, mark the target block as to erase
					else if(iMatchCount > BLOCKCOUNT)
					{
						g_stGameInfo.vvbEraseBlock[j][i] = ERASEBLOCK;
					}
				}

				// if doesn't match, start with new block
				else
				{
					if(g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK)
					{
						iMatchCount = 1;
						bBlockKind = g_stGameInfo.vvbBoard[j][i];
					}
					else
					{
						iMatchCount = 0;
						bBlockKind = 0xFF;
					}
				}
			}
		}
	}


	return bMarked;
}


BOOL MarkContinuousVerticalBlockOnBoard(void)
{
	int iMatchCount;
	BYTE bBlockKind;
	int i;
	int j;
	int k;
	BOOL bMarked;
	

	bMarked = FALSE;


	// search whole game board to find over 3 continuous vertical blocks
	for(i=0; i<BOARDWIDTH; i++)
	{
		iMatchCount = 0;
		bBlockKind = 0xFF;

		for(j=0; j<BOARDHEIGHT; j++)
		{
			// if the first one, save block kind
			if( (iMatchCount == 0) && (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK) )
			{
				bBlockKind = g_stGameInfo.vvbBoard[j][i];
				iMatchCount++;
			}
			else
			{
				// if find the correct block kind, increase match block count
				if(g_stGameInfo.vvbBoard[j][i] == bBlockKind)
				{
					iMatchCount++;

					
					// if the match block is 3, mark previous 3 blocks as to erase
					if(iMatchCount == BLOCKCOUNT)
					{
						for(k=0; k<iMatchCount; k++)
						{
							g_stGameInfo.vvbEraseBlock[j-k][i] = ERASEBLOCK;
						}

						bMarked = TRUE;
					}

					// if the match block over 3, mark the target block as to erase
					else if(iMatchCount > BLOCKCOUNT)
					{
						g_stGameInfo.vvbEraseBlock[j][i] = ERASEBLOCK;
					}
				}
				
				// if doesn't match, start with new block
				else
				{
					if(g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK)
					{
						iMatchCount = 1;
						bBlockKind = g_stGameInfo.vvbBoard[j][i];
					}
					else
					{
						iMatchCount = 0;
						bBlockKind = 0xFF;
					}
				}
			}
		}
	}


	return bMarked;
}


BOOL MarkContinuousDiagonalBlockOnBoard(void)
{
    int iMatchCount;
    BYTE bBlockKind;
    int i;
    int j;
    int k;
    int l;
    BOOL bMarked;


    bMarked = FALSE;


    // -----------------------------------------------------------------------------------------
    // search whole game board to find over 3 continuous diagonal block (from top to bottom)
    // -----------------------------------------------------------------------------------------
	
    for(i = 0 ; i < BOARDWIDTH ; i++)
    {
        for(j = 0 ; j < BOARDHEIGHT ; j++)
        {
            iMatchCount = 0;
            bBlockKind = 0xFF;

            for(k = 0 ; ((i + k) < BOARDWIDTH) && ((j + k) < BOARDHEIGHT) ; k++)
            {
                // if the first one, save block kind
                if( (iMatchCount == 0) && (g_stGameInfo.vvbBoard[j + k][i + k] != EMPTYBLOCK) )
                {
                    bBlockKind = g_stGameInfo.vvbBoard[j + k][i + k];
                    iMatchCount++;
                }
                else
                {
					// if the block kind matches, increase block match count
                    if( g_stGameInfo.vvbBoard[ j + k ][ i + k ] == bBlockKind )
                    {
                        iMatchCount++;

						// if continuous block is 3, mark previous 3 block as to erase
                        if( iMatchCount == BLOCKCOUNT )
                        {
                            for(l = 0 ; l < iMatchCount ; l++)
                            {
                                g_stGameInfo.vvbEraseBlock[ j + k - l ][ i + k - l] = ERASEBLOCK;
                            }
                            
							bMarked = TRUE;
                        }

                        // if continuous block over 3, mark current block as to erase immediately
                        else if( iMatchCount > BLOCKCOUNT )
                        {
                            g_stGameInfo.vvbEraseBlock[ j + k ][ i + k ] = ERASEBLOCK;
                        }
                    }

                    // if the block kind doesn't match, set new block kind
                    else
                    {
                        if( g_stGameInfo.vvbBoard[ j + k ][ i + k ] != EMPTYBLOCK )
                        {
                            // set new block kind
                            iMatchCount = 1;
                            bBlockKind = g_stGameInfo.vvbBoard[ j + k ][ i + k ];
                        }
                        else
                        {
                            iMatchCount = 0;
                            bBlockKind = 0xFF;
                        }
                    }
                }
            }
        }
    }


    // -----------------------------------------------------------------------------------------
    // search whole game board to find over 3 continuous diagonal block (from bottom to top)
    // -----------------------------------------------------------------------------------------
	
    for( i = 0 ; i < BOARDWIDTH ; i++ )
    {
        for( j = 0 ; j < BOARDHEIGHT ; j++ )
        {
            iMatchCount = 0;
            bBlockKind = 0xFF;

            for( k = 0 ; ( ( i + k ) < BOARDWIDTH ) && ( ( j - k ) >= 0 ) ; k++ )
            {
				// if the first one, save block kind
                if( ( iMatchCount == 0 ) && ( g_stGameInfo.vvbBoard[ j - k ][ i + k ] != EMPTYBLOCK ) )
                {
                    bBlockKind = g_stGameInfo.vvbBoard[ j - k ][ i + k ];
                    iMatchCount++;
                }
                else
                {
					// if block kind matches, increase match block count
                    if( g_stGameInfo.vvbBoard[ j - k ][ i + k ] == bBlockKind )
                    {
                        iMatchCount++;

						// if continuous block is 3, mark previous 3 blocks as to erase
                        if( iMatchCount == BLOCKCOUNT )
                        {
                            for( l = 0 ; l < iMatchCount ; l++ )
                            {
                                g_stGameInfo.vvbEraseBlock[ j - k + l ][ i + k - l ] = ERASEBLOCK;
                            }

                            bMarked = TRUE;
                        }

						// if continuous block over 3, mark current block as to erase immediately
                        else if( iMatchCount > BLOCKCOUNT )
                        {
                            g_stGameInfo.vvbEraseBlock[ j - k ][ i + k ] = ERASEBLOCK;
                        }
                    }

					// if block kind doesn't matches, set new block kind
                    else
                    {
                        if( g_stGameInfo.vvbBoard[ j - k ][ i + k ] != EMPTYBLOCK )
                        {
							// set new block kind
                            iMatchCount = 1;
                            bBlockKind = g_stGameInfo.vvbBoard[ j - k ][ i + k ];
                        }
                        else
                        {
                            iMatchCount = 0;
                            bBlockKind = 0xFF;
                        }
                    }
                }
            }
        }
    }

    return bMarked;
}


void EraseMarkedBlock(void)
{
	int i;
	int j;


	for(j=0; j<BOARDHEIGHT; j++)
	{
		for(i=0; i<BOARDWIDTH; i++)
		{
			// if the block to erase, erase target block on game board and increase game score
			if(g_stGameInfo.vvbEraseBlock[j][i] == ERASEBLOCK)
			{
				g_stGameInfo.vvbBoard[j][i] = EMPTYBLOCK;

				g_stGameInfo.qwScore++;
			}
		}
	}
}


void CompactBlockOnBoard(void)
{
	int i;
	int j;
	int iEmptyPosition;


	// loop whole board to fill block into empty space
	for(i=0; i<BOARDWIDTH; i++)
	{
		iEmptyPosition = -1;


		// loop from bottom to top
		for(j = BOARDHEIGHT - 1; j >= 0; j--)
		{
			// if empty block, save current position to use it when move block
			if( (iEmptyPosition == -1) && (g_stGameInfo.vvbBoard[j][i] == EMPTYBLOCK) )
			{
				iEmptyPosition = j;
			}

			// if empty block found while loop and current position is not empty, move 1 step down
			else if( (iEmptyPosition != -1) && (g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK) )
			{
				g_stGameInfo.vvbBoard[iEmptyPosition][i] = g_stGameInfo.vvbBoard[j][i];

				
				// move Y coord up to continue compact
				iEmptyPosition--;


				// set current block as empty
				g_stGameInfo.vvbBoard[j][i] = EMPTYBLOCK;
			}
		}
	}
}


void EraseAllContinuousBlockOnBoard(QWORD qwWindowID)
{
	BOOL bMarked;


	// init erase block array
	memset(g_stGameInfo.vvbEraseBlock, 0, sizeof(g_stGameInfo.vvbEraseBlock));

	
	while(1)
	{
		Sleep(300);


		bMarked = FALSE;


		bMarked |= MarkContinuousHorizonBlockOnBoard();
		bMarked |= MarkContinuousVerticalBlockOnBoard();
		bMarked |= MarkContinuousDiagonalBlockOnBoard();


		// if there's no block to erase, no need to loop
		if(bMarked == FALSE)
		{
			break;
		}


		// erase marked blocks
		EraseMarkedBlock();


		// fill empty space with block
		CompactBlockOnBoard();


		// increase game level by 30 score
		g_stGameInfo.qwLevel = (g_stGameInfo.qwScore / 30) + 1;

		
		// if erased block exists, redraw game info area, game board
		DrawGameArea(qwWindowID);
		DrawInformation(qwWindowID);


		// update redrawn window to screen
		ShowWindow(qwWindowID, TRUE);
	}
}


void DrawInformation(QWORD qwWindowID)
{
	char vcBuffer[200];
	int iLength;


	// draw game information area background
	DrawRect(qwWindowID, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_WIDTH - 2, WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT,
			 RGB(55, 215, 47), TRUE);


	sprintf(vcBuffer, "Level: %d, Score: %d\n", g_stGameInfo.qwLevel, g_stGameInfo.qwScore);
	iLength = strlen(vcBuffer);


	// draw game information at the center of game info area
	DrawText(qwWindowID, (WINDOW_WIDTH - iLength * FONT_ENGLISHWIDTH) / 2, WINDOW_TITLEBAR_HEIGHT + 2,
			 RGB(255, 255, 255), RGB(55, 215, 47), vcBuffer, strlen(vcBuffer));
}


void DrawGameArea( QWORD qwWindowID )
{
    COLOR stColor;
    int i;
    int j;
    int iY;


    // Game area starts from at iY
    iY = WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT;


    // draw Game area background
    DrawRect(qwWindowID, 0, iY, BLOCKSIZE * BOARDWIDTH, iY + BLOCKSIZE * BOARDHEIGHT, RGB( 0, 0, 0 ), TRUE);


    // draw Game area contents
    for( j = 0 ; j < BOARDHEIGHT ; j++ )
    {
        for( i = 0 ; i < BOARDWIDTH ; i++ )
        {
            // draw blocks except empty
            if( g_stGameInfo.vvbBoard[j][i] != EMPTYBLOCK )
            {
                // draw inside of block
                stColor = g_stGameInfo.vstBlockColor[ g_stGameInfo.vvbBoard[j][i] ];
                DrawRect(qwWindowID, i * BLOCKSIZE, iY + (j * BLOCKSIZE), (i + 1) * BLOCKSIZE, iY + ((j + 1) * BLOCKSIZE),
                         stColor, TRUE);

                // draw outside of block
                stColor = g_stGameInfo.vstEdgeColor[ g_stGameInfo.vvbBoard[j][i] ];
                DrawRect(qwWindowID, i * BLOCKSIZE, iY + (j * BLOCKSIZE), (i + 1) * BLOCKSIZE, iY + ((j + 1) * BLOCKSIZE),
                         stColor, FALSE);
                stColor = g_stGameInfo.vstEdgeColor[ g_stGameInfo.vvbBoard[j][i] ];
                DrawRect(qwWindowID, i * BLOCKSIZE + 1, iY + (j * BLOCKSIZE) + 1, 
						 (i + 1) * BLOCKSIZE - 1, iY + ((j + 1) * BLOCKSIZE) - 1,
                         stColor, FALSE);
            }
        }
    }


    // draw currently moving block
    if(g_stGameInfo.iBlockX != -1)
    {
        for(i = 0 ; i < BLOCKCOUNT ; i++)
        {
            // draw the block only in the game area range
            if( WINDOW_TITLEBAR_HEIGHT < (iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE)) )
            {
                // draw inside of block
                stColor = g_stGameInfo.vstBlockColor[ g_stGameInfo.vbBlock[i] ];
                DrawRect(qwWindowID, g_stGameInfo.iBlockX * BLOCKSIZE, iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE),
						 (g_stGameInfo.iBlockX + 1) * BLOCKSIZE, iY + ((g_stGameInfo.iBlockY + i + 1) * BLOCKSIZE),
						 stColor, TRUE);


                // draw outside of block
                stColor = g_stGameInfo.vstEdgeColor[ g_stGameInfo.vbBlock[i] ];
                DrawRect(qwWindowID, g_stGameInfo.iBlockX * BLOCKSIZE, iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE),
						 (g_stGameInfo.iBlockX + 1) * BLOCKSIZE, iY + ((g_stGameInfo.iBlockY + i + 1) * BLOCKSIZE),
						 stColor, FALSE );
                DrawRect(qwWindowID, g_stGameInfo.iBlockX * BLOCKSIZE + 1, iY + ((g_stGameInfo.iBlockY + i) * BLOCKSIZE) + 1,
						 (g_stGameInfo.iBlockX + 1) * BLOCKSIZE - 1, iY + ((g_stGameInfo.iBlockY + i + 1) * BLOCKSIZE) - 1,
						 stColor, FALSE );
            }
        }
    }


    // draw outside of game area
    DrawRect(qwWindowID, 0, iY, BLOCKSIZE * BOARDWIDTH - 1, iY + BLOCKSIZE * BOARDHEIGHT - 1, RGB( 0, 255, 0 ), FALSE);
}



