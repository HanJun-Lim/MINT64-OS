#ifndef __HANGULINPUT_H__
#define __HANGULINPUT_H__


// ---------- Macro ----------

// make upper-case to lower-case

#define TOLOWER(x)	( (('A' <= (x)) && ((x) <= 'Z')) ? ((x) - 'A' + 'a') : (x) )



// ---------- Structure ----------

// one entry of Hangul input table
typedef struct HangulInputTableItemStruct
{
	// Hangul
	char* pcHangul;

	// key input that corresponding with Hangul
	char* pcInput;
} HANGULINPUTITEM;


// one entry of index table of Hangul input table
typedef struct HangulIndexTableItemStruct
{
	// first charactor of ÇÑ±Û ³¹ÀÚ
	char cStartCharactor;

	// start index of hangul input table
	DWORD dwIndex;
} HANGULINDEXITEM;



// ---------- Function ----------

BOOL IsJaum(char cInput);
BOOL IsMoum(char cInput);
BOOL FindLongestHangulInTable(const char* pcInputBuffer, int iBufferCount, int* piMatchIndex, int* piMatchLength);
BOOL ComposeHangul(char* pcInputBuffer, 			  int* piInputBufferLength, 
				   char* pcOutputBufferForProcessing, char* pcOutputBufferForComplete);



#endif
