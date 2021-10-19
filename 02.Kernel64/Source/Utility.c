#include "Utility.h"
#include "AssemblyUtility.h"
#include <stdarg.h>
#include "VBE.h"


// Counter for PIT Controller occurence
volatile QWORD g_qwTickCount = 0;


// fill memory with specific value
void kMemSet(void* pvDestination, BYTE bData, int iSize)
{
	int i;
	QWORD qwData;
	int iRemainByteStartOffset;

	/*
	for(i=0; i<iSize; i++)
		((char*)pvDestination)[i] = bData;
	*/

	
	// Fill 8 byte Data
	qwData = 0;

	for(i=0; i<8; i++)
	{
		qwData = (qwData << 8) | bData;
	}


	// Fill 8 byte to Data
	for(i=0; i<(iSize / 8); i++)
	{
		((QWORD*)pvDestination)[i] = qwData;
	}


	// Fill remaining bytes
	iRemainByteStartOffset = i*8;

	for(i=0; i<(iSize % 8); i++)
	{
		((char*)pvDestination)[iRemainByteStartOffset++] = bData;
	}
}


// memory copy
int kMemCpy(void* pvDestination, const void* pvSource, int iSize)
{
	int i;
	int iRemainByteStartOffset;

	/*
	for(i=0; i<iSize; i++)
		((char*)pvDestination)[i] = ((char*)pvSource)[i];
	*/


	// Copy data by 8 Byte
	for(i=0; i<(iSize / 8); i++)
	{
		((QWORD*)pvDestination)[i] = ((QWORD*)pvSource)[i];
	}


	// Copy remaining data
	iRemainByteStartOffset = i*8;

	for(i=0; i<(iSize % 8); i++)
	{
		((char*)pvDestination)[iRemainByteStartOffset] = ((char*)pvSource)[iRemainByteStartOffset];

		iRemainByteStartOffset++;
	}


	return iSize;
}


