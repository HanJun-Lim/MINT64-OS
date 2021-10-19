#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdarg.h>
#include "Types.h"


// Macro

#define MIN(x, y)	( ((x) < (y)) ? (x) : (y) )
#define MAX(x, y)	( ((x) > (y)) ? (x) : (y) )



// functions

void kMemSet(void* pvDestination, BYTE bData, int iSize);
int	 kMemCpy(void* pvDestination, const void* pvSource, int iSize);
int	 kMemCmp(const void* pvDestination, const void* pvSource, int iSize);

BOOL kSetInterruptFlag(BOOL bEnableInterrupt);

void  kCheckTotalRAMSize(void);
QWORD kGetTotalRAMSize(void);
void  kReverseString(char* pcBuffer);
long  kAToI(const char* pcBuffer, int iRadix);
QWORD kHexStringToQword(const char* pcBuffer);			// for kAToI()
long  kDecimalStringToLong(const char* pcBuffer);		// for kAToI()
int   kIToA(long lValue, char* pcbuffer, int iRadix);
int   kHexToString(QWORD qwValue, char* pcBuffer);		// for kIToA()
int   kDecimalToString(long lValue, char* pcBuffer);	// for kIToA()
int   kSPrintf(char* pcBuffer, const char* pcFormatString, ... );
int   kVSPrintf(char* pcBuffer, const char* pcFormatString, va_list ap);	// for kPrintf(), kSPrintf()

QWORD kGetTickCount(void);

void kSleep(QWORD qwMilisecond);

inline void kMemSetWord(void* pvDestination, WORD wData, int iWordSize);

BOOL kIsGraphicMode(void);



// etc

extern volatile QWORD g_qwTickCount;



#endif
