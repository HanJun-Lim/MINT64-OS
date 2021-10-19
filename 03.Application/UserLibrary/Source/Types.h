#ifndef __TYPES_H__
#define __TYPES_H__


// ---------- Macro ----------

// ======================================
// basic types
// ======================================

#define BYTE	unsigned char
#define WORD	unsigned short
#define DWORD	unsigned int
#define QWORD	unsigned long
#define BOOL	unsigned char

#define TRUE	1
#define FALSE	0
#define NULL	0



// ======================================
// for Console
// ======================================

// Console Width, Height

#define CONSOLE_WIDTH		80
#define CONSOLE_HEIGHT		25



// ======================================
// for Keyboard
// ======================================

// Key Status Flags

#define KEY_FLAGS_UP			0x00
#define KEY_FLAGS_DOWN			0x01
#define KEY_FLAGS_EXTENDEDKEY	0x02

// Scan Code Mapping Table (ASCII)
// 		some Function Key (Home, F1, etc...) have no allocated ASCII Code
// 		MINT64 OS allocates ASCII value upper 0x80 (extended ASCII) for them

#define KEY_NONE				0x00
#define KEY_ENTER				'\n'
#define KEY_TAB					'\t'
#define KEY_ESC					0x1B
#define KEY_BACKSPACE			0x08

#define KEY_CTRL				0x81
#define KEY_LSHIFT				0x82
#define KEY_RSHIFT				0x83
#define KEY_PRINTSCREEN			0x84
#define KEY_LALT				0x85
#define KEY_CAPSLOCK			0x86
#define KEY_F1					0x87
#define KEY_F2					0x88
#define KEY_F3					0x89
#define KEY_F4					0x8A
#define KEY_F5					0x8B
#define KEY_F6					0x8C
#define KEY_F7					0x8D
#define KEY_F8					0x8E
#define KEY_F9					0x8F
#define KEY_F10					0x90
#define KEY_NUMLOCK				0x91
#define KEY_SCROLLLOCK			0x92
#define KEY_HOME				0x93
#define KEY_UP					0x94
#define KEY_PAGEUP				0x95
#define KEY_LEFT				0x96
#define KEY_CENTER				0x97
#define KEY_RIGHT				0x98
#define KEY_END					0x99
#define KEY_DOWN				0x9A
#define KEY_PAGEDOWN			0x9B
#define KEY_INS					0x9C
#define KEY_DEL					0x9D
#define KEY_F11					0x9E
#define KEY_F12					0x9F
#define KEY_PAUSE				0xA0



// ======================================
// for Task, Scheduler
// ======================================

// Task Priority

#define TASK_FLAGS_HIGHEST			0
#define TASK_FLAGS_HIGH				1
#define TASK_FLAGS_MEDIUM			2
#define TASK_FLAGS_LOW				3
#define TASK_FLAGS_LOWEST			4
#define TASK_FLAGS_WAIT				0xFF

// Task Flags

#define TASK_FLAGS_ENDTASK			0x8000000000000000
#define TASK_FLAGS_SYSTEM			0x4000000000000000
#define TASK_FLAGS_PROCESS			0x2000000000000000
#define TASK_FLAGS_THREAD			0x1000000000000000
#define TASK_FLAGS_IDLE				0x0800000000000000
#define TASK_FLAGS_USERLEVEL		0x0400000000000000

// Function Macro

#define GETPRIORITY(x)				( (x) & 0xFF )											// x : Flags
#define SETPRIORITY(x, priority)	( (x) = ((x) & 0xFFFFFFFFFFFFFF00) | (priority) )		// x : Flags
#define GETTCBOFFSET(x)				( (x) & 0xFFFFFFFF )									// x : Task ID

// Invalid Task ID

#define TASK_INVALIDID				0xFFFFFFFFFFFFFFFF

// Task Affinity ID for Load Balancing

#define TASK_LOADBALANCINGID		0xFF



// ======================================
// for File System
// ======================================

// Max File Name Length

#define FILESYSTEM_MAXFILENAMELENGTH		24

// Seek Options

#define SEEK_SET		0
#define SEEK_CUR		1
#define SEEK_END		2

// Redefine MINT File System types as standard I/O type

#define size_t			DWORD
#define dirent			DirectoryEntryStruct
#define d_name			vcFileName



// ======================================
// for GUI System
// ======================================

// store Color information (use 16-bit RGB565)
typedef WORD			COLOR;

