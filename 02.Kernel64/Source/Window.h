#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "Types.h"
#include "Synchronization.h"
#include "2DGraphics.h"
#include "List.h"
#include "Queue.h"
#include "Keyboard.h"


// --------------- Macro --------------- 

// Max Window Count
#define WINDOW_MAXCOUNT								2048


// Macro for Offset in Window Pool using Window ID, low 32 bit means Offset
#define GETWINDOWOFFSET(x)							( (x) & 0xFFFFFFFF )

// Max length of Window Title
#define WINDOW_TITLEMAXLENGTH						40


// Invalid Window ID
#define WINDOW_INVALIDID							0xFFFFFFFFFFFFFFFF


// *************** Window attribute *************** 

// show Window
#define WINDOW_FLAGS_SHOW							0x00000001


// draw Window Frame
#define WINDOW_FLAGS_DRAWFRAME						0x00000002


// draw Window Title Bar
#define WINDOW_FLAGS_DRAWTITLE						0x00000004


// draw Window Resize Button
#define WINDOW_FLAGS_RESIZABLE						0x00000008


// Window default attribute : draws Frame, Title and set Window to be shown
#define WINDOW_FLAGS_DEFAULT						( WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLE )


// Title Bar Height
#define WINDOW_TITLEBAR_HEIGHT						21


// Size of Close Button
#define WINDOW_XBUTTON_SIZE							19


// Window Minimum Width, Height
#define WINDOW_WIDTH_MIN							(WINDOW_XBUTTON_SIZE * 2 + 30)
#define WINDOW_HEIGHT_MIN							(WINDOW_TITLEBAR_HEIGHT + 30)


// Window Color 
#define WINDOW_COLOR_FRAME							RGB(109, 218,  22)
#define WINDOW_COLOR_BACKGROUND						RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARTEXT					RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARACTIVEBACKGROUND		RGB( 79, 204,  11)
#define WINDOW_COLOR_TITLEBARINACTIVEBACKGROUND		RGB( 55, 135,  11)
#define WINDOW_COLOR_TITLEBARBRIGHT1				RGB(183, 249, 171)
#define WINDOW_COLOR_TITLEBARBRIGHT2				RGB(150, 210, 140)
#define WINDOW_COLOR_TITLEBARUNDERLINE				RGB( 46,  59,  30)
#define WINDOW_COLOR_BUTTONBRIGHT					RGB(229, 229, 229)
#define WINDOW_COLOR_BUTTONDARK						RGB( 86,  86,  86)
#define WINDOW_COLOR_SYSTEMBACKGROUND				RGB(232, 255, 232)
#define WINDOW_COLOR_XBUTTONLINECOLOR				RGB( 71, 199,  21)


// Title of Background
#define WINDOW_BACKGROUNDWINDOWTITLE				"SYS_BACKGROUND"


// Cursor Width, Height
#define MOUSE_CURSOR_WIDTH							20
#define MOUSE_CURSOR_HEIGHT							20


// Color of Cursor Image
#define MOUSE_CURSOR_OUTERLINE						RGB(  0,   0,   0)
#define MOUSE_CURSOR_OUTER							RGB( 79, 204,  11)
#define MOUSE_CURSOR_INNER							RGB(232, 255, 232)


// *************** for Events *************** 

// Size of Event Queue
#define EVENTQUEUE_WINDOWMAXCOUNT						100
#define EVENTQUEUE_WINDOWMANAGERMAXCOUNT				WINDOW_MAXCOUNT


// *** Events between Window and Window Manager ***

// Mouse Event
#define EVENT_UNKNOWN									0
#define EVENT_MOUSE_MOVE								1
#define EVENT_MOUSE_LBUTTONDOWN							2
#define EVENT_MOUSE_LBUTTONUP							3
#define EVENT_MOUSE_RBUTTONDOWN							4
#define EVENT_MOUSE_RBUTTONUP							5
#define EVENT_MOUSE_MBUTTONDOWN							6
#define EVENT_MOUSE_MBUTTONUP							7

// Window Event
#define EVENT_WINDOW_SELECT								8
#define EVENT_WINDOW_DESELECT							9
#define EVENT_WINDOW_MOVE								10
#define EVENT_WINDOW_RESIZE								11
#define EVENT_WINDOW_CLOSE								12

// Key Event
#define EVENT_KEY_DOWN									13
#define EVENT_KEY_UP									14

// Screen Update Event
#define EVENT_WINDOWMANAGER_UPDATESCREENBYID			15
#define EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA	16
#define EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA	17


