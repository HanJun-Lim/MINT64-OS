#ifndef __MAIN_H__
#define __MAIN_H__


// --------------- Macro ---------------

// Max count of the bubble
#define MAXBUBBLECOUNT		50

// Radius of the bubble
#define RADIUS				16

// Base speed of bubble
#define DEFAULTSPEED		3

// Maximum player life
#define MAXLIFE				20


// Window width, height
#define WINDOW_WIDTH		250
#define WINDOW_HEIGHT		300

// Game info area height
#define INFORMATION_HEIGHT	20



// --------------- Structure ---------------

typedef struct BubbleStruct
{
	// (X, Y)
	QWORD qwX;
	QWORD qwY;

	// falling speed (Delta Y)
	QWORD qwSpeed;

	// bubble color
	COLOR stColor;

	// is the bubble alive?
	BOOL bAlive;
} BUBBLE;


typedef struct GameInfoStruct
{
	// ********** for bubble management **********
	
	// information of the bubbles
	BUBBLE* pstBubbleBuffer;

	// the number of alive bubble
	int iAliveBubbleCount;


	// ********** for game flow management **********
	
	// player life
	int iLife;

	// user score
	QWORD qwScore;

	// is the game started?
	BOOL bGameStart;
} GAMEINFO;



// --------------- Function ---------------

BOOL Initialize(void);
BOOL CreateBubble(void);
void MoveBubble(void);
void DeleteBubbleUnderMouse(POINT* pstMouseXY);
void DrawInformation(QWORD qwWindowID);
void DrawGameArea(QWORD qwWindowID, POINT* pstMouseXY);



#endif
