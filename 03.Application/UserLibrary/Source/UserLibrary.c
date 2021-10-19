#include "Types.h"
#include "UserLibrary.h"
#include <stdarg.h>


// ==================================================
// for Video I/O, Standard Function
// ==================================================


// Fill memory with specific value (bData)
void memset(void* pvDestination, BYTE bData, int iSize)
{
	int i;
	QWORD qwData;
	int iRemainByteStartOffset;

	
	// fill 8 byte at once
	qwData = 0;

	for(i=0; i<8; i++)
	{
		qwData = (qwData << 8) | bData;
	}

	
	// fill 8 byte to destination
	for(i=0; i < (iSize / 8); i++)
	{
		((QWORD*)pvDestination)[i] = qwData;
	}


	// fill remaning bytes to destination
	iRemainByteStartOffset = i*8;

	for(i=0; i < (iSize % 8); i++)
	{
		((char*)pvDestination)[iRemainByteStartOffset++] = bData;
	}
}


// copy pvSource to pvDestination much as iSize
int memcpy(void* pvDestination, const void* pvSource, int iSize)
{
	int i;
	int iRemainByteStartOffset;


	// copy 8 byte at once
	for(i=0; i < (iSize / 8); i++)
	{

		((QWORD*)pvDestination)[i] = ((QWORD*)pvSource)[i];
	}

	
	// copy remaning bytes
	iRemainByteStartOffset = i*8;

	for(i=0; i < (iSize % 8); i++)
	{
		((char*)pvDestination)[iRemainByteStartOffset] = ((char*)pvSource)[iRemainByteStartOffset];
		
		iRemainByteStartOffset++;
	}


	return iSize;
}


// compare pvDestination with pvSource
// 		return value 0 means two parameter is same, otherwise not same
int memcmp(const void* pvDestination, const void* pvSource, int iSize)
{
	int i, j;
	int iRemainByteStartOffset;
	QWORD qwValue;
	char cValue;


	// verify 8 bytes
	for(i=0; i < (iSize / 8); i++)
	{
		qwValue = ((QWORD*)pvDestination)[i] - ((QWORD*)pvSource)[i];

		if(qwValue != 0)
		{
			// find incorrect part
			for(j=0; j<8; j++)
			{
				// search from rear to front (from bit 0 to bit 63)
				if( ((qwValue >> (j*8)) & 0xFF) != 0 )
				{
					return (qwValue >> (j*8)) & 0xFF;
				}
			}
		}
	}


	// verify remaining bytes
	iRemainByteStartOffset = i*8;

	for(i=0; i < (iSize % 8); i++)
	{
		cValue = ((char*)pvDestination)[iRemainByteStartOffset] - ((char*)pvSource)[iRemainByteStartOffset];

		if(cValue != 0)
		{
			return cValue;
		}

		iRemainByteStartOffset++;
	}


	return 0;
}


// copy pcSource to pcDestination 
int strcpy(char* pcDestination, const char* pcSource)
{
	int i;

	for(i=0; pcSource[i] != 0; i++)
	{
		pcDestination[i] = pcSource[i];
	}


	return i;
}


// compare two string (pcDestination, pcSource)
int strcmp(char* pcDestination, const char* pcSource)
{
	int i;

	for(i=0; (pcDestination[i] != 0) && (pcSource[i] != 0); i++)
	{
		if( (pcDestination[i] - pcSource[i]) != 0 )
		{
			break;
		}
	}

	return (pcDestination[i] - pcSource[i]);
}


// return string length
int strlen(const char* pcBuffer)
{
	int i;

	for(i=0; ; i++)
	{
		if(pcBuffer[i] == '\0')
		{
			break;
		}
	}


	return i;
}


// ascii to integer
long atoi(const char* pcBuffer, int iRadix)
{
	long lReturn;


	switch(iRadix)
	{
		// Hexadecimal
		case 16:
			
			lReturn = HexStringToQword(pcBuffer);
			
			break;

		// Decimal, etc
		case 10:
		default:

			lReturn = DecimalStringToLong(pcBuffer);

			break;
	}


	return lReturn;
}


