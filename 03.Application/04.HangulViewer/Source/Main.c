#include "MINTOSLibrary.h"
#include "Main.h"


int Main(char* pcArgument)
{
	QWORD qwWindowID;
	int iX;
	int iY;
	int iWidth;
	int iHeight;
	TEXTINFO stInfo;
	int iMoveLine;
	EVENT stReceivedEvent;
	KEYEVENT* pstKeyEvent;
	WINDOWEVENT* pstWindowEvent;
	DWORD dwFileOffset;
	RECT stScreenArea;



	// -------------------------------------------------------------------------------
	// is this Graphic mode?
	// -------------------------------------------------------------------------------
	
	if(IsGraphicMode() == FALSE)
	{
		printf("This task can run only GUI mode\n");

		return -1;
	}



	// -------------------------------------------------------------------------------
	// save file contents to file buffer, create buffer for file offset of line
	// -------------------------------------------------------------------------------
	
	// fail if no argument exists
	if(strlen(pcArgument) == 0)
	{
		printf("ex) exec hanviewer.elf abc.txt\n");

		return 0;
	}

	
	// find target file in directory.
	// 		if target file found, allocate memory of file size to save all file contents
	// 		also, create buffer for file offset of line
	if(ReadFileToBuffer(pcArgument, &stInfo) == FALSE)
	{
		printf("%s file is not found\n", pcArgument);

		return 0;
	}
	


	// -------------------------------------------------------------------------------
	// create a Window, line index. and print lines
	// -------------------------------------------------------------------------------
	
	// create a 500*500 Window at the center of screen
	GetScreenArea(&stScreenArea);
	iWidth = 500;
	iHeight = 500;
	iX = (GetRectangleWidth(&stScreenArea) - iWidth) / 2;
	iY = (GetRectangleWidth(&stScreenArea) - iHeight) / 2;

	qwWindowID = CreateWindow(iX, iY, iWidth, iHeight, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE, "한글 뷰어(Hangul Viewer)");

	
	// calculate file offset of line. and set current line index to 0
	CalculateFileOffsetOfLine(iWidth, iHeight, &stInfo);
	stInfo.iCurrentLineIndex = 0;

	// draw text in text buffer
	DrawTextBuffer(qwWindowID, &stInfo);



	// -------------------------------------------------------------------------------
	// Event handling loop
	// -------------------------------------------------------------------------------
	
	while(1)
	{
		// receive a Event from Event queue
		if(ReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			Sleep(10);
			continue;
		}


		// handle Event by Event type
		switch(stReceivedEvent.qwType)
		{
			case EVENT_KEY_DOWN:
				
				pstKeyEvent = &(stReceivedEvent.stKeyEvent);

				if(pstKeyEvent->bFlags & KEY_FLAGS_DOWN)
				{
					switch(pstKeyEvent->bASCIICode)
					{
						// Page Up, Page Down : move row count
						case KEY_PAGEUP:
							
							iMoveLine = -stInfo.iRowCount;
							break;

						case KEY_PAGEDOWN:
							
							iMoveLine = stInfo.iRowCount;
							break;

						// Up, Down : move one line
						case KEY_UP:

							iMoveLine = -1;
							break;

						case KEY_DOWN:

							iMoveLine = 1;
							break;

						// other key ignored
						default:
							iMoveLine = 0;
							break;
					}

					
					// if out of minimum line range
					if(stInfo.iCurrentLineIndex + iMoveLine < 0)
					{
						iMoveLine = -stInfo.iCurrentLineIndex;
					}

					// if out of maximum line range
					else if(stInfo.iCurrentLineIndex + iMoveLine >= stInfo.iMaxLineCount)
					{
						iMoveLine = stInfo.iMaxLineCount - stInfo.iCurrentLineIndex - 1;
					}


					// if other key or no need to move, exit
					if(iMoveLine == 0)
					{
						break;
					}


					// modify current line index, draw text
					stInfo.iCurrentLineIndex += iMoveLine;
					DrawTextBuffer(qwWindowID, &stInfo);
				}

				
				break;

			case EVENT_WINDOW_RESIZE:

				pstWindowEvent = &(stReceivedEvent.stWindowEvent);
				iWidth = GetRectangleWidth(&(pstWindowEvent->stArea));
				iHeight = GetRectangleHeight(&(pstWindowEvent->stArea));


				// save file offset of current line
				dwFileOffset = stInfo.pdwFileOffsetOfLine[stInfo.iCurrentLineIndex];


				// calculate line count, text count of one line, file offset
				// 		calculate current line by dividing previous file offset / re-calculated column count
				CalculateFileOffsetOfLine(iWidth, iHeight, &stInfo);
				stInfo.iCurrentLineIndex = dwFileOffset / stInfo.iColumnCount;


				// draw text from current line
				DrawTextBuffer(qwWindowID, &stInfo);


				break;

			case EVENT_WINDOW_CLOSE:

				// delete a Window, free memory
				DeleteWindow(qwWindowID);
				free(stInfo.pbFileBuffer);
				free(stInfo.pdwFileOffsetOfLine);

				return 0;

				break;

			default:

				break;

		}	// switch by Event type end

	}	// Event handling loop end


	return 0;
}


