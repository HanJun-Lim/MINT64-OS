#include "MINTOSLibrary.h"
#include "Main.h"
#include "HangulInput.h"


int Main(char* pcArgument)
{
	QWORD qwWindowID;
	int iX;
	int iY;
	int iWidth;
	int iHeight;
	EVENT stReceivedEvent;
	KEYEVENT* pstKeyEvent;
	WINDOWEVENT* pstWindowEvent;
	RECT stScreenArea;
	BUFFERMANAGER stBufferManager;
	BOOL bHangulMode;



	// -------------------------------------------------------------------
	// is now Graphic mode?
	// -------------------------------------------------------------------
	
	if(IsGraphicMode() == FALSE)
	{
		printf("This task can run only GUI mode\n");
		return -1;
	}



	// -------------------------------------------------------------------
	// create a Window and a Cursor
	// -------------------------------------------------------------------
	
	// create a Window with size 60 letter * 40 pixels
	GetScreenArea(&stScreenArea);
	iWidth = MAXOUTPUTLENGTH * FONT_ENGLISHWIDTH + 5;
	iHeight = 40;

	iX = (GetRectangleWidth(&stScreenArea) - iWidth) / 2;
	iY = (GetRectangleWidth(&stScreenArea) - iHeight) / 2;

	qwWindowID = CreateWindow(iX, iY, iWidth, iHeight, WINDOW_FLAGS_DEFAULT, "One Line Memo (Alt to convert Han/Eng)");


	// initialize buffer, set input mode as english
	memset(&stBufferManager, 0, sizeof(stBufferManager));
	bHangulMode = FALSE;


	// draw a Cursor, update Window
	DrawRect(qwWindowID, 3, 4 + WINDOW_TITLEBAR_HEIGHT, 5, 3 + WINDOW_TITLEBAR_HEIGHT + FONT_ENGLISHHEIGHT, RGB(0, 250, 0), TRUE);
	ShowWindow(qwWindowID, TRUE);



	// -------------------------------------------------------------------
	// Event handling loop
	// -------------------------------------------------------------------
	
	while(1)
	{
		// get Event from Event queue
		if(ReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			Sleep(10);
			continue;
		}

		
		switch(stReceivedEvent.qwType)
		{
			case EVENT_KEY_DOWN:
				
				// get Key event
				pstKeyEvent = &(stReceivedEvent.stKeyEvent);


				// print english or compose hangul by input key
				switch(pstKeyEvent->bASCIICode)
				{
					// -----------------------------------------------
					// change input mode (Han/Eng)
					// -----------------------------------------------

					case KEY_LALT:

						// if Alt down while hangul input mode, end composing hangul
						if(bHangulMode == TRUE)
						{
							// initialize key input buffer
							stBufferManager.iInputBufferLength = 0;

							if( (stBufferManager.vcOutputBufferForProcessing[0] != '\0') &&		// buffer is not empty
								(stBufferManager.iOutputBufferLength + 2 < MAXOUTPUTLENGTH) )	// output buffer has free space
							{
								// copy composing hangul to output buffer
								memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength,
									   stBufferManager.vcOutputBufferForProcessing, 
									   2);

								stBufferManager.iOutputBufferLength += 2;


								// initialize buffer for processing
								stBufferManager.vcOutputBufferForProcessing[0] = '\0';
							}
						}

						// if Alt down while english input mode, initialize buffer for hangul composing
						else
						{
							stBufferManager.iInputBufferLength = 0;
							stBufferManager.vcOutputBufferForComplete[0] = '\0';
							stBufferManager.vcOutputBufferForProcessing[0] = '\0';
						}

						bHangulMode = TRUE - bHangulMode;

						break;


					// -----------------------------------------------
					// handle backspace
					// -----------------------------------------------
						
					case KEY_BACKSPACE:
						
						// if now composing hangul..
						// 		delete input buffer and compose hangul by remaining letter
						if( (bHangulMode == TRUE) && (stBufferManager.iInputBufferLength > 0) )
						{
							// delete one value of input buffer and re-compose hangul
							stBufferManager.iInputBufferLength--;

							ComposeHangul(stBufferManager.vcInputBuffer,
										  &stBufferManager.iInputBufferLength,
										  stBufferManager.vcOutputBufferForProcessing,
										  stBufferManager.vcOutputBufferForComplete);
						}

						// if not composing hangul now..
						// 		delete one letter in output buffer
						else
						{
							if(stBufferManager.iOutputBufferLength > 0)
							{
								// length of output buffer is more than 2 byte (at least one hangul letter) &&
								// target letter is hangul
								if( (stBufferManager.iOutputBufferLength >= 2) &&
									(stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength - 1] & 0x80) )
								{
									stBufferManager.iOutputBufferLength -= 2;

									memset(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength,
										   0, 2);
								}

								// if target letter is english, delete last 1 byte
								else
								{
									stBufferManager.iOutputBufferLength--;
									stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] = '\0';
								}
							}
						}

						break;

					
					// -------------------------------------------------------------------------------------
					// handle other key
					// 		compose hangul or insert into output buffer by current input mode
					// -------------------------------------------------------------------------------------

					default:

						// ignore special keys
						if(pstKeyEvent->bASCIICode & 0x80)
						{
							break;
						}

						
						// if now composing hangul, compose hangul
						// 		also check there's enough space to store hangul in output buffer
						if( (bHangulMode == TRUE) &&
							(stBufferManager.iOutputBufferLength + 2 <= MAXOUTPUTLENGTH) )
						{
							// if not 쌍자음 or 쌍모음, convert to lower charactor
							// 		(to prevent the 자/모음 being composed with shift)
							ConvertJaumMoumToLowerCharactor(&pstKeyEvent->bASCIICode);


							// fill key value into input buffer, increase data length
							stBufferManager.vcInputBuffer[stBufferManager.iInputBufferLength] = pstKeyEvent->bASCIICode;
							stBufferManager.iInputBufferLength++;


							// try hangul composition : give buffers for composition
							if( ComposeHangul(stBufferManager.vcInputBuffer, 
											  &stBufferManager.iInputBufferLength,
											  stBufferManager.vcOutputBufferForProcessing,
											  stBufferManager.vcOutputBufferForComplete) == TRUE )
							{
								// if there's value in vcOutputBufferForComplete..
								// 		copy value to output buffer
								if(stBufferManager.vcOutputBufferForComplete[0] != '\0')
								{
									memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength,
										   stBufferManager.vcOutputBufferForComplete,
										   2);

									stBufferManager.iOutputBufferLength += 2;


									// if there's no enough space after copying..
									// 		initialize key input buffer, vcOutputBufferForProcessing
									if(stBufferManager.iOutputBufferLength + 2 > MAXOUTPUTLENGTH)
									{
										stBufferManager.iInputBufferLength = 0;
										stBufferManager.vcOutputBufferForProcessing[0] = '\0';
									}
								}
							}

							// hangul composition failed
							// 		means that last input of input buffer is not hangul 자/모음
							// 		copy value of vcOutputBufferForComplete to output buffer
							else
							{
								// if there's value in vcOutputBufferForComplete..
								// 		copy value to output buffer
								if(stBufferManager.vcOutputBufferForComplete[0] != '\0')
								{
									memcpy(stBufferManager.vcOutputBuffer + stBufferManager.iOutputBufferLength,
										   stBufferManager.vcOutputBufferForComplete,
										   2);

									stBufferManager.iOutputBufferLength += 2;
								}

								
								// if there's enough space in output buffer..
								// 		copy last input value (not 한글 자/모) into key input buffer
								if(stBufferManager.iOutputBufferLength < MAXOUTPUTLENGTH)
								{
									stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] =
										stBufferManager.vcInputBuffer[0];

									stBufferManager.iOutputBufferLength++;
								}


								// flush key input buffer
								stBufferManager.iInputBufferLength = 0;
							}	
						}

						// case of english input mode
						else if( (bHangulMode == FALSE) && 
								 (stBufferManager.iOutputBufferLength + 1 <= MAXOUTPUTLENGTH) )
						{
							// store key input into output buffer
							stBufferManager.vcOutputBuffer[stBufferManager.iOutputBufferLength] = pstKeyEvent->bASCIICode;
							stBufferManager.iOutputBufferLength++;
						}

						break;
						
				}	// switch by key value end

				
				// -----------------------------------------------------------------------------------
				// print string in output buffer, hangul now in composing. update cursor position
				// -----------------------------------------------------------------------------------

				// print string in output buffer
				DrawText(qwWindowID, 2, WINDOW_TITLEBAR_HEIGHT + 4, RGB(0, 0, 0), RGB(255, 255, 255),
						 stBufferManager.vcOutputBuffer, MAXOUTPUTLENGTH);


				// if there's hangul in vcOutputBufferForProcessing, print it behind of string
				if(stBufferManager.vcOutputBufferForProcessing[0] != '\0')
				{
					DrawText(qwWindowID, 2 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH,
							 WINDOW_TITLEBAR_HEIGHT + 4, RGB(0, 0, 0), RGB(255, 255, 255),
							 stBufferManager.vcOutputBufferForProcessing, 2);
				}

				
				// draw cursor
				DrawRect(qwWindowID, 
						 3 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH,
						 4 + WINDOW_TITLEBAR_HEIGHT,
						 5 + stBufferManager.iOutputBufferLength * FONT_ENGLISHWIDTH,
						 3 + WINDOW_TITLEBAR_HEIGHT + FONT_ENGLISHHEIGHT,
						 RGB(0, 250, 0), TRUE);

				
				// update window
				ShowWindow(qwWindowID, TRUE);

				break;	


			case EVENT_WINDOW_CLOSE:

				DeleteWindow(qwWindowID);
				return 0;

				break;


			default:

				break;

		}	// switch by Event type end

	}	// Event handling loop end


	return 0;
}


void ConvertJaumMoumToLowerCharactor(BYTE* pbInput)
{
	// is *pbInput in upper alphabet charactor range?
	if( (*pbInput < 'A') || (*pbInput > 'Z') )
	{
		return;
	}

	
	// is *pbInput 쌍자음/쌍모음?
	switch(*pbInput)
	{
		case 'Q':	// 'ㅃ'
		case 'W':	// 'ㅉ'
		case 'E':	// 'ㄸ'
		case 'R':	// 'ㄲ'
		case 'T':	// 'ㅆ'
		case 'O':	// 'ㅒ'
		case 'P':	// 'ㅖ'
			return;

			break;
	}


	// convert *pbInput to lower charactor
	*pbInput = TOLOWER(*pbInput);
}