// convert hexadecimal string to qword
// 		for atoi()
QWORD HexStringToQword(const char* pcBuffer)
{
	QWORD qwValue = 0;
	int i;


	for(i=0; pcBuffer[i] != '\0'; i++)
	{
		qwValue *= 16;

		if( ('A' <= pcBuffer[i]) && (pcBuffer[i] <= 'Z') )
		{
			qwValue += (pcBuffer[i] - 'A') + 10;
		}
		else if( ('a' <= pcBuffer[i]) && (pcBuffer[i] <= 'z') )
		{
			qwValue += (pcBuffer[i] - 'a') + 10;
		}
		else
		{
			qwValue += pcBuffer[i] - '0';
		}
	}


	return qwValue;
}


// convert decimal string to long
// 		for atoi()
long DecimalStringToLong(const char* pcBuffer)
{
	long lValue = 0;
	int i;

	
	if(pcBuffer[0] == '-')
	{
		i = 1;
	}
	else
	{
		i = 0;
	}


	for( ; pcBuffer[i] != '\0'; i++)
	{
		lValue *= 10;
		lValue += pcBuffer[i] - '0';
	}


	if(pcBuffer[0] == '-')
	{
		lValue = -lValue;
	}


	return lValue;
}


// integer to ascii
int itoa(long lValue, char* pcBuffer, int iRadix)
{
	int iReturn;


	switch(iRadix)
	{
		// hexadecimal
		case 16:
		
			iReturn = HexToString(lValue, pcBuffer);

			break;

		// decimal, etc
		case 10:
		default:

			iReturn = DecimalToString(lValue, pcBuffer);

			break;
	}

	
	return iReturn;
}


// convert hexadecimal number to string
// 		for itoa()
int HexToString(QWORD qwValue, char* pcBuffer)
{
	QWORD i;
	QWORD qwCurrentValue;


	// 0 prints '0' immediately
	if(qwValue == 0)
	{
		pcBuffer[0] = '0';
		pcBuffer[1] = '\0';

		return 1;
	}

	
	// insert number in order 16, 256...
	for(i=0; qwValue > 0; i++)
	{
		qwCurrentValue = qwValue % 16;

		if(qwCurrentValue >= 10)
		{
			pcBuffer[i] = 'A' + (qwCurrentValue - 10);
		}
		else
		{
			pcBuffer[i] = '0' + qwCurrentValue;
		}

		qwValue = qwValue / 16;
	}

	pcBuffer[i] = '\0';


	// reverse string
	ReverseString(pcBuffer);

	return i;
}


// convert decimal number to string
// 		for itoa()
int DecimalToString(long lValue, char* pcBuffer)
{
	long i;


	// 0 prints '0' immediately
	if(lValue == 0)
	{
		pcBuffer[0] = '0';
		pcBuffer[1] = '\0';
		return 1;
	}


	// if negative, add '-' to buffer, convert to positive number
	if(lValue < 0)
	{
		i = 1;
		pcBuffer[0] = '-';
		lValue = -lValue;
	}
	else
	{
		i = 0;
	}


	// insert number in order 1, 10, 100, 1000...
	for( ; lValue > 0; i++)
	{
		pcBuffer[i] = '0' + lValue % 10;
		lValue = lValue / 10;
	}
	pcBuffer[i] = '\0';


	// reverse string
	if(pcBuffer[0] == '-')
	{
		// if negative, except '-' (buffer[0])
		ReverseString(&(pcBuffer[1]));
	}
	else
	{
		ReverseString(pcBuffer);
	}


	return i;
}


// reverse string order
void ReverseString(char* pcBuffer)
{
	int iLength;
	int i;
	char cTemp;

	
	// swap left side char with right side char
	iLength = strlen(pcBuffer);

	for(i=0; i < iLength / 2; i++)
	{
		cTemp = pcBuffer[i];
		pcBuffer[i] = pcBuffer[iLength - 1 - i];
		pcBuffer[iLength - 1 - i] = cTemp;
	}
}


// copy formatted string to buffer
// 		return length of buffer
int sprintf(char* pcBuffer, const char* pcFormatString, ...)
{
	va_list ap;							// type to hold information about variable arguments
	int iReturn;


	va_start(ap, pcFormatString);		// initialize a variable argument list
	iReturn = vsprintf(pcBuffer, pcFormatString, ap);
	va_end(ap);							// end using variable argument list


	return iReturn;
}