BOOL ReadFileToBuffer(const char* pcFileName, TEXTINFO* pstInfo)
{	
	DIR* pstDirectory;
	struct dirent* pstEntry;
	DWORD dwFileSize;
	FILE* pstFile;
	DWORD dwReadSize;


	// -------------------------------------------------------------------------------
	// open directory to search file
	// -------------------------------------------------------------------------------
	
	pstDirectory = opendir("/");
	dwFileSize = 0;

	
	// search target file by checking file name, file name length
	while(1)
	{
		// get one entry to read
		pstEntry = readdir(pstDirectory);

		if(pstEntry == NULL)
		{
			break;
		}
		

		// check file name, file size
		if( (strlen(pstEntry->d_name) == strlen(pcFileName)) && 
			(memcmp(pstEntry->d_name, pcFileName, strlen(pcFileName)) == 0) )
		{
			dwFileSize = pstEntry->dwFileSize;

			break;
		}
	}


	// return directory handle
	closedir(pstDirectory);


	if(dwFileSize == 0)
	{
		printf("%s file doesn't exist or size is zero\n", pcFileName);

		return FALSE;
	}


	// save file name
	memcpy(&(pstInfo->vcFileName), pcFileName, sizeof(pstInfo->vcFileName));
	pstInfo->vcFileName[sizeof(pstInfo->vcFileName) - 1] = '\0';	



	// -------------------------------------------------------------------------------
	// allocate temporary buffer for file, and for file offset of line
	// -------------------------------------------------------------------------------
	
	// allocate buffer for file offset of line
	pstInfo->pdwFileOffsetOfLine = malloc(MAXLINECOUNT * sizeof(DWORD));

	if(pstInfo->pdwFileOffsetOfLine == NULL)
	{
		printf("Memory allocation fail\n");

		return FALSE;
	}


	// allocate buffer for file
	pstInfo->pbFileBuffer = (BYTE*)malloc(dwFileSize);
	
	if(pstInfo->pbFileBuffer == NULL)
	{
		printf("Memory %d bytes allocate fail\n", dwFileSize);
		free(pstInfo->pdwFileOffsetOfLine);

		return FALSE;
	}


	// open file to save it into memory
	pstFile = fopen(pcFileName, "r");

	if( (pstFile != NULL) &&
		(fread(pstInfo->pbFileBuffer, 1, dwFileSize, pstFile) == dwFileSize) )
	{
		fclose(pstFile);

		printf("%s file read success\n", pcFileName);
	}
	else
	{
		printf("%s file read fail\n", pcFileName);
		free(pstInfo->pdwFileOffsetOfLine);
		free(pstInfo->pbFileBuffer);
		fclose(pstFile);

		return FALSE;
	}


	// save file size
	pstInfo->dwFileSize = dwFileSize;


	return TRUE;
}


void CalculateFileOffsetOfLine(int iWidth, int iHeight, TEXTINFO* pstInfo)
{
	DWORD i;
	int iLineIndex;
	int iColumnIndex;
	BOOL bHangul;


	// calculate number of character per line, printable line count (consider margin space, height of title bar)
	pstInfo->iColumnCount = (iWidth - MARGIN * 2) / FONT_ENGLISHWIDTH;
	pstInfo->iRowCount	  = (iHeight - (WINDOW_TITLEBAR_HEIGHT * 2) - (MARGIN * 2)) / FONT_ENGLISHHEIGHT;


	// calculate line number of file from BOF to EOF to save file offset
	iLineIndex = 0;
	iColumnIndex = 0;
	pstInfo->pdwFileOffsetOfLine[0] = 0;

	for(i=0; i < pstInfo->dwFileSize; i++)
	{
		// ignore line feed (\r)
		if(pstInfo->pbFileBuffer[i] == '\r')
		{
			continue;
		}

		// if tab, change print offset (cursor position) by TABSPACE
		else if(pstInfo->pbFileBuffer[i] == '\t')
		{
			iColumnIndex = iColumnIndex + TABSPACE;
			iColumnIndex -= iColumnIndex % TABSPACE;
		}

		// handle hangul
		else if(pstInfo->pbFileBuffer[i] & 0x80)
		{
			bHangul = TRUE;
			iColumnIndex += 2;
			i++;
		}

		else
		{
			bHangul = FALSE;
			iColumnIndex++;
		}


		// change line if..
		//		1. '\n' found
		//		2. print offset (cursor position) is over the max character count of line
		//		3. no enough space to print tab
		if( (iColumnIndex >= pstInfo->iColumnCount) || (pstInfo->pbFileBuffer[i] == '\n') )
		{
			iLineIndex++;
			iColumnIndex = 0;


			// if current character to print is hangul, print it to other line
			if(bHangul == TRUE)
			{
				i -= 2;
			}


			// save file offset to line index buffer
			if( i + 1 < pstInfo->dwFileSize )
			{
				pstInfo->pdwFileOffsetOfLine[iLineIndex] = i+1;
			}


			// exit if over the supportable line count
			if(iLineIndex >= MAXLINECOUNT)
			{
				break;
			}
		}

	}	// file character analyze loop end


	// save last line number
	pstInfo->iMaxLineCount = iLineIndex;
}


