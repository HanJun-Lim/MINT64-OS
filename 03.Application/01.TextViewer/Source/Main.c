#include "MINTOSLibrary.h"
#include "Main.h"


// Entry Point - Main()
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


	// ------------------------------------------------
	// judge Graphic Mode
	// ------------------------------------------------
	
	if(IsGraphicMode() == FALSE)
	{
		printf("This task can run only GUI mode\n");

		return -1;
	}


	// ------------------------------------------------
	// store File contents to File buffer,
	// make a buffer for File offset of line
	// ------------------------------------------------
	
	// if no argument, fail
	if(strlen(pcArgument) == 0)
	{
		printf("ex) exec textviewer.elf abc.txt\n");

		return 0;
	}


	// find the target file in directory, allocate a buffer much as File size to store the File
	// also create a buffer for File offset of line
	if(ReadFileToBuffer(pcArgument, &stInfo) == FALSE)
	{
		printf("%s file is not found\n", pcArgument);

		return 0;
	}


	// ------------------------------------------------
	// create a Window, line index.
	// and print from the first line
	// ------------------------------------------------
	
	// create a Window with size 500*500 at the center of the screen
	GetScreenArea(&stScreenArea);
	iWidth = 500;
	iHeight = 500;
	iX = (GetRectangleWidth(&stScreenArea) - iWidth) / 2;
	iY = (GetRectangleHeight(&stScreenArea) - iHeight) / 2;
	qwWindowID = CreateWindow(iX, iY, iWidth, iHeight, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE, "Text Viewer");


	// calculate File offset of line, set current(currently printing) line index 0
	CalculateFileOffsetOfLine(iWidth, iHeight, &stInfo);
	stInfo.iCurrentLineIndex = 0;


	// draw Text Buffer (from current line to max printable line)
	DrawTextBuffer(qwWindowID, &stInfo);


	// ------------------------------------------------
	// Event handling
	// ------------------------------------------------

	while(1)
	{
		// get an Event from Event Queue
		if(ReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			Sleep(10);

			continue;
		}


		// handle the Event by Event type
		switch(stReceivedEvent.qwType)
		{
			case EVENT_KEY_DOWN:

				pstKeyEvent = &(stReceivedEvent.stKeyEvent);

				// if key down...
				if(pstKeyEvent->bFlags & KEY_FLAGS_DOWN)
				{
					switch(pstKeyEvent->bASCIICode)
					{
						// Page Up / Page Down Key moves one page
						case KEY_PAGEUP:
							iMoveLine = -stInfo.iRowCount;
							break;

						case KEY_PAGEDOWN:
							iMoveLine = stInfo.iRowCount;
							break;

						// Up / Down Key moves one line
						case KEY_UP:
							iMoveLine = -1;
							break;

						case KEY_DOWN:
							iMoveLine = 1;
							break;

						default:
							iMoveLine = 0;
							break;
					}


					// if over Min/Max Line range, adjust the current line index
					if(stInfo.iCurrentLineIndex + iMoveLine < 0)
					{
						iMoveLine = -stInfo.iCurrentLineIndex;
					}
					else if(stInfo.iCurrentLineIndex + iMoveLine >= stInfo.iMaxLineCount)
					{
						iMoveLine = stInfo.iMaxLineCount - stInfo.iCurrentLineIndex - 1;
					}

					
					// if no need to move, break
					if(iMoveLine == 0)
					{
						break;
					}


					// modify current line index, print lines
					stInfo.iCurrentLineIndex += iMoveLine;
					DrawTextBuffer(qwWindowID, &stInfo);

				}	// KEY_FLAGS_DOWN endif

				break;


			// if the window resizing...
			case EVENT_WINDOW_RESIZE:

				pstWindowEvent = &(stReceivedEvent.stWindowEvent);

				iWidth = GetRectangleWidth(&(pstWindowEvent->stArea));
				iHeight = GetRectangleHeight(&(pstWindowEvent->stArea));


				// save file offset of current line index
				dwFileOffset = stInfo.pdwFileOffsetOfLine[stInfo.iCurrentLineIndex];


				// calculate Line count, Number of char per line, File offset, Current line index
				CalculateFileOffsetOfLine(iWidth, iHeight, &stInfo);
				stInfo.iCurrentLineIndex = dwFileOffset / stInfo.iColumnCount;


				// print lines from the current line
				DrawTextBuffer(qwWindowID, &stInfo);

				break;


			// if the window closing...
			case EVENT_WINDOW_CLOSE:
				
				// delete allocated Window, free allocated memories
				DeleteWindow(qwWindowID);

				free(stInfo.pbFileBuffer);
				free(stInfo.pdwFileOffsetOfLine);

				return 0;

				break;


			default:
				
				break;
		}
	}


	return 0;
}



