#include "SystemCallLibrary.h"


// =============================
// for Console I/O
// =============================

// print string to Console
// 		used in printf()
// 		returns next position to print
int ConsolePrintString(const char* pcBuffer)
{
	PARAMETERTABLE stParameter;


	// insert Parameter
	PARAM(0) = (QWORD)pcBuffer;


	// System Call
	return ExecuteSystemCall(SYSCALL_CONSOLEPRINTSTRING, &stParameter);
}


// set Cursor position
BOOL SetCursor(int iX, int iY)
{
	PARAMETERTABLE stParameter;


	// insert Parameter
	PARAM(0) = (QWORD)iX;
	PARAM(1) = (QWORD)iY;


	// System Call
	return ExecuteSystemCall(SYSCALL_SETCURSOR, &stParameter);
}


// get current Cursor position
BOOL GetCursor(int* piX, int* piY)
{
	PARAMETERTABLE stParameter;


	// insert Parameter
	PARAM(0) = (QWORD)piX;
	PARAM(1) = (QWORD)piY;


	// System Call
	return ExecuteSystemCall(SYSCALL_GETCURSOR, &stParameter);
}


BOOL ClearScreen(void)
{
	// System Call
	return ExecuteSystemCall(SYSCALL_CLEARSCREEN, NULL);
}


BYTE getch(void)
{
	// System Call
	return ExecuteSystemCall(SYSCALL_GETCH, NULL);
}



// =============================
// for Dynamic Memory
// =============================

void* malloc(QWORD qwSize)
{
	PARAMETERTABLE stParameter;


	// insert Parameter
	PARAM(0) = qwSize;


	// System Call
	return (void*)ExecuteSystemCall(SYSCALL_MALLOC, &stParameter);
}


BOOL free(void* pvAddress)
{
	PARAMETERTABLE stParameter;


	// insert Parameter
	PARAM(0) = (QWORD)pvAddress;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_FREE, &stParameter);
}



// =============================
// for File, Directory I/O
// =============================

// open/create a File
FILE* fopen(const char* pcFileName, const char* pcMode)
{
	PARAMETERTABLE stParameter;


	// insert Parameter
	PARAM(0) = (QWORD)pcFileName;
	PARAM(1) = (QWORD)pcMode;

	
	// System Call
	return (FILE*)ExecuteSystemCall(SYSCALL_FOPEN, &stParameter);
}


// copy read File to buffer
DWORD fread(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pvBuffer;
	PARAM(1) = (QWORD)dwSize;
	PARAM(2) = (QWORD)dwCount;
	PARAM(3) = (QWORD)pstFile;


	// System Call
	return ExecuteSystemCall(SYSCALL_FREAD, &stParameter);
}


// write buffer contents to target File
DWORD fwrite(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pvBuffer;
	PARAM(1) = (QWORD)dwSize;
	PARAM(2) = (QWORD)dwCount;
	PARAM(3) = (QWORD)pstFile;


	// System Call
	return ExecuteSystemCall(SYSCALL_FWRITE, &stParameter);
}


// move File Pointer position
int fseek(FILE* pstFile, int iOffset, int iOrigin)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pstFile;
	PARAM(1) = (QWORD)iOffset;
	PARAM(2) = (QWORD)iOrigin;


	// System Call
	return (int)ExecuteSystemCall(SYSCALL_FSEEK, &stParameter);
}


// close opened File
int fclose(FILE* pstFile)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pstFile;


	// System Call
	return (int)ExecuteSystemCall(SYSCALL_FCLOSE, &stParameter);
}


// remove target File
int remove(const char* pcFileName)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pcFileName;


	// System Call
	return (int)ExecuteSystemCall(SYSCALL_REMOVE, &stParameter);
}


// open the Directory
DIR* opendir(const char* pcDirectoryName)
{
	PARAMETERTABLE stParameter;


	// insert Parameter
	PARAM(0) = (QWORD)pcDirectoryName;


	// System Call
	return (DIR*)ExecuteSystemCall(SYSCALL_OPENDIR, &stParameter);
}


// get a Directory entry and move to next one
struct dirent* readdir(DIR* pstDirectory)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pstDirectory;


	// System Call
	return (struct dirent*)ExecuteSystemCall(SYSCALL_READDIR, &stParameter);
}