BOOL DrawTextBuffer(QWORD qwWindowID, TEXTINFO* pstInfo)
{
	DWORD i;
	DWORD j;
	DWORD dwBaseOffset;
	BYTE bTemp;
	int iXOffset;
	int iYOffset;
	int iLineCountToPrint;
	int iColumnCountToPrint;
	char vcBuffer[100];
	RECT stWindowArea;
	int iLength;
	int iWidth;
	int iColumnIndex;


	// base value of coordinate
	iXOffset = MARGIN;
	iYOffset = WINDOW_TITLEBAR_HEIGHT;
	GetWindowArea(qwWindowID, &stWindowArea);


	// ----------------------------------------------------------
	// draw information in file information area
	// ----------------------------------------------------------
	
	// print file name, current line, line count
	iWidth = GetRectangleWidth(&stWindowArea);
	DrawRect(qwWindowID, 2, iYOffset, iWidth - 3, WINDOW_TITLEBAR_HEIGHT * 2, RGB(55, 215, 47), TRUE);

	sprintf(vcBuffer, "파일: %s, 행 번호: %d/%d\n", pstInfo->vcFileName, pstInfo->iCurrentLineIndex + 1, pstInfo->iMaxLineCount);
	iLength = strlen(vcBuffer);

	DrawText(qwWindowID, (iWidth - iLength * FONT_ENGLISHWIDTH) / 2, WINDOW_TITLEBAR_HEIGHT + 2, 
			 RGB(255, 255, 255), RGB(55, 215, 47), vcBuffer, strlen(vcBuffer));



	// ----------------------------------------------------------
	// draw file contents in file contents area
	// ----------------------------------------------------------
	
	// draw whole file contents area with white at first
	iYOffset = (WINDOW_TITLEBAR_HEIGHT * 2) + MARGIN;
	DrawRect(qwWindowID, iXOffset, iYOffset, 
			 iXOffset + FONT_ENGLISHWIDTH * pstInfo->iColumnCount, iYOffset + FONT_ENGLISHHEIGHT * pstInfo->iRowCount,
			 RGB(255, 255, 255), TRUE);


	// ----------------------------------------------------------
	// draw lines by looping
	// ----------------------------------------------------------
	
	iLineCountToPrint = MIN( pstInfo->iRowCount, (pstInfo->iMaxLineCount - pstInfo->iCurrentLineIndex) );

	for(j=0; j < iLineCountToPrint; j++)
	{
		// file offset of line to print
		dwBaseOffset = pstInfo->pdwFileOffsetOfLine[pstInfo->iCurrentLineIndex + j];
	
		
		// ----------------------------------------------------------
		// draw character in current line 
		// ----------------------------------------------------------

		iColumnCountToPrint = MIN( pstInfo->iColumnCount, (pstInfo->dwFileSize - dwBaseOffset) );
		iColumnIndex = 0;

		for(i=0; (i < iColumnCountToPrint) && (iColumnIndex < pstInfo->iColumnCount); i++)
		{
			bTemp = pstInfo->pbFileBuffer[i + dwBaseOffset];

			
			// if new line, break
			if(bTemp == '\n')
			{
				break;
			}

			// if tab, change offset
			else if(bTemp == '\t')
			{
				iColumnIndex = iColumnIndex + TABSPACE; 
				iColumnIndex -= iColumnIndex % TABSPACE;
			}

			// if line feed, ignore it
			else if(bTemp == '\r')
			{
				// nothing
			}

			// draw other characters
			else
			{
				// if english..
				if( (bTemp & 0x80) == 0 )
				{
					DrawText(qwWindowID, iColumnIndex * FONT_ENGLISHWIDTH + iXOffset, iYOffset + (j * FONT_ENGLISHHEIGHT),
							 RGB(0, 0, 0), RGB(255, 255, 255), &bTemp, 1);
					iColumnIndex++;
				}

				// if hangul, handle hangle that not over the max column count only
				else if( (iColumnIndex + 2) < pstInfo->iColumnCount )
				{
					DrawText(qwWindowID, iColumnIndex * FONT_ENGLISHWIDTH + iXOffset, iYOffset + (j * FONT_ENGLISHHEIGHT),
							 RGB(0, 0, 0), RGB(255, 255, 255), &pstInfo->pbFileBuffer[i + dwBaseOffset], 2);

					iColumnIndex += 2;
					i++;
				}
			}

		}	// loop for each character end

	}	// loop for each line end
	
	
	// update window
	ShowWindow(qwWindowID, TRUE);
	
	
	return TRUE;
}