// search target file and allocate buffer by file size,
// also allocate file offset buffer and read file to store them into memory
BOOL ReadFileToBuffer(const char* pcFileName, TEXTINFO* pstInfo)
{
	DIR* pstDirectory;
	struct dirent* pstEntry;
	DWORD dwFileSize;
	FILE* pstFile;
	DWORD dwReadSize;

	
	// ----------------------------------------------
	// open root directory to search target file
	// ----------------------------------------------
	
	pstDirectory = opendir("/");
	dwFileSize = 0;

	// find target file
	while(1)
	{
		pstEntry = readdir(pstDirectory);

		// if there's no entry to check..
		if(pstEntry == NULL)
		{
			break;
		}

		if( (strlen(pstEntry->d_name) == strlen(pcFileName)) && 
			(memcmp(pstEntry->d_name, pcFileName, strlen(pcFileName)) == 0) )
		{
			dwFileSize = pstEntry->dwFileSize;

			break;
		}
	}

	closedir(pstDirectory);

	if(dwFileSize == 0)
	{
		printf("%s file doesn't exist or size is zero\n", pcFileName);

		return FALSE;
	}


	// save file name
	memcpy(&(pstInfo->vcFileName), pcFileName, sizeof(pstInfo->vcFileName));
	pstInfo->vcFileName[sizeof(pstInfo->vcFileName) - 1] = '\0';



	// ----------------------------------------------
	// allocate File buffer, File offset buffer
	// ----------------------------------------------
	
	// allocate a buffer for file offset
	pstInfo->pdwFileOffsetOfLine = malloc( MAXLINECOUNT * sizeof(DWORD) );

	if(pstInfo->pdwFileOffsetOfLine == NULL)
	{
		printf("Memory allocation fail\n");

		return FALSE;
	}


	// allocate a buffer for a file
	pstInfo->pbFileBuffer = (BYTE*)malloc(dwFileSize);

	if(pstInfo->pbFileBuffer == NULL)
	{
		printf("Memory %d bytes allocate fail\n", dwFileSize);
		free(pstInfo->pdwFileOffsetOfLine);

		return FALSE;
	}


	// open file to store into File buffer
	pstFile = fopen(pcFileName, "r");

	if( (pstFile != NULL) && (fread(pstInfo->pbFileBuffer, 1, dwFileSize, pstFile) == dwFileSize) )
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


// analyze File buffer to calculate the file offset of line
void CalculateFileOffsetOfLine(int iWidth, int iHeight, TEXTINFO* pstInfo)
{
	DWORD i;
	int iLineIndex;
	int iColumnIndex;


	// calculate printable row/column count by Window size
	pstInfo->iColumnCount = (iWidth - MARGIN * 2) / FONT_ENGLISHWIDTH;
	pstInfo->iRowCount	  = (iHeight - (WINDOW_TITLEBAR_HEIGHT * 2) - (MARGIN * 2)) / FONT_ENGLISHHEIGHT;


	iLineIndex = 0;
	iColumnIndex = 0;

	pstInfo->pdwFileOffsetOfLine[0] = 0;
	
	for(i=0; i<pstInfo->dwFileSize; i++)
	{
		// ignore Line Feed (\r)
		if(pstInfo->pbFileBuffer[i] == '\r')
		{
			continue;
		}

		// handle tab
		else if(pstInfo->pbFileBuffer[i] == '\t')
		{
			iColumnIndex = iColumnIndex + TABSPACE;
			iColumnIndex -= iColumnIndex % TABSPACE;
		}

		else
		{
			iColumnIndex++;
		}


		// go to next line when..
		// 		1. calculated column index over max column count
		// 		2. \n detected
		if( (iColumnIndex >= pstInfo->iColumnCount) || (pstInfo->pbFileBuffer[i] == '\n') )
		{
			iLineIndex++;
			iColumnIndex = 0;


			// insert offset into Line Index Buffer
			if( i+1 < pstInfo->dwFileSize )			// check if the find pointer over the file size
			{
				pstInfo->pdwFileOffsetOfLine[iLineIndex] = i+1;
			}

			
			// if Line Index over the max line count, stop analyzing
			if(iLineIndex >= MAXLINECOUNT)
			{
				break;
			}
		}

	}	// analayzing with file pointer loop end


	// save the last Line Index
	pstInfo->iMaxLineCount = iLineIndex;
}


BOOL DrawTextBuffer(QWORD qwWindowID, TEXTINFO* pstInfo)
{
	DWORD i;
	DWORD j;
	DWORD dwBaseOffset;
	BYTE  bTemp;
	int   iXOffset;
	int	  iYOffset;
	int   iLineCountToPrint;
	int   iColumnCountToPrint;
	char  vcBuffer[100];
	RECT  stWindowArea;
	int   iLength;
	int   iWidth;
	int   iColumnIndex;


	// default value of coordinate
	iXOffset = MARGIN;
	iYOffset = WINDOW_TITLEBAR_HEIGHT;

	GetWindowArea(qwWindowID, &stWindowArea);


	// -------------------------------------------------
	// print information on File Info area
	// -------------------------------------------------
	
	iWidth = GetRectangleWidth(&stWindowArea);
	DrawRect(qwWindowID, 2, iYOffset, iWidth - 3, WINDOW_TITLEBAR_HEIGHT * 2, RGB(55, 215, 47), TRUE);

	// save File information to temporary buffer
	sprintf(vcBuffer, "File: %s, Line: %d/%d\n", pstInfo->vcFileName, pstInfo->iCurrentLineIndex + 1, pstInfo->iMaxLineCount);

	iLength = strlen(vcBuffer);

	// print File information at the center
	DrawText(qwWindowID, (iWidth - iLength * FONT_ENGLISHWIDTH) / 2, WINDOW_TITLEBAR_HEIGHT + 2,
			 RGB(255, 255, 255), RGB(55, 215, 47), vcBuffer, strlen(vcBuffer));


	// -------------------------------------------------
	// print file data
	// -------------------------------------------------
	
	// fill Data area with white, then print lines
	iYOffset = (WINDOW_TITLEBAR_HEIGHT * 2) + MARGIN;

	DrawRect(qwWindowID, iXOffset, iYOffset, 
			 iXOffset + FONT_ENGLISHWIDTH * pstInfo->iColumnCount, iYOffset + FONT_ENGLISHHEIGHT * pstInfo->iRowCount,
			 RGB(255, 255, 255), TRUE);


	// -------------------------------------------------
	// print lines with loop
	// -------------------------------------------------
	
	iLineCountToPrint = MIN( pstInfo->iRowCount, (pstInfo->iMaxLineCount - pstInfo->iCurrentLineIndex) );

	for(j=0; j<iLineCountToPrint; j++)
	{
		dwBaseOffset = pstInfo->pdwFileOffsetOfLine[pstInfo->iCurrentLineIndex + j];


		// -------------------------------------------------
		// print characters on current line
		// -------------------------------------------------

		iColumnCountToPrint = MIN( pstInfo->iColumnCount, (pstInfo->dwFileSize - dwBaseOffset) );
		iColumnIndex = 0;

		for(i=0; (i < iColumnCountToPrint) && (iColumnIndex < pstInfo->iColumnCount); i++)
		{
			bTemp = pstInfo->pbFileBuffer[i + dwBaseOffset];

			// '\n' exits the loop
			if(bTemp == '\n')
			{
				break;
			}

			// '\t' changes offset to print
			else if(bTemp == '\t')
			{
				// align the offset by Tab space
				iColumnIndex = iColumnIndex + TABSPACE;
				iColumnIndex -= iColumnIndex % TABSPACE;
			}

			// ignore Line Feed ('\r')
			else if(bTemp == '\r')
			{
				// nothing
			}
			
			// other characters is printed
			else
			{
				// print character and move to next index
				DrawText(qwWindowID, iColumnIndex * FONT_ENGLISHWIDTH + iXOffset, iYOffset + (j * FONT_ENGLISHHEIGHT),
						 RGB(0, 0, 0), RGB(255, 255, 255), &bTemp, 1);

				iColumnIndex++;
			}
		}
	}


	// update the Window
	ShowWindow(qwWindowID, TRUE);


	return TRUE;
}



