#ifndef __WINDOWMANAGER_H__
#define __WINDOWMANAGER_H__


// ---------- Macro ----------

// Max count of Data/Event integration
#define WINDOWMANAGER_DATAACCUMULATECOUNT		20

// Window Resize Marker Size
#define WINDOWMANAGER_RESIZEMARKERSIZE			20

// Window Resize Marker Color
#define WINDOWMANAGER_COLOR_RESIZEMARKER		RGB(210, 20, 20)

// Window Resize Marker Thickness
#define WINDOWMANAGER_THICK_RESIZEMARKER		4



// ---------- Function ----------

void kStartWindowManager(void);
BOOL kProcessMouseData(void);
BOOL kProcessKeyData(void);
BOOL kProcessEventQueueData(void);


void kDrawResizeMarker(const RECT* pstArea, BOOL bShowMarker);



#endif
