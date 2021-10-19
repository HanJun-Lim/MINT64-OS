#ifndef __APPLICATIONPANELTASK_H__
#define __APPLICATIONPANELTASK_H__

#include "Types.h"
#include "Window.h"
#include "Font.h"



// --------------- Macro ---------------

// Application Panel Window Height
#define APPLICATIONPANEL_HEIGHT				31


// Application Panel Window Title
#define APPLICATIONPANEL_TITLE				"SYS_APPLICATIONPANEL"


// Application Panel Task Clock Width
#define APPLICATIONPANEL_CLOCKWIDTH			(8 * FONT_ENGLISHWIDTH)


// Height of Item in Application List Window
#define APPLICATIONPANEL_LISTITEMHEIGHT		(FONT_ENGLISHHEIGHT + 4)


// Application List Window Title
#define APPLICATIONPANEL_LISTTITLE			"SYS_APPLICATIONLIST"


// Application Panel Color
#define APPLICATIONPANEL_COLOR_OUTERLINE	RGB(109, 218,  22)
#define APPLICATIONPANEL_COLOR_MIDDLELINE	RGB(183, 240, 171)
#define APPLICATIONPANEL_COLOR_INNERLINE	RGB(150, 210, 140)
#define APPLICATIONPANEL_COLOR_BACKGROUND	RGB( 55, 135,  11)
#define APPLICATIONPANEL_COLOR_ACTIVE		RGB( 79, 204,  11)



// --------------- Structure ---------------

typedef struct kApplicationPanelDataStruct
{
	// Application Panel Window ID
	QWORD qwApplicationPanelID;

	// Application List Window ID
	QWORD qwApplicationListID;

	// Application Panel Button Location
	RECT stButtonArea;

	// Application List Window Width
	int iApplicationListWidth;

	// Index of Item previously Mouse over
	int iPreviousMouseOverIndex;

	// Is Application List Window visible?
	BOOL bApplicationWindowVisible;
} APPLICATIONPANELDATA;


typedef struct kApplicationEntryStruct
{
	// GUI Task Name
	char* pcApplicationName;

	// GUI Task Entry Point
	void* pvEntryPoint;
} APPLICATIONENTRY;



// --------------- Function ---------------

void kApplicationPanelGUITask(void);

static void kDrawClockInApplicationPanel(QWORD qwApplicationPanelID);
static BOOL kCreateApplicationPanelWindow(void);
static void kDrawDigitalClock(QWORD qwApplicationPanelID);
static BOOL kCreateApplicationListWindow(void);
static void kDrawApplicationListItem(int iIndex, BOOL bSelected);
static BOOL kProcessApplicationPanelWindowEvent(void);
static BOOL kProcessApplicationListWindowEvent(void);
static int  kGetMouseOverItemIndex(int iMouseY);



#endif