// for printf(), sprintf()
// 		return final length of buffer
int vsprintf(char* pcBuffer, const char* pcFormatString, va_list ap)
{
	QWORD i, j, k;
	int   iBufferIndex = 0;
	int   iFormatLength, iCopyLength;
	char* pcCopyString;
	QWORD qwValue;
	int   iValue;
	double dValue;

	
	// get length of format string, print data to output buffer
	iFormatLength = strlen(pcFormatString);

	for(i=0; i<iFormatLength; i++)
	{
		// if '%', handle it as Data Type Character
		if(pcFormatString[i] == '%')
		{
			i++;
			
			switch(pcFormatString[i])
			{
				case 's':		// String
					pcCopyString = (char*)(va_arg(ap, char*));			// va_arg() : retrieve next argument
					iCopyLength  = strlen(pcCopyString);

					memcpy(pcBuffer + iBufferIndex, pcCopyString, iCopyLength);
					iBufferIndex += iCopyLength;
					break;

				case 'c':		// Character
					pcBuffer[iBufferIndex] = (char)(va_arg(ap, int));
					iBufferIndex++;
					break;


				case 'd':
				case 'i':		// Integer
					iValue = (int)(va_arg(ap, int));
					iBufferIndex += itoa(iValue, pcBuffer + iBufferIndex, 10);
					break;


				case 'x':		
				case 'X':		// 4 Byte Hexadecimal
					qwValue = (DWORD)(va_arg(ap, DWORD)) & 0xFFFFFFFF;
					iBufferIndex += itoa(qwValue, pcBuffer + iBufferIndex, 16);
					break;


				case 'q':
				case 'Q':
				case 'p':		// 8 Byte Hexadecimal
					qwValue = (QWORD)(va_arg(ap, QWORD));
					iBufferIndex += itoa(qwValue, pcBuffer + iBufferIndex, 16);
					break;


				case 'f':		// floating point
					dValue = (double)(va_arg(ap, double));

					// round off at third digit
					dValue += 0.005;


					// calculate from second decimal point
					pcBuffer[iBufferIndex]   = '0' + (QWORD)(dValue * 100) % 10;
					pcBuffer[iBufferIndex+1] = '0' + (QWORD)(dValue * 10) % 10;
					pcBuffer[iBufferIndex+2] = '.';


					// decimal calculation
					for(k=0; ; k++)
					{
						// exit if decimal part is 0
						if( ((QWORD)dValue == 0) && (k != 0) )
						{
							break;
						}

						pcBuffer[iBufferIndex + 3 + k] = '0' + ((QWORD)dValue % 10);
						dValue = dValue / 10;
					}

					pcBuffer[iBufferIndex + 3 + k] = '\0';
					
					ReverseString(pcBuffer + iBufferIndex);
					iBufferIndex += 3+k;		// two fractional part + point + number of decimal
					break;


				default:		// if else, print String
					pcBuffer[iBufferIndex] = pcFormatString[i];
					iBufferIndex++;
					break;
			}
		}
		
		// string (not format)
		else
		{
			pcBuffer[iBufferIndex] = pcFormatString[i];
			iBufferIndex++;
		}
	}

	// end of Buffer
	pcBuffer[iBufferIndex] = '\0';


	return iBufferIndex;
}


void printf(const char* pcFormatString, ...)
{
	va_list ap;
	char vcBuffer[1024];
	int iNextPrintOffset;


	va_start(ap, pcFormatString);
	vsprintf(vcBuffer, pcFormatString, ap);
	va_end(ap);


	// print formatted string
	iNextPrintOffset = ConsolePrintString(vcBuffer);


	// update cursor position
	SetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}


// for making random value
static volatile QWORD gs_qwRandomValue = 0;


// set initial value of random value (seed)
void srand(QWORD qwSeed)
{
	gs_qwRandomValue = qwSeed;
}


// return random value
QWORD rand(void)
{
	gs_qwRandomValue = (gs_qwRandomValue * 412153 + 5571031) >> 16;

	return gs_qwRandomValue;
}



// ==================================================
// for GUI System
// ==================================================