// move Directory Pointer to the first
BOOL rewinddir(DIR* pstDirectory)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pstDirectory;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_REWINDDIR, &stParameter);
}


// close opened Directory
int closedir(DIR* pstDirectory)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pstDirectory;


	// System Call
	return (int)ExecuteSystemCall(SYSCALL_CLOSEDIR, &stParameter);
}


// check if target File opened by checking Handle Pool
BOOL IsFileOpened(const struct dirent* pstEntry)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pstEntry;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_ISFILEOPENED, &stParameter);
}



// =============================
// for Hard Disk I/O
// =============================

// read HDD Sector
//		can read 256 sector max at once
//		returns read sector count
int ReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer)
{
	PARAMETERTABLE stParameter;


	// insert Parameter
	PARAM(0) = (QWORD)bPrimary;
	PARAM(1) = (QWORD)bMaster;
	PARAM(2) = (QWORD)dwLBA;
	PARAM(3) = (QWORD)iSectorCount;
	PARAM(4) = (QWORD)pcBuffer;


	// System Call
	return (int)ExecuteSystemCall(SYSCALL_READHDDSECTOR, &stParameter);
}


// write to HDD Sector
// 		can write 256 sector max at once
// 		returns written sector count
int WriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)bPrimary;
	PARAM(1) = (QWORD)bMaster;
	PARAM(2) = (QWORD)dwLBA;
	PARAM(3) = (QWORD)iSectorCount;
	PARAM(4) = (QWORD)pcBuffer;


	// System Call
	return (int)ExecuteSystemCall(SYSCALL_WRITEHDDSECTOR, &stParameter);
}



// =============================
// for Task, Scheduler
// =============================

QWORD CreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress, BYTE bAffinity)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)qwFlags;
	PARAM(1) = (QWORD)pvMemoryAddress;
	PARAM(2) = (QWORD)qwMemorySize;
	PARAM(3) = (QWORD)qwEntryPointAddress;
	PARAM(4) = (QWORD)bAffinity;


	// System Call
	return ExecuteSystemCall(SYSCALL_CREATETASK, &stParameter);
}


// switch to other process
BOOL Schedule(void)
{
	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_SCHEDULE, NULL);
}


// change Task Priority
BOOL ChangePriority(QWORD qwID, BYTE bPriority, BOOL bExecutedInInterrupt)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwID;
	PARAM(1) = (QWORD)bPriority;
	PARAM(2) = (QWORD)bExecutedInInterrupt;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_CHANGEPRIORITY, &stParameter);	
}


// end target Task
BOOL EndTask(QWORD qwTaskID)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwTaskID;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_ENDTASK, &stParameter);
}


// end Task itself
void exit(int iExitValue)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)iExitValue;


	// System Call
	ExecuteSystemCall(SYSCALL_EXITTASK, &stParameter);
}


// get current Task count
int GetTaskCount(BYTE bAPICID)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)bAPICID;


	// System Call
	return (int)ExecuteSystemCall(SYSCALL_GETTASKCOUNT, &stParameter);
}


// is target Task exist?
BOOL IsTaskExist(QWORD qwID)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwID;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_ISTASKEXIST, &stParameter);
}


// get Processor load percentage
QWORD GetProcessorLoad(BYTE bAPICID)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)bAPICID;


	// System Call
	return ExecuteSystemCall(SYSCALL_GETPROCESSORLOAD, &stParameter);
}


// change Processor affinity
BOOL ChangeProcessorAffinity(QWORD qwTaskID, BYTE bAffinity)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwTaskID;
	PARAM(1) = (QWORD)bAffinity;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_CHANGEPROCESSORAFFINITY, &stParameter);
}


// create Application Program
QWORD ExecuteProgram(const char* pcFileName, const char* pcArgumentString, BYTE bAffinity)
{
	PARAMETERTABLE stParameter;


	// insert Parameter
	PARAM(0) = (QWORD)pcFileName;
	PARAM(1) = (QWORD)pcArgumentString;
	PARAM(2) = (QWORD)bAffinity;


	// System Call
	return ExecuteSystemCall(SYSCALL_EXECUTEPROGRAM, &stParameter);
}