// Max count of previously updated Area
#define WINDOW_OVERLAPPEDAREALOGMAXCOUNT				20



// --------------- Structure --------------- 

// for Mouse Event
typedef struct kMouseEventStruct
{
	// Window ID
	QWORD qwWindowID;

	// X, Y coordinates of Mouse (Screen coordinate), Button status
	POINT stPoint;
	BYTE bButtonStatus;
} MOUSEEVENT;


// for Key Event
typedef struct kKeyEventStruct
{
	// Window ID
	QWORD qwWindowID;

	// ASCII Code, Scan Code of Key
	BYTE bASCIICode;
	BYTE bScanCode;

	// Key Flags
	BYTE bFlags;
} KEYEVENT;


// for Window Event
typedef struct kWindowEventStruct
{
	// Window ID
	QWORD qwWindowID;

	// Area Information
	RECT stArea;
} WINDOWEVENT;


typedef struct kEventStruct
{
	// Event Type
	QWORD qwType;

	// Union for Event Data area definition
	union
	{
		// for Mouse Event
		MOUSEEVENT stMouseEvent;

		// for Key Event
		KEYEVENT stKeyEvent;

		// for Window Event
		WINDOWEVENT stWindowEvent;

		// for User-defined Event
		QWORD vqwData[3];
	};
} EVENT;


// for Window Information
typedef struct kWindowStruct
{
	// current Window ID, Pointer of next data
	LISTLINK stLink;

	// for Synchronization
	MUTEX stLock;

	// Window Area info (Screen-based)
	RECT stArea;

	// Window Screen Buffer Address
	COLOR* pstWindowBuffer;

	// Task ID which Window belongs
	QWORD qwTaskID;

	// Window Attributes
	DWORD dwFlags;

	// Event Queue and its Buffer
	QUEUE stEventQueue;
	EVENT* pstEventBuffer;

	// Window Title
	char vcWindowTitle[WINDOW_TITLEMAXLENGTH + 1];
} WINDOW;


// for Window Pool Management
typedef struct kWindowPoolManagerStruct
{
	// for Synchronization
	MUTEX stLock;

	// Window Pool Information
	WINDOW* pstStartAddress;
	int iMaxCount;
	int iUseCount;

	// Window Allocation Count
	int iAllocatedCount;
} WINDOWPOOLMANAGER;


// for Screen Area Bitmap Information
typedef struct kDrawBitmapStruct
{
	// Screen Area to update
	RECT stArea;

	// Screen Area Bitmap Address
	BYTE* pbBitmap;
} DRAWBITMAP;


// for Window Management
typedef struct kWindowManagerStruct
{
	// for Synchronization
	MUTEX stLock;

	// Window List
	LIST stWindowList;

	// current X, Y pos of Mouse
	int iMouseX;
	int iMouseY;

	// Screen Area Information
	RECT stScreenArea;

	// Video Memory Address
	COLOR* pstVideoMemory;

	// Background Window ID
	QWORD qwBackgroundWindowID;

	// Event Queue and its Buffer
	QUEUE stEventQueue;
	EVENT* pstEventBuffer;

	// previous status of Mouse Button
	BYTE bPreviousButtonStatus;

	// ID of moving Window, Window Move Mode
	QWORD qwMovingWindowID;
	BOOL bWindowMoveMode;

	// Resize Info
	BOOL bWindowResizeMode;
	QWORD qwResizingWindowID;
	RECT stResizingWindowArea;

	// Address of Bitmap Buffer used for Screen Update
	BYTE* pbDrawBitmap;
} WINDOWMANAGER;



// --------------- Function --------------- 

// for Window Pool
static void kInitializeWindowPool(void);
static WINDOW* kAllocateWindow(void);
static void kFreeWindow(QWORD qwID);


// for Window, Window Manager
void kInitializeGUISystem(void);
WINDOWMANAGER* kGetWindowManager(void);
QWORD kGetBackgroundWindowID(void);
void kGetScreenArea(RECT* pstScreenArea);
QWORD kCreateWindow(int iX, int iY, int iWidth, int iHeight, DWORD dwFlags, const char* pcTitle);
BOOL kDeleteWindow(QWORD qwWindowID);
BOOL kDeleteAllWindowInTaskID(QWORD qwTaskID);
WINDOW* kGetWindow(QWORD qwWindowID);
WINDOW* kGetWindowWithWindowLock(QWORD qwWindowID);
BOOL kShowWindow(QWORD qwWindowID, BOOL bShow);
BOOL kRedrawWindowByArea(const RECT* pstArea, QWORD qwDrawWindowID);
static void kCopyWindowBufferToFrameBuffer(const WINDOW* pstWindow, DRAWBITMAP* pstDrawBitmap);

