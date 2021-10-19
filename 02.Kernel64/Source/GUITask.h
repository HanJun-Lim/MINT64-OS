#ifndef __GUITASK_H__
#define __GUITASK_H__

#include "Types.h"
#include "Window.h"


// --------------- Macro ---------------

// User Event Type
#define EVENT_USER_TESTMESSAGE				0x80000001


// ***** for System Monitor Task *****

// Processor Info Bar Width
#define SYSTEMMONITOR_PROCESSOR_WIDTH		150

// Margin between Process Info Area
#define SYSTEMMONITOR_PROCESSOR_MARGIN		20

// Processor Info Bar Height
#define SYSTEMMONITOR_PROCESSOR_HEIGHT		150

// System Monitor Window Height
#define SYSTEMMONITOR_WINDOW_HEIGHT			310

// Memory Info Bar Height
#define SYSTEMMONITOR_MEMORY_HEIGHT			100

// Bar Color
#define SYSTEMMONITOR_BAR_COLOR				RGB(55, 215, 47)



// --------------- Function ---------------  

// Base GUI Task, Hello World GUI Task
void kBaseGUITask(void);
void kHelloWorldGUITask(void);

// System Monitor Task functions
void kSystemMonitorTask(void);
static void kDrawProcessorInformation(QWORD qwWindowID, int iX, int iY, BYTE bAPICID);
static void kDrawMemoryInformation(QWORD qwWindowID, int iY, int iWindowWidth);

// GUI Console Shell Task functions
void kGUIConsoleShellTask(void);
static void kProcessConsoleBuffer(QWORD qwWindowID);

// Image Viewer Task functions
void kImageViewerTask(void);
static void kDrawFileName(QWORD qwWindowID, RECT* pstArea, char* pcFileName, int iNameLength);
static BOOL kCreateImageViewerWindowAndExecute(QWORD qwMainWindowID, const char* pcFileName);



#endif