// is (iX, iY) in pstArea?
BOOL IsInRectangle(const RECT* pstArea, int iX, int iY)
{
	if( (iX < pstArea->iX1) || (pstArea->iX2 < iX) || (iY < pstArea->iY1) || (pstArea->iY2 < iY) )
	{
		return FALSE;
	}

	return TRUE;
}


// return width of pstArea
int GetRectangleWidth(const RECT* pstArea)
{
	int iWidth;

	iWidth = pstArea->iX2 - pstArea->iX1 + 1;

	if(iWidth < 0)
	{
		return -iWidth;
	}

	return iWidth;
}


// return height of pstArea
int GetRectangleHeight(const RECT* pstArea)
{
	int iHeight;

	iHeight = pstArea->iY2 - pstArea->iY1 + 1;

	if(iHeight < 0)
	{
		return -iHeight;
	}

	return iHeight;
}


// get overlapped area between pstArea1 and pstArea2
BOOL GetOverlappedRectangle(const RECT* pstArea1, const RECT* pstArea2, RECT* pstInterSection)
{
	int iMaxX1;
	int iMinX2;
	int iMaxY1;
	int iMinY2;


	iMaxX1 = MAX(pstArea1->iX1, pstArea2->iX1);
	iMinX2 = MIN(pstArea1->iX2, pstArea2->iX2);

	// not overlapped
	if(iMinX2 < iMaxX1)
	{
		return FALSE;
	}


	iMaxY1 = MAX(pstArea1->iY1, pstArea2->iY1);
	iMinY2 = MIN(pstArea1->iY2, pstArea2->iY2);

	// not overlapped
	if(iMinY2 < iMaxY1)
	{
		return FALSE;
	}


	// set intersection information
	pstInterSection->iX1 = iMaxX1;
	pstInterSection->iY1 = iMaxY1;
	pstInterSection->iX2 = iMinX2;
	pstInterSection->iY2 = iMinY2;


	return TRUE;
}


// convert Screen (X, Y) to Window coordinate
BOOL ConvertPointScreenToClient(QWORD qwWindowID, const POINT* pstXY, POINT* pstXYInWindow)
{
	RECT stArea;

	
	// get Window area
	if(GetWindowArea(qwWindowID, &stArea) == FALSE)
	{
		return FALSE;
	}


	pstXYInWindow->iX = pstXY->iX - stArea.iX1;			// Screen-based iX - iX1 of Window
	pstXYInWindow->iY = pstXY->iY - stArea.iY1;			// Screen-based iY - iY1 of Window
	

	return TRUE;
}


// convert Window (X, Y) to Screen coordinate
BOOL ConvertPointClientToScreen(QWORD qwWindowID, const POINT* pstXY, POINT* pstXYInScreen)
{
	RECT stArea;

	
	// get Window area
	if(GetWindowArea(qwWindowID, &stArea) == FALSE)
	{
		return FALSE;
	}


	pstXYInScreen->iX = pstXY->iX + stArea.iX1;			// Window-based iX + iX1 of Window
	pstXYInScreen->iY = pstXY->iY + stArea.iY1;			// Window-based iY + iY1 of Window


	return TRUE;
}


// convert Screen-based RECT coordinate to Window-based
BOOL ConvertRectScreenToClient(QWORD qwWindowID, const RECT* pstArea, RECT* pstAreaInWindow)
{
	RECT stWindowArea;


	// get Window area
	if(GetWindowArea(qwWindowID, &stWindowArea) == FALSE)
	{
		return FALSE;
	}


	pstAreaInWindow->iX1 = pstArea->iX1 - stWindowArea.iX1;
	pstAreaInWindow->iY1 = pstArea->iY1 - stWindowArea.iY1;
	pstAreaInWindow->iX2 = pstArea->iX2 - stWindowArea.iX1;
	pstAreaInWindow->iY2 = pstArea->iY2 - stWindowArea.iY1;


	return TRUE;
}


// convert Window-based RECT coordinate to Screen-based
BOOL ConvertRectClientToScreen(QWORD qwWindowID, const RECT* pstArea, RECT* pstAreaInScreen)
{
	RECT stWindowArea;


	// get Window area
	if(GetWindowArea(qwWindowID, &stWindowArea) == FALSE)
	{
		return FALSE;
	}

	
	pstAreaInScreen->iX1 = pstArea->iX1 + stWindowArea.iX1;
	pstAreaInScreen->iY1 = pstArea->iY1 + stWindowArea.iY1;
	pstAreaInScreen->iX2 = pstArea->iX2 + stWindowArea.iX1;
	pstAreaInScreen->iY2 = pstArea->iY2 + stWindowArea.iY1;


	return TRUE;
}