QWORD kFindWindowByPoint(int iX, int iY);
QWORD kFindWindowByTitle(const char* pcTitle);
BOOL kIsWindowExist(QWORD qwWindowID);
QWORD kGetTopWindowID(void);
BOOL kMoveWindowToTop(QWORD qwWindowID);
BOOL kIsInTitleBar(QWORD qwWindowID, int iX, int iY);
BOOL kIsInCloseButton(QWORD qwWindowID, int iX, int iY);
BOOL kMoveWindow(QWORD qwWindowID, int iX, int iY);
static BOOL kUpdateWindowTitle(QWORD qwWindowID, BOOL bSelectedTitle);
BOOL kResizeWindow(QWORD qwWindowID, int iX, int iY, int iWidth, int iHeight);


// for converting coordinate (Screen coord <-> Window coord)
BOOL kGetWindowArea(QWORD qwWindowID, RECT* pstArea);
BOOL kConvertPointScreenToClient(QWORD qwWindowID, const POINT* pstXY, POINT* pstXYInWindow);
BOOL kConvertPointClientToScreen(QWORD qwWindowID, const POINT* pstXY, POINT* pstXYInScreen);
BOOL kConvertRectScreenToClient(QWORD qwWindowID, const RECT* pstArea, RECT* pstAreaInWindow);
BOOL kConvertRectClientToScreen(QWORD qwWindowID, const RECT* pstArea, RECT* pstAreaInScreen);


// for updating screen (used by Task)
BOOL kUpdateScreenByID(QWORD qwWindowID);
BOOL kUpdateScreenByWindowArea(QWORD qwWindowID, const RECT* pstArea);
BOOL kUpdateScreenByScreenArea(const RECT* pstArea);


// for Event Queue
BOOL kSendEventToWindow(QWORD qwWindowID, const EVENT* pstEvent);
BOOL kReceiveEventFromWindowQueue(QWORD qwWindowID, EVENT* pstEvent);
BOOL kSendEventToWindowManager(const EVENT* pstEvent);
BOOL kReceiveEventFromWindowManagerQueue(EVENT* pstEvent);
BOOL kSetMouseEvent(QWORD qwWindowID, QWORD qwEventType, int iMouseX, int iMouseY, BYTE bButtonStatus, EVENT* pstEvent);
BOOL kSetWindowEvent(QWORD qwWindowID, QWORD qwEventType, EVENT* pstEvent);
void kSetKeyEvent(QWORD qwWindow, const KEYDATA* pstKeyData, EVENT* pstEvent);


// Drawing functions for a Window, others for Mouse Cursor
BOOL kDrawWindowFrame(QWORD qwWindowID);
BOOL kDrawWindowBackground(QWORD qwWindowID);
BOOL kDrawWindowTitle(QWORD qwWindowID, const char* pcTitle, BOOL bSelectedTitle);
BOOL kDrawButton(QWORD qwWindowID, RECT* pstButtonArea, COLOR stBackgroundColor, const char* pcText, COLOR stTextColor);
BOOL kDrawPixel(QWORD qwWindowID, int iX, int iY, COLOR stColor);
BOOL kDrawLine(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor);
BOOL kDrawRect(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor, BOOL bFill);
BOOL kDrawCircle(QWORD qwWindowID, int iX, int iY, int iRadius, COLOR stColor, BOOL bFill);
BOOL kDrawText(QWORD qwWindowID, int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor, const char* pcString, int iLength);
static void kDrawCursor(int iX, int iY);
void kMoveCursor(int iX, int iY);
void kGetCursorPosition(int* piX, int* piY);
BOOL kBitBlt(QWORD qwWindowID, int iX, int iY, COLOR* pstBuffer, int iWidth, int iHeight);
void kDrawBackgroundImage(void);


// for Bitmap used for Screen Update
BOOL kCreateDrawBitmap(const RECT* pstArea, DRAWBITMAP* pstDrawBitmap);
static BOOL kFillDrawBitmap(DRAWBITMAP* pstDrawBitmap, RECT* pstArea, BOOL bFill);
inline BOOL kGetStartPositionInDrawBitmap(const DRAWBITMAP* pstDrawBitmap, int iX, int iY, int* piByteOffset, int* piBitOffset);
inline BOOL kIsDrawBitmapAllOff(const DRAWBITMAP* pstDrawBitmap);



#endif