// create Thread
QWORD CreateThread(QWORD qwEntryPoint, QWORD qwArgument, BYTE bAffinity)
{
	PARAMETERTABLE stParameter;


	// insert Parameter
	PARAM(0) = (QWORD)qwEntryPoint;
	PARAM(1) = (QWORD)qwArgument;
	PARAM(2) = (QWORD)bAffinity;
	
	// specify exit() on the function called when exiting, so that the thread terminates itself
	PARAM(3) = (QWORD)exit;


	// System Call
	return ExecuteSystemCall(SYSCALL_EXECUTEPROGRAM, &stParameter);
}



// =============================
// for GUI System
// =============================

// get Background Window ID
QWORD GetBackgroundWindowID(void)
{
	// System Call
	return ExecuteSystemCall(SYSCALL_GETBACKGROUNDWINDOWID, NULL);
}


// get Screen area
void GetScreenArea(RECT* pstScreenArea)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pstScreenArea;


	// System Call
	ExecuteSystemCall(SYSCALL_GETSCREENAREA, &stParameter);
}


// create a Window
QWORD CreateWindow(int iX, int iY, int iWidth, int iHeight, DWORD dwFlags, const char* pcTitle)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)iX;
	PARAM(1) = (QWORD)iY;
	PARAM(2) = (QWORD)iWidth;
	PARAM(3) = (QWORD)iHeight;
	PARAM(4) = (QWORD)dwFlags;
	PARAM(5) = (QWORD)pcTitle;


	// System Call
	return ExecuteSystemCall(SYSCALL_CREATEWINDOW, &stParameter);
}


// delete target Window
BOOL DeleteWindow(QWORD qwWindowID)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_DELETEWINDOW, &stParameter);
}


// hide/show Window
BOOL ShowWindow(QWORD qwWindowID, BOOL bShow)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)bShow;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_SHOWWINDOW, &stParameter);
}


// find the top Window at (iX, iY)
QWORD FindWindowByPoint(int iX, int iY)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)iX;
	PARAM(1) = (QWORD)iY;


	// System Call
	return ExecuteSystemCall(SYSCALL_FINDWINDOWBYPOINT, &stParameter);
}


// find the Window by Title
QWORD FindWindowByTitle(const char* pcTitle)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pcTitle;


	// System Call
	return ExecuteSystemCall(SYSCALL_FINDWINDOWBYTITLE, &stParameter);
}


// is target Window exist?
BOOL IsWindowExist(QWORD qwWindowID)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_ISWINDOWEXIST, &stParameter);
}


// get Top Window (activated Window) ID
QWORD GetTopWindowID(void)
{
	// System Call
	return ExecuteSystemCall(SYSCALL_GETTOPWINDOWID, NULL);
}


// move target Window to top of Z-order
BOOL MoveWindowToTop(QWORD qwWindowID)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_MOVEWINDOWTOTOP, &stParameter);
}


// is Mouse (X,Y) in Title Bar of target Window?
BOOL IsInTitleBar(QWORD qwWindowID, int iX, int iY)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)iX;
	PARAM(2) = (QWORD)iY;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_ISINTITLEBAR, &stParameter);
}


// is Mouse (X,Y) in Close Button of target Window?
BOOL IsInCloseButton(QWORD qwWindowID, int iX, int iY)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)iX;
	PARAM(2) = (QWORD)iY;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_ISINCLOSEBUTTON, &stParameter);
}


// move the Window to (iX, iY)
BOOL MoveWindow(QWORD qwWindowID, int iX, int iY)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)iX;
	PARAM(2) = (QWORD)iY;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_MOVEWINDOW, &stParameter);
}


// resize target Window
BOOL ResizeWindow(QWORD qwWindowID, int iX, int iY, int iWidth, int iHeight)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)iX;
	PARAM(2) = (QWORD)iY;
	PARAM(3) = (QWORD)iWidth;
	PARAM(4) = (QWORD)iHeight;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_RESIZEWINDOW, &stParameter);
}


// get target Window area
BOOL GetWindowArea(QWORD qwWindowID, RECT* pstArea)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)pstArea;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_GETWINDOWAREA, &stParameter);
}