// convert 0~255 RGB info to 16-bit RGB565
#define RGB(r, g, b)	( ((BYTE)(r) >> 3) << 11 | ((BYTE)(g) >> 2) << 5 | ((BYTE)(b) >> 3) )

// Window Title Max Length
#define WINDOW_TITLEMAXLENGTH		40

// Invalid Window ID
#define WINDOW_INVALIDID			0xFFFFFFFFFFFFFFFF


// ********** Window Attribute **********

// show Window to Screen
#define WINDOW_FLAGS_SHOW					0x00000001

// draw Window Frame
#define WINDOW_FLAGS_DRAWFRAME				0x00000002

// draw Window Title Bar
#define WINDOW_FLAGS_DRAWTITLE				0x00000004

// draw Window Resize Button
#define WINDOW_FLAGS_RESIZABLE				0x00000008

// default Window Attribute - draw Title Bar, Frame, show Window
#define WINDOW_FLAGS_DEFAULT				(WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLE)

// Title Bar height
#define WINDOW_TITLEBAR_HEIGHT				21

// Close Button size
#define WINDOW_XBUTTON_SIZE					19

// Minimum Window Width - 30 Pixel + 2 Close Button size
#define WINDOW_WIDTH_MIN					(WINDOW_XBUTTON_SIZE * 2 + 30)

// Minimum Window Height - 30 Pixel + Title Bar height
#define WINDOW_HEIGHT_MIN					(WINDOW_TITLEBAR_HEIGHT + 30)

// Window Color
#define WINDOW_COLOR_FRAME						RGB(109, 218,  22)
#define WINDOW_COLOR_BACKGROUND					RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARTEXT				RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARACTIVEBACKGROUND	RGB( 79, 204,  11)
#define WINDOW_COLOR_TITLEBARINACTIVEBACKGROUND	RGB( 55, 135,  11)
#define WINDOW_COLOR_TITLEBARBRIGHT1			RGB(183, 249, 171)
#define WINDOW_COLOR_TITLEBARBRIGHT2			RGB(150, 210, 140)
#define WINDOW_COLOR_TITLEBARUNDERLINE			RGB( 46,  59,  30)
#define WINDOW_COLOR_BUTTONBRIGHT				RGB(229, 229, 229)
#define WINDOW_COLOR_BUTTONDARK					RGB( 86,  86,  86)
#define WINDOW_COLOR_SYSTEMBACKGROUND			RGB(232, 255, 232)
#define WINDOW_COLOR_XBUTTONLINECOLOR			RGB( 71, 199,  21)

// Background Window Title
#define WINDOW_BACKGROUNDWINDOWTITLE			"SYS_BACKGROUND"


// ********** Events between Window, Window Manager Task **********

// Mouse Event
#define EVENT_UNKNOWN					0
#define EVENT_MOUSE_MOVE				1
#define EVENT_MOUSE_LBUTTONDOWN			2
#define EVENT_MOUSE_LBUTTONUP			3
#define EVENT_MOUSE_RBUTTONDOWN			4
#define EVENT_MOUSE_RBUTTONUP			5
#define EVENT_MOUSE_MBUTTONDOWN			6
#define EVENT_MOUSE_MBUTTONUP			7

// Window Event
#define EVENT_WINDOW_SELECT				8
#define EVENT_WINDOW_DESELECT			9
#define EVENT_WINDOW_MOVE				10
#define EVENT_WINDOW_RESIZE				11
#define EVENT_WINDOW_CLOSE				12

// Key Event
#define EVENT_KEY_DOWN					13
#define EVENT_KEY_UP					14


// English Font width, height
#define FONT_ENGLISHWIDTH			8
#define FONT_ENGLISHHEIGHT			16



// ======================================
// etc
// ======================================

#define MIN(x, y)		( ((x) < (y)) ? (x) : (y) )
#define MAX(x, y)		( ((x) > (y)) ? (x) : (y) )



// ---------- Structure ----------

#pragma pack(push, 1)


// ======================================
// for Keyboard
// ======================================

// inserted into Key Queue
typedef struct KeyDataStruct
{
	// Scan Code
	BYTE bScanCode;

	// ASCII Code (converted Scan Code)
	BYTE bASCIICode;

	// Key Status Flag
	BYTE bFlags;
} KEYDATA;



// ======================================
// for File System
// ======================================

// Directory Entry
typedef struct DirectoryEntryStruct
{
	// File Name
	char vcFileName[FILESYSTEM_MAXFILENAMELENGTH];

	// File Size
	DWORD dwFileSize;

	// Start Cluster Index of the File
	DWORD dwStartClusterIndex;
} DIRECTORYENTRY;

