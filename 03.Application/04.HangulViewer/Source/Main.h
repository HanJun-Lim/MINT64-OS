#ifndef __MAIN_H__
#define __MAIN_H__


// ---------- Macro ----------

// Max printable line count

#define MAXLINECOUNT		(256 * 1024)

// Margin between Window area and Text area

#define MARGIN				5

// Tab space

#define TABSPACE			4



// ---------- Structure ----------

typedef struct TextInformationStruct
{
	// File buffer, File size
	BYTE* pbFileBuffer;
	DWORD dwFileSize;

	// Printable Row/Column count
	int iColumnCount;
	int iRowCount;

	// Buffer for file offset of line
	DWORD* pdwFileOffsetOfLine;

	// Max line count
	int iMaxLineCount;

	// Current line index
	int iCurrentLineIndex;

	// File name
	char vcFileName[100];

} TEXTINFO;



// ---------- Function ----------

BOOL ReadFileToBuffer(const char* pcFileName, TEXTINFO* pstInfo);
void CalculateFileOffsetOfLine(int iWidth, int iHeight, TEXTINFO* pstInfo);
BOOL DrawTextBuffer(QWORD qwWindowID, TEXTINFO* pstInfo);



#endif