// memory compare
int kMemCmp(const void* pvDestination, const void* pvSource, int iSize)
{
	int i, j;
	int iRemainByteStartOffset;
	QWORD qwValue;
	char cValue;

	/*
	for(i=0; i<iSize; i++)
	{
		cTemp = ((char*)pvDestination)[i] - ((char*)pvSource)[i];

		if(cTemp != 0)
			return (int)cTemp;
	}
	*/

	
	// Verity 8 bytes
	for(i=0; i<(iSize / 8); i++)
	{
		qwValue = ((QWORD*)pvDestination)[i] - ((QWORD*)pvSource)[i];
		
		if(qwValue != 0)
		{
			// Search incorrect part
			for(j=0; j<8; j++)
			{
				if( ((qwValue >> (j*8)) & 0xFF) != 0 )
				{
					return (qwValue >> (j*8)) & 0xFF;
				}
			}
		}
	}

	
	// Verify remaining bytes
	iRemainByteStartOffset = i*8;

	for(i=0; i<(iSize % 8); i++)
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


// change Interrupt Flag of RFLAGS Register, return previous Interrupt Flag status
BOOL kSetInterruptFlag(BOOL bEnableInterrupt)
{
	QWORD qwRFLAGS;


	// Read previous RFLAGS, activate/deactivate Interrupt
	qwRFLAGS = kReadRFLAGS();

	if(bEnableInterrupt == TRUE)
		kEnableInterrupt();
	else
		kDisableInterrupt();

	// check bit 9 (IF bit) of prev RFLAGS Reg, return previous Interrupt status
	if(qwRFLAGS & 0x0200)
		return TRUE;

	return FALSE;
}


// return length of string
int kStrLen(const char* pcBuffer)
{
	int i;
	
	for(i=0; ;i++)
	{
		if(pcBuffer[i] == '\0')
			break;
	}

	return i;
}


// RAM Total Size
static gs_qwTotalRAMMBSize = 0;

// Check RAM Size (from 64 MB)
// 		should be called once on boot sequence
void kCheckTotalRAMSize(void)
{
	DWORD* pdwCurrentAddress;
	DWORD  dwPreviousValue;

	// check RAM size (from 64MB) by 4MB
	pdwCurrentAddress = (DWORD*)0x4000000;		// 1024^2 * 64
	
	while(1)
	{
		dwPreviousValue = *pdwCurrentAddress;
		*pdwCurrentAddress = 0x12345678;

		if(*pdwCurrentAddress != 0x12345678)
			break;
		
		*pdwCurrentAddress = dwPreviousValue;

		pdwCurrentAddress += (0x400000 / 4);
	}

	gs_qwTotalRAMMBSize = (QWORD)pdwCurrentAddress / 0x100000;		// Current Address / 1MB, return MB Size
}


QWORD kGetTotalRAMSize(void)
{
	return gs_qwTotalRAMMBSize;
}



// Convert ASCII to Integer
long kAToI(const char* pcBuffer, int iRadix)
{
	long lReturn;

	switch(iRadix)
	{
		case 16:
			lReturn = kHexStringToQword(pcBuffer);
			break;

		case 10:
		default:
			lReturn = kDecimalStringToLong(pcBuffer);
			break;
	}

	return lReturn;
}


QWORD kHexStringToQword(const char* pcBuffer)		// for kAToI(), Hexadecimal
{
	QWORD qwValue = 0;
	int i;

	for(i=0; pcBuffer[i] != '\0'; i++)
	{
		qwValue *= 16;
		
		if( ('A' <= pcBuffer[i]) && (pcBuffer[i] <= 'Z') )		// case of Uppercase Alphabet
			qwValue += (pcBuffer[i] - 'A') + 10;				
		else if( ('a' <= pcBuffer[i]) && (pcBuffer[i] <= 'z') ) // case of Lowercase Alphabet
			qwValue += (pcBuffer[i] - 'a') + 10;
		else													// case of Number
			qwValue += pcBuffer[i] - '0';
	}
	
	return qwValue;
}


long kDecimalStringToLong(const char* pcBuffer)		// for kAToI(), Decimal
{
	long lValue = 0;
	int i;

	// is this negative ?
	if(pcBuffer[0] == '-')
		i = 1;
	else
		i = 0;

	
	for( ; pcBuffer[i] != '\0'; i++)
	{
		lValue *= 10;
		lValue += pcBuffer[i] - '0';
	}

	// if negative, add '-'
	if(pcBuffer[0] == '-')
		lValue = -lValue;
	
	return lValue;
}


// Convert Integer to ASCII
int kIToA(long lValue, char* pcBuffer, int iRadix)
{
	int iReturn;
	
	switch(iRadix)
	{
		case 16:
			iReturn = kHexToString(lValue, pcBuffer);
			break;

		case 10:
		default:
			iReturn = kDecimalToString(lValue, pcBuffer);
			break;
	}

	return iReturn;
}


int kHexToString(QWORD qwValue, char* pcBuffer)		// for kIToA(), Hexadecimal
{
	QWORD i;
	QWORD qwCurrentValue;


	// case of zero
	if(qwValue == 0)
	{
		pcBuffer[0] = '0';
		pcBuffer[1] = '\0';
		return 1;
	}

	for(i=0; qwValue > 0; i++)
	{
		qwCurrentValue = qwValue % 16;
		
		if(qwCurrentValue >= 10)
			pcBuffer[i] = 'A' + (qwCurrentValue - 10);
		else
			pcBuffer[i] = '0' + qwCurrentValue;

		qwValue = qwValue / 16;
	}
	
	pcBuffer[i] = '\0';

	kReverseString(pcBuffer);
	
	return i;
}


int kDecimalToString(long lValue, char* pcBuffer)	// for kIToA(), Decimal
{
	long i;


	// case of zero
	if(lValue == 0)
	{
		pcBuffer[0] = '0';
		pcBuffer[1] = '\0';
		return 1;
	}

	// case of negative
	if(lValue < 0)
	{
		i = 1;
		pcBuffer[0] = '-';
		lValue = -lValue;
	}
	else
		i = 0;

	
	for( ; lValue > 0; i++)
	{
		pcBuffer[i] = '0' + lValue % 10;
		lValue = lValue / 10;
	}

	pcBuffer[i] = '\0';

	
	// Reverse String in Buffer
	if(pcBuffer[0] == '-')
		kReverseString(&(pcBuffer[1]));
	else
		kReverseString(pcBuffer);

	return i;
}


// Reverse String Sequence
void kReverseString(char* pcBuffer)
{
	int iLength;
	int i;
	char cTemp;

	iLength = kStrLen(pcBuffer);

	for(i=0; i < iLength/2; i++)
	{
		cTemp = pcBuffer[i];
		pcBuffer[i] = pcBuffer[iLength - 1 - i];
		pcBuffer[iLength - 1 - i] = cTemp;
	}
}



int kSPrintf(char* pcBuffer, const char* pcFormatString, ... )
{
	va_list ap;
	int 	iReturn;
	
	va_start(ap, pcFormatString);
	iReturn = kVSPrintf(pcBuffer, pcFormatString, ap);
	va_end(ap);

	return iReturn;
}


int kVSPrintf(char* pcBuffer, const char* pcFormatString, va_list ap)	// for kSPrintf()
{
	QWORD i, j, k;
	int   iBufferIndex = 0;
	int   iFormatLength, iCopyLength;
	char* pcCopyString;
	QWORD qwValue;
	int   iValue;
	double dValue;

	
	// Get Length of Format String, Print Data to Output Buffer
	iFormatLength = kStrLen(pcFormatString);

	for(i=0; i<iFormatLength; i++)
	{
		// if '%', handle it as Data Type Character
		if(pcFormatString[i] == '%')
		{
			i++;
			
			switch(pcFormatString[i])
			{
				case 's':		// String
					pcCopyString = (char*)(va_arg(ap, char*));		// retrieve next argument
					iCopyLength  = kStrLen(pcCopyString);

					kMemCpy(pcBuffer + iBufferIndex, pcCopyString, iCopyLength);
					iBufferIndex += iCopyLength;
					break;

				case 'c':		// Character
					pcBuffer[iBufferIndex] = (char)(va_arg(ap, int));
					iBufferIndex++;
					break;

				case 'd':
				case 'i':		// Integer
					iValue = (int)(va_arg(ap, int));
					iBufferIndex += kIToA(iValue, pcBuffer + iBufferIndex, 10);
					break;

				case 'x':		
				case 'X':		// 4 Byte Hexadecimal
					qwValue = (DWORD)(va_arg(ap, DWORD)) & 0xFFFFFFFF;
					iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
					break;
				
				case 'q':
				case 'Q':
				case 'p':		// 8 Byte Hexadecimal
					qwValue = (QWORD)(va_arg(ap, QWORD));
					iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
					break;

				case 'f':
					dValue = (double)(va_arg(ap, double));

					// Round off at third digit
					dValue += 0.005;

					// Decimal Point Calculation 
					pcBuffer[iBufferIndex]   = '0' + (QWORD)(dValue * 100) % 10;
					pcBuffer[iBufferIndex+1] = '0' + (QWORD)(dValue * 10) % 10;
					pcBuffer[iBufferIndex+2] = '.';

					// Decimal Calculation
					for(k=0; ; k++)
					{
						if( ((QWORD)dValue == 0) && (k != 0) )
							break;

						pcBuffer[iBufferIndex + 3 + k] = '0' + ((QWORD)dValue % 10);
						dValue = dValue / 10;
					}

					pcBuffer[iBufferIndex + 3 + k] = '\0';
					
					kReverseString(pcBuffer + iBufferIndex);
					iBufferIndex += 3+k;
					break;

				default:		// if else, print String
					pcBuffer[iBufferIndex] = pcFormatString[i];
					iBufferIndex++;
					break;
			}
		}
		
		// String (not format)
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


QWORD kGetTickCount(void)
{
	return g_qwTickCount;
}


void kSleep(QWORD qwMilisecond)
{
	QWORD qwLastTickCount;

	qwLastTickCount = g_qwTickCount;

	while((g_qwTickCount - qwLastTickCount) <= qwMilisecond)
		kSchedule();
}



void kMemSetWord(void* pvDestination, WORD wData, int iWordSize)
{
	int i;
	QWORD qwData;
	int iRemainWordStartOffset;


	// fill WORD type data into 8 bytes
	qwData = 0;
	
	for(i=0; i<4; i++)
	{
		qwData = (qwData << 16) | wData;
	}


	// fill WORD * 4(qwData) at once
	for(i=0; i<(iWordSize / 4); i++)
	{
		((QWORD*)pvDestination)[i] = qwData;
	}


	// fill remaining part
	iRemainWordStartOffset = i * 4;

	for(i=0; i<(iWordSize % 4); i++)
	{
		((WORD*)pvDestination)[iRemainWordStartOffset++] = wData;
	}
}



BOOL kIsGraphicMode(void)
{
	if(*(BYTE*)VBE_STARTGRAPHICMODEFLAGADDRESS == 0)
	{
		return FALSE;
	}

	return TRUE;
}