// update Window to Screen
// 		used by Task
BOOL UpdateScreenByID(QWORD qwWindowID)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_UPDATESCREENBYID, &stParameter);
}


// update Window to Screen
// 		used by Task
BOOL UpdateScreenByWindowArea(QWORD qwWindowID, const RECT* pstArea)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)pstArea;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_UPDATESCREENBYWINDOWAREA, &stParameter);
}


// update Window to Screen
// 		used by Task
BOOL UpdateScreenByScreenArea(const RECT* pstArea)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pstArea;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_UPDATESCREENBYSCREENAREA, &stParameter);
}


// send the Event to target Window
BOOL SendEventToWindow(QWORD qwWindowID, const EVENT* pstEvent)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)pstEvent;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_SENDEVENTTOWINDOW, &stParameter);
}


// receive the Event from Window Event Queue of target Window
BOOL ReceiveEventFromWindowQueue(QWORD qwWindowID, EVENT* pstEvent)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)pstEvent;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_RECEIVEEVENTFROMWINDOWQUEUE, &stParameter);
}


// draw Window Frame to target Window's buffer
BOOL DrawWindowFrame(QWORD qwWindowID)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_DRAWWINDOWFRAME, &stParameter);
}


// draw Background to target Window's buffer
BOOL DrawWindowBackground(QWORD qwWindowID)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_DRAWWINDOWBACKGROUND, &stParameter);
}


// draw Title Bar to target Window's buffer
BOOL DrawWindowTitle(QWORD qwWindowID, const char* pcTitle, BOOL bSelectedTitle)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)pcTitle;
	PARAM(2) = (QWORD)bSelectedTitle;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_DRAWWINDOWTITLE, &stParameter);
}


// draw Button to target Window's buffer
BOOL DrawButton(QWORD qwWindowID, RECT* pstButtonArea, COLOR stBackgroundColor, const char* pcText, COLOR stTextColor)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)pstButtonArea;
	PARAM(2) = (QWORD)stBackgroundColor;
	PARAM(3) = (QWORD)pcText;
	PARAM(4) = (QWORD)stTextColor;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_DRAWBUTTON, &stParameter);
}


// draw a Pixel to target Window's buffer
BOOL DrawPixel(QWORD qwWindowID, int iX, int iY, COLOR stColor)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)iX;
	PARAM(2) = (QWORD)iY;
	PARAM(3) = (QWORD)stColor;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_DRAWPIXEL, &stParameter);
}


// draw a Line to target Window's buffer
BOOL DrawLine(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)iX1;
	PARAM(2) = (QWORD)iY1;
	PARAM(3) = (QWORD)iX2;
	PARAM(4) = (QWORD)iY2;
	PARAM(5) = (QWORD)stColor;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_DRAWLINE, &stParameter);
}


// draw a Rectangle to target Window's buffer
BOOL DrawRect(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor, BOOL bFill)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)iX1;
	PARAM(2) = (QWORD)iY1;
	PARAM(3) = (QWORD)iX2;
	PARAM(4) = (QWORD)iY2;
	PARAM(5) = (QWORD)stColor;
	PARAM(6) = (QWORD)bFill;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_DRAWRECT, &stParameter);
}


// draw a Circle to target Window's buffer
BOOL DrawCircle(QWORD qwWindowID, int iX, int iY, int iRadius, COLOR stColor, BOOL bFill)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)iX;
	PARAM(2) = (QWORD)iY;
	PARAM(3) = (QWORD)iRadius;
	PARAM(4) = (QWORD)stColor;
	PARAM(5) = (QWORD)bFill;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_DRAWCIRCLE, &stParameter);
}


// draw Text to target Window's buffer
BOOL DrawText(QWORD qwWindowID, int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor, const char* pcString, int iLength)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)iX;
	PARAM(2) = (QWORD)iY;
	PARAM(3) = (QWORD)stTextColor;
	PARAM(4) = (QWORD)stBackgroundColor;
	PARAM(5) = (QWORD)pcString;
	PARAM(6) = (QWORD)iLength;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_DRAWTEXT, &stParameter);
}


