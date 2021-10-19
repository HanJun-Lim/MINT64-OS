#ifndef __MAIN_H__
#define __MAIN_H__


// ---------- Macro ----------

// Max byte length of printable letter
#define MAXOUTPUTLENGTH		60



// ---------- Structure ----------

typedef struct BufferManagerStruct
{
	// ========== for composing hangul ==========
	
	// store key input to compose hangul
	char vcInputBuffer[20];
	int iInputBufferLength;

	// store composing/composed hangul
	char vcOutputBufferForProcessing[3];		// NULL included
	char vcOutputBufferForComplete[3];			// NULL included

	
	// ========== for printing output buffer ==========
	
	// output buffers to print
	char vcOutputBuffer[MAXOUTPUTLENGTH];
	int iOutputBufferLength;

} BUFFERMANAGER;



// ---------- Function ----------

void ConvertJaumMoumToLowerCharactor(BYTE* pbInput);



#endif