// fill RECT Data Structure
void SetRectangleData(int iX1, int iY1, int iX2, int iY2, RECT* pstRect)
{
	// make always x1 < x2
	if(iX1 < iX2)
	{
		pstRect->iX1 = iX1;
		pstRect->iX2 = iX2;
	}
	else
	{
		pstRect->iX1 = iX2;
		pstRect->iX2 = iX1;
	}


	// make always y1 < y2
	if(iY1 < iY2)
	{
		pstRect->iY1 = iY1;
		pstRect->iY2 = iY2;
	}
	else
	{
		pstRect->iY1 = iY2;
		pstRect->iY2 = iY1;
	}
}


// set Mouse Event
BOOL SetMouseEvent(QWORD qwWindowID, QWORD qwEventType, int iMouseX, int iMouseY, BYTE bButtonStatus, EVENT* pstEvent)
{
	POINT stMouseXYInWindow;
	POINT stMouseXY;


	// make Mouse Event by Event type
	switch(qwEventType)
	{
		case EVENT_MOUSE_MOVE:
		case EVENT_MOUSE_LBUTTONDOWN:
		case EVENT_MOUSE_LBUTTONUP:
		case EVENT_MOUSE_RBUTTONDOWN:
		case EVENT_MOUSE_RBUTTONUP:
		case EVENT_MOUSE_MBUTTONDOWN:
		case EVENT_MOUSE_MBUTTONUP:

			// set Mouse (X, Y)
			stMouseXY.iX = iMouseX;
			stMouseXY.iY = iMouseY;

			// convert Screen-based (X, Y) to Window-based (X, Y)
			if(ConvertPointScreenToClient(qwWindowID, &stMouseXY, &stMouseXYInWindow) == FALSE)
			{
				return FALSE;
			}


			// set Event type
			pstEvent->qwType = qwEventType;

			// set Window ID
			pstEvent->stMouseEvent.qwWindowID = qwWindowID;
			
			// set Mouse button status
			pstEvent->stMouseEvent.bButtonStatus = bButtonStatus;

			// set Mouse point (Window-based)
			memcpy(&(pstEvent->stMouseEvent.stPoint), &stMouseXYInWindow, sizeof(POINT));

			break;

		default:

			return FALSE;

			break;
	}

	return TRUE;
}


// set Window Event
BOOL SetWindowEvent(QWORD qwWindowID, QWORD qwEventType, EVENT* pstEvent)
{
	RECT stArea;


	// make Window Event by Event type
	switch(qwEventType)
	{
		case EVENT_WINDOW_SELECT:
		case EVENT_WINDOW_DESELECT:
		case EVENT_WINDOW_MOVE:
		case EVENT_WINDOW_RESIZE:
		case EVENT_WINDOW_CLOSE:

			// set Event type
			pstEvent->qwType = qwEventType;

			// set Window ID
			pstEvent->stWindowEvent.qwWindowID = qwWindowID;

			// get Window area to set
			if(GetWindowArea(qwWindowID, &stArea) == FALSE)
			{
				return FALSE;
			}

			memcpy(&(pstEvent->stWindowEvent.stArea), &stArea, sizeof(RECT));

			break;

		default:

			return FALSE;

			break;
	}

	return TRUE;
}


// set Key Event
void SetKeyEvent(QWORD qwWindow, const KEYDATA* pstKeyData, EVENT* pstEvent)
{
	// set Event type
	if(pstKeyData->bFlags & KEY_FLAGS_DOWN)
	{
		pstEvent->qwType = EVENT_KEY_DOWN;
	}
	else
	{
		pstEvent->qwType = EVENT_KEY_UP;
	}

	// set other info
	pstEvent->stKeyEvent.bASCIICode = pstKeyData->bASCIICode;
	pstEvent->stKeyEvent.bScanCode  = pstKeyData->bScanCode;
	pstEvent->stKeyEvent.bFlags		= pstKeyData->bFlags;
}