// move Mouse Cursor to (iX, iY) and redraw
void MoveCursor(int iX, int iY)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)iX;
	PARAM(1) = (QWORD)iY;


	// System Call
	ExecuteSystemCall(SYSCALL_MOVECURSOR, &stParameter);
}


// get current Cursor position
void GetCursorPosition(int* piX, int* piY)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)piX;
	PARAM(1) = (QWORD)piY;


	// System Call
	ExecuteSystemCall(SYSCALL_GETCURSORPOSITION, &stParameter);
}


// send bit block to Window buffer (Bit Block Transfer)
// 		(iX, iY) is Window-based coordinate
BOOL BitBlt(QWORD qwWindowID, int iX, int iY, COLOR* pstBuffer, int iWidth, int iHeight)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwWindowID;
	PARAM(1) = (QWORD)iX;
	PARAM(2) = (QWORD)iY;
	PARAM(3) = (QWORD)pstBuffer;
	PARAM(4) = (QWORD)iWidth;
	PARAM(5) = (QWORD)iHeight;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_BITBLT, &stParameter);
}



// =============================
// for JPEG Module
// =============================

// initialize JPEG Data Structure using JPEG Image File buffer and its size
// 		insert Image size, etc info by analyzing Image File buffer
BOOL JPEGInit(JPEG* jpeg, BYTE* pbFileBuffer, DWORD dwFileSize)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)jpeg;
	PARAM(1) = (QWORD)pbFileBuffer;
	PARAM(2) = (QWORD)dwFileSize;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_JPEGINIT, &stParameter);
}


// decode the Image referring info of JPEG Data Structure, and save decoded result into buffer
BOOL JPEGDecode(JPEG* jpeg, COLOR* rgb)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)jpeg;
	PARAM(1) = (QWORD)rgb;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_JPEGDECODE, &stParameter);
}



// =============================
// for RTC
// =============================

// read current time from RTC Controller in CMOS Memory
BOOL ReadRTCTime(BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pbHour;
	PARAM(1) = (QWORD)pbMinute;
	PARAM(2) = (QWORD)pbSecond;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_READRTCTIME, &stParameter);

}


// read current date from RTC Controller in CMOS Memory
BOOL ReadRTCDate(WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, BYTE* pbDayOfWeek)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pwYear;
	PARAM(1) = (QWORD)pbMonth;
	PARAM(2) = (QWORD)pbDayOfMonth;
	PARAM(3) = (QWORD)pbDayOfWeek;


	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_READRTCDATE, &stParameter);
}



// =============================
// for Serial Communication
// =============================

// send data to Serial Port
void SendSerialData(BYTE* pbBuffer, int iSize)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pbBuffer;
	PARAM(1) = (QWORD)iSize;


	// System Call
	ExecuteSystemCall(SYSCALL_SENDSERIALDATA, &stParameter);
}


// receive data from Serial Port
int ReceiveSerialData(BYTE* pbBuffer, int iSize)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = (QWORD)pbBuffer;
	PARAM(1) = (QWORD)iSize;


	// System Call
	return (int)ExecuteSystemCall(SYSCALL_RECEIVESERIALDATA, &stParameter);
}


// initialize Serial Port Controller FIFO
void ClearSerialFIFO(void)
{
	// System Call
	ExecuteSystemCall(SYSCALL_CLEARSERIALFIFO, NULL);
}



// =============================
// etc
// =============================

// get total RAM size
QWORD GetTotalRAMSize(void)
{
	// System Call
	return ExecuteSystemCall(SYSCALL_GETTOTALRAMSIZE, NULL);
}


// get tick count
QWORD GetTickCount(void)
{
	// System Call
	return ExecuteSystemCall(SYSCALL_GETTICKCOUNT, NULL);
}


// wait for qwMillisecond
void Sleep(QWORD qwMillisecond)
{
	PARAMETERTABLE stParameter;

	
	// insert Parameter
	PARAM(0) = qwMillisecond;


	// System Call
	ExecuteSystemCall(SYSCALL_SLEEP, &stParameter);
}


// is now graphic mode?
BOOL IsGraphicMode(void)
{
	// System Call
	return (BOOL)ExecuteSystemCall(SYSCALL_ISGRAPHICMODE, NULL);
}