#pragma pack(pop)


// File Handle (Manager)
typedef struct kFileHandleStruct
{
	// Directory Entry Offset that File exists
	int iDirectoryEntryOffset;

	// File Size
	DWORD dwFileSize;
	
	// Start Cluster Index
	DWORD dwStartClusterIndex;

	// Current I/O Running Cluster Index
	DWORD dwCurrentClusterIndex;

	// Previous Cluster Index
	DWORD dwPreviousClusterIndex;

	// Current File Pointer Offset
	DWORD dwCurrentOffset;
} FILEHANDLE;


// Directory Handle (Manager)
typedef struct kDirectoryHandleStruct
{
	// Root Directory Buffer
	DIRECTORYENTRY* pstDirectoryBuffer;

	// current Directory Pointer Offset
	int iCurrentOffset;
} DIRECTORYHANDLE;


typedef struct kFileDirectoryHandleStruct
{
	// Data Structure Type : File, Directory, Empty
	BYTE bType;

	// Handle Type defined by bType field
	union
	{
		// File Handle
		FILEHANDLE stFileHandle;

		// Directory Handle
		DIRECTORYHANDLE stDirectoryHandle;
	};
} FILE, DIR;



// ======================================
// for GUI System
// ======================================

// Rectangle information
typedef struct kRectangleStruct
{
	// Left Top X (Start Point)
	int iX1;

	// Left Top Y (Start Point)
	int iY1;

	// Right Bottom X (End Point)
	int iX2;

	// Right Bottom Y (End Point)
	int iY2;
} RECT;


// Point information
typedef struct kPointStruct
{
	// (X, Y)
	int iX;
	int iY;
} POINT;


// Mouse Event
typedef struct kMouseEventStruct
{
	// Window ID
	QWORD qwWindowID;

	// Mouse coordinate, Button status
	POINT stPoint;
	BYTE bButtonStatus;
} MOUSEEVENT;


// Key Event 
typedef struct kKeyEventStruct
{
	// Window ID
	QWORD qwWindowID;

	// ASCII Code, Scan Code of the Key
	BYTE bASCIICode;
	BYTE bScanCode;

	// Key Flags
	BYTE bFlags;
} KEYEVENT;


// Window Event
typedef struct kWindowEventStruct
{
	// Window ID
	QWORD qwWindowID;

	// Area info
	RECT stArea;
} WINDOWEVENT;


typedef struct kEventStruct
{
	// Event Type
	QWORD qwType;

	// Event Type defined by qwType field
	union
	{
		// for Mouse Event
		MOUSEEVENT stMouseEvent;

		// for Key Event
		KEYEVENT stKeyEvent;

		// for Window Event
		WINDOWEVENT stWindowEvent;

		// for User-define Event
		QWORD vqwData[3];
	};
} EVENT;



// ======================================
// for JPEG Decoder
// ======================================

// Huffman Table
typedef struct
{
	int 		   elem;			// element count
	unsigned short code[256];
	unsigned char  size[256];
	unsigned char  value[256];
} HUFF;


// for JPEG Decoding, contains important information used in Encoding Process
typedef struct
{
	// SOF (Start Of Frame)
	int width;					// image width
	int height;					// image height


	// MCU (Minimum Coded Unit)
	int mcu_width;
	int mcu_height;

	int max_h, max_v;
	int compo_count;
	int compo_id[3];
	int compo_sample[3];
	int compo_h[3];
	int compo_v[3];
	int compo_qt[3];


	// SOS (Start Of Scan)
	int scan_count;
	int scan_id[3];
	int scan_ac[3];
	int scan_dc[3];
	int scan_h[3];				// sampling element count
	int scan_v[3];				// sampling element count
	int scan_qt[3];				// quantization table index
	

	// DRI (Define Restart Interval)
	int  interval;

	int  mcu_buf[32 * 32 * 4];	// buffer
	int* mcu_yuv[4];
	int  mcu_preDC[3];


	// DQT (Define Quantization Table)
	int dqt[3][64];
	int n_dqt;


	// DHT (Define Huffman Tables)
	HUFF huff[2][3];			// 6 Huffman tables, about 6 KB


	// I/O
	unsigned char* data;
	int 		   data_index;
	int 		   data_size;

	unsigned long  bit_buff;
	int 		   bit_remain;

} JPEG;



#endif
