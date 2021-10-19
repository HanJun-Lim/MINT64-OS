#ifndef __MAIN_H__
#define __MAIN_H__


// --------------- Macro ---------------

// width, height of Game board
#define BOARDWIDTH				8
#define BOARDHEIGHT				12

// size of one block
#define BLOCKSIZE				32

// moving block count
#define BLOCKCOUNT				3

// mark of empty block
#define EMPTYBLOCK				0

// mark of block to erase
#define ERASEBLOCK				0xFF

// kind of the block
#define BLOCKKIND				5

// width, height of Window
#define WINDOW_WIDTH			(BOARDWIDTH * BLOCKSIZE)
#define WINDOW_HEIGHT			(WINDOW_TITLEBAR_HEIGHT + INFORMATION_HEIGHT + BOARDHEIGHT * BLOCKSIZE)

// height of Game info area
#define INFORMATION_HEIGHT		20



// --------------- Structure ---------------

typedef struct GameInfoStruct
{
	// *************** for block management ***************
	
	// color of the blocks (internal/edge color)
	COLOR vstBlockColor[BLOCKKIND + 1];
	COLOR vstEdgeColor[BLOCKKIND + 1];

	// current position of moving block
	int iBlockX;
	int iBlockY;

	// manages fixed block
	BYTE vvbBoard[BOARDHEIGHT][BOARDWIDTH];

	// manages block to erase
	BYTE vvbEraseBlock[BOARDHEIGHT][BOARDWIDTH];

	// store the information of currently moving block
	BYTE vbBlock[BLOCKCOUNT];


	// *************** for game flow ***************
	
	// is the game started?
	BOOL bGameStart;

	// user score
	QWORD qwScore;

	// game level
	QWORD qwLevel;
} GAMEINFO;



// --------------- Function ---------------

void Initialize(void);
void CreateBlock(void);
BOOL IsMovePossible(int iBlockX, int iBlockY);
BOOL FreezeBlock(int iBlockX, int iBlockY);
void EraseAllContinuousBlockOnBoard(QWORD qwWindowID);

BOOL MarkContinuousVerticalBlockOnBoard(void);
BOOL MarkContinuousHorizonBlockOnBoard(void);
BOOL MarkContinuousDiagonalBlockOnBoard(void);
void EraseMarkedBlock(void);
void CompactBlockOnBoard(void);

void DrawInformation(QWORD qwWindowID);
void DrawGameArea(QWORD qwWindowID);



#endif
