#include "Types.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"
#include "Queue.h"
#include "Synchronization.h"


/////////////////////////////////////////////////////////////////////
// functions related to Keyboard Controller, Keyboard Controll
/////////////////////////////////////////////////////////////////////


// Is Output Buffer of Keyboard empty?
BOOL kIsOutputBufferFull(void)
{
	if(kInPortByte(0x64) & 0x01)
		return TRUE;
	
	return FALSE;
}


// Is Input Buffer of Keyboard empty?
BOOL kIsInputBufferFull(void)
{
	if(kInPortByte(0x64) & 0x02)
		return TRUE;
		
	return FALSE;
}


// Wait for ACK signal
// 		insert the Data which not ACK into proper Queue (Keyboard Queue or Mouse Queue)
BOOL kWaitForACKAndPutOtherScanCode(void)
{
	int i, j;
	BYTE bData;
	BOOL bResult = FALSE;
	BOOL bMouseData;


	// possible to come Key code first before ACK comes
	// receive Key Code : maximum 100
	for(j=0; j<100; j++)
	{
		for(i=0; i<0xFFFF; i++)
		{
			if(kIsOutputBufferFull() == TRUE)
			{
				break;
			}
		}


		// is the Data of Keyboard? or Mouse?
		if(kIsMouseDataInOutputBuffer() == TRUE)
		{
			bMouseData = TRUE;
		}
		else
		{
			bMouseData = FALSE;
		}


		bData = kInPortByte(0x60);


		// if received Data is ACK, success
		if(bData == 0xFA)
		{
			bResult = TRUE;
			break;
		}
		// if received Data not ACK, insert Data into Keyboard Queue or Mouse Queue
		else
		{
			if(bMouseData == FALSE)
			{
				kConvertScanCodeAndPutQueue(bData);
			}
			else
			{
				kAccumulateMouseDataAndPutQueue(bData);
			}
		}
	}

	return bResult;
}


// Activate Keyboard (both Keyboard Controller and Keyboard)
BOOL kActivateKeyboard(void)
{
	int i;
	int j;
	BOOL bPreviousInterrupt;
	BOOL bResult;

	
	// ----------------------------------------------
	// Disable Interrupt
	bPreviousInterrupt = kSetInterruptFlag(FALSE);
	// ----------------------------------------------
	

	// Activate Keyboard Controller	
	kOutPortByte(0x64, 0xAE);


	// Activate Keyboard 
	for(i=0; i<0xFFFF; i++)
		if(kIsInputBufferFull() == FALSE)
			break;

	kOutPortByte(0x60, 0xF4);

	
	// Wait for ACK
	bResult = kWaitForACKAndPutOtherScanCode();


	// ----------------------------------------------
	// Restore Previous Interrupt Status
	kSetInterruptFlag(bPreviousInterrupt);
	// ----------------------------------------------
	
	
	return bResult;
}


// Get Keyboard Scan Code and return
BYTE kGetKeyboardScanCode(void)
{
	while(kIsOutputBufferFull() == FALSE)
	{
		;
	}

	return kInPortByte(0x60);
}


// Change Keyboard LED Status
BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn)
{
	int i;
	int j;
	BYTE bLEDStatus = 0;

	BOOL bPreviousInterrupt;
	BOOL bResult;
	BYTE bData;


	// ----------------------------------------------
	// Disable Interrupt
	bPreviousInterrupt = kSetInterruptFlag(FALSE);
	// ----------------------------------------------
	

	for(i=0; i<0xFFFF; i++)
		if(kIsInputBufferFull() == FALSE)
			break;
	

	kOutPortByte(0x60, 0xED);	// 0xED : change LED status


	for(i=0; i<0xFFFF; i++)
		if(kIsInputBufferFull() == FALSE)
			break;

	
	bResult = kWaitForACKAndPutOtherScanCode();

	if(bResult == FALSE)
	{
		// ----------------------------------------------
		// Restore Previous Interrupt Status
		kSetInterruptFlag(bPreviousInterrupt);
		// ----------------------------------------------
		
		return FALSE;
	}


	bLEDStatus = ((bCapsLockOn<<2) | (bNumLockOn<<1) | (bScrollLockOn));
	
	kOutPortByte(0x60, bLEDStatus);


	for(i=0; i<0xFFFF; i++)
		if(kIsInputBufferFull() == FALSE)
			break;

	
	bResult = kWaitForACKAndPutOtherScanCode();

	// ----------------------------------------------
	// Restore Previous Interrupt Status
	kSetInterruptFlag(bPreviousInterrupt);
	// ----------------------------------------------
	

	return bResult;
}


// Enable A20 Gate
void kEnableA20Gate(void)
{
	int i;
	BYTE bOutputPortData;

	kOutPortByte(0x64, 0xD0);	// 0xD0 : read PS/2 Controller
	
	for(i=0; i<0xFFFF; i++)
		if(kIsOutputBufferFull() == TRUE)
			break;

	bOutputPortData = kInPortByte(0x60);

	bOutputPortData |= 0x02;

	for(i=0; i<0xFFFF; i++)
		if(kIsInputBufferFull() == FALSE)
			break;

	kOutPortByte(0x64, 0xD1);

	kOutPortByte(0x60, bOutputPortData);
}


// Process reset
void kReboot(void)
{
	int i;

	for(i=0; i<0xFFFF; i++)
		if(kIsOutputBufferFull() == TRUE)
			break;

	kOutPortByte(0x64, 0xD1);

	kOutPortByte(0x60, 0x00);

	while(1);
}



//////////////////////////////////////////////////////////////////////////
// structures and functions related to converting Scan Code to ASCII
//////////////////////////////////////////////////////////////////////////

//------------------ Structures --------------------

// Manage Keyboard status
static KEYBOARDMANAGER gs_stKeyboardManager = {0, };


// Key Queue and Key Queue Buffer
static QUEUE 	gs_stKeyQueue;
static KEYDATA	gs_vstKeyQueueBuffer[KEY_MAXQUEUECOUNT];


// Table Converting Scan Code to ASCII Code
static KEYMAPPINGENTRY gs_vstKeyMappingTable[KEY_MAPPINGTABLEMAXCOUNT] =
{
	// Scan Code		Normal Code			Combined Code
	/* 0  (0x00) */  {	KEY_NONE		,	KEY_NONE		},	
	/* 1  (0x01) */  {	KEY_ESC			,	KEY_ESC			},	
	/* 2  (0x02) */  {	'1'				,	'!'				},	
	/* 3  (0x03) */  {	'2'				,	'@'				},	
	/* 4  (0x04) */  {	'3'				,	'#'				},	
	/* 5  (0x05) */  {	'4'				,	'$'				},	
	/* 6  (0x06) */  {	'5'				,	'%'				},	
	/* 7  (0x07) */  {	'6'				,	'^'				},	
	/* 8  (0x08) */  {	'7'				,	'&'				},	
	/* 9  (0x09) */  {	'8'				,	'*'				},	
	/* 10 (0x0A) */  {	'9'				,	'('				},	
	/* 11 (0x0B) */  {	'0'				,	')'				},	
	/* 12 (0x0C) */  {	'-'				,	'_'				},	
	/* 13 (0x0D) */  {	'='				,	'+'				},	
	/* 14 (0x0E) */  {	KEY_BACKSPACE	,	KEY_BACKSPACE	},	
	/* 15 (0x0F) */  {	KEY_TAB			,	KEY_TAB			},	
	/* 16 (0x10) */  {	'q'				,	'Q'				},	
	/* 17 (0x11) */  {	'w'				,	'W'				},	
	/* 18 (0x12) */  {	'e'				,	'E'				},	
	/* 19 (0x13) */  {	'r'				,	'R'				},	
	/* 20 (0x14) */  {	't'				,	'T'				},	
	/* 21 (0x15) */  {	'y'				,	'Y'				},	
	/* 22 (0x16) */  {	'u'				,	'U'				},	
	/* 23 (0x17) */  {	'i'				,	'I'				},	
	/* 24 (0x18) */  {	'o'				,	'O'				},	
	/* 25 (0x19) */  {	'p'				,	'P'				},	
	/* 26 (0x1A) */  {	'['				,	'{'				},	
	/* 27 (0x1B) */  {	']'				,	'}'				},	
	/* 28 (0x1C) */  {	'\n'			,	'\n'			},	
	/* 29 (0x1D) */  {	KEY_CTRL		,	KEY_CTRL		},	
	/* 30 (0x1E) */  {	'a'				,	'A'				},	
	/* 31 (0x1F) */  {	's'				,	'S'				},	
	/* 32 (0x20) */  {	'd'				,	'D'				},	
	/* 33 (0x21) */  {	'f'				,	'F'				},	
	/* 34 (0x22) */  {	'g'				,	'G'				},	
	/* 35 (0x23) */  {	'h'				,	'H'				},	
	/* 36 (0x24) */  {	'j'				,	'J'				},	
	/* 37 (0x25) */  {	'k'				,	'K'				},	
	/* 38 (0x26) */  {	'l'				,	'L'				},	
	/* 39 (0x27) */  {	';'				,	':'				},	
	/* 40 (0x28) */  {	'\''			,	'\"'			},	
	/* 41 (0x29) */  {	'`'				,	'~'				},	
	/* 42 (0x2A) */  {	KEY_LSHIFT		,	KEY_LSHIFT		},	
	/* 43 (0x2B) */  {	'\\'			,	'|'				},	
	/* 44 (0x2C) */  {	'z'				,	'Z'				},	
	/* 45 (0x2D) */  {	'x'				,	'X'				},	
	/* 46 (0x2E) */  {	'c'				,	'C'				},	
	/* 47 (0x2F) */  {	'v'				,	'V'				},	
	/* 48 (0x30) */  {	'b'				,	'B'				},	
	/* 49 (0x31) */  {	'n'				,	'N'				},	
	/* 50 (0x32) */  {	'm'				,	'M'				},	
	/* 51 (0x33) */  {	','				,	'<'				},	
	/* 52 (0x34) */  {	'.'				,	'>'				},	
	/* 53 (0x35) */  {	'/'				,	'?'				},	
	/* 54 (0x36) */  {	KEY_RSHIFT		,	KEY_RSHIFT		},	
	/* 55 (0x37) */  {	'*'				,	'*'				},	
	/* 56 (0x38) */  {	KEY_LALT		,	KEY_LALT		},	
	/* 57 (0x39) */  {	' '				,	' '				},	
	/* 58 (0x3A) */  {	KEY_CAPSLOCK	,	KEY_CAPSLOCK	},	
	/* 59 (0x3B) */  {	KEY_F1			,	KEY_F1			},	
	/* 60 (0x3C) */  {	KEY_F2			,	KEY_F2			},	
	/* 61 (0x3D) */  {	KEY_F3			,	KEY_F3			},	
	/* 62 (0x3E) */  {	KEY_F4			,	KEY_F4			},	
	/* 63 (0x3F) */  {	KEY_F5			,	KEY_F5			},	
	/* 64 (0x40) */  {	KEY_F6			,	KEY_F6			},	
	/* 65 (0x41) */  {	KEY_F7			,	KEY_F7			},	
	/* 66 (0x42) */  {	KEY_F8			,	KEY_F8			},	
	/* 67 (0x43) */  {	KEY_F9			,	KEY_F9			},	
	/* 68 (0x44) */  {	KEY_F10			,	KEY_F10			},	
	/* 69 (0x45) */  {	KEY_NUMLOCK		,	KEY_NUMLOCK		},	
	/* 70 (0x46) */  {	KEY_SCROLLLOCK	,	KEY_SCROLLLOCK	},

	/* 71 (0x47) */  {	KEY_HOME		,	'7'				},	
	/* 72 (0x48) */  {	KEY_UP			,	'8'				},	
	/* 73 (0x49) */  {	KEY_PAGEUP		,	'9'				},	
	/* 74 (0x4A) */  {	'-'				,	'-'				},
	/* 75 (0x4B) */  {	KEY_LEFT		,	'4'				},	
	/* 76 (0x4C) */  {	KEY_CENTER		,	'5'				},	
	/* 77 (0x4D) */  {	KEY_RIGHT		,	'6'				},	
	/* 78 (0x4E) */  {	'+'				,	'+'				},	
	/* 79 (0x4F) */  {	KEY_END			,	'1'				},	
	/* 80 (0x50) */  {	KEY_DOWN		,	'2'				},	
	/* 81 (0x51) */  {	KEY_PAGEDOWN	,	'3'				},	
	/* 82 (0x52) */  {	KEY_INS			,	'0'				},	
	/* 83 (0x53) */  {	KEY_DEL			,	'.'				},	
	/* 84 (0x54) */  {	KEY_NONE		,	KEY_NONE		},	
	/* 85 (0x55) */  {	KEY_NONE		,	KEY_NONE		},	
	/* 86 (0x56) */  {	KEY_NONE		,	KEY_NONE		},	
	/* 87 (0x57) */  {	KEY_F11			,	KEY_F11			},	
	/* 88 (0x58) */  {	KEY_F12			,	KEY_F12			}
};



//------------------ Functions --------------------

// Is Scan Code in Alphabet range?
BOOL kIsAlphabetScanCode(BYTE bScanCode)
{
	if(('a' <= gs_vstKeyMappingTable[bScanCode].bNormalCode) && (gs_vstKeyMappingTable[bScanCode].bNormalCode <= 'z'))
		return TRUE;

	return FALSE;
}


// Is Scan code in Number or Symbol range?
BOOL kIsNumberOrSymbolScanCode(BYTE bScanCode)
{
	if((2<=bScanCode) && (bScanCode<=53) && (kIsAlphabetScanCode(bScanCode)==FALSE))
		return TRUE;

	return FALSE;
}


// Is Scan Code in Num Pad range?
BOOL kIsNumberPadScanCode(BYTE bScanCode)
{
	if((71<=bScanCode) && (bScanCode<=83))
		return TRUE;

	return FALSE;
}


// Is scanned Code use combined Key?
BOOL kIsUseCombinedCode(BYTE bScanCode)
{
	BYTE bDownScanCode;
	BOOL bUseCombinedKey = FALSE;
	
	bDownScanCode = bScanCode & 0x7F;

	
	// if Alphabet Key, affected by Caps Lock and Shift
	if(kIsAlphabetScanCode(bDownScanCode) == TRUE)
	{
		if(gs_stKeyboardManager.bShiftDown ^ gs_stKeyboardManager.bCapsLockOn)
			bUseCombinedKey = TRUE;
		else
			bUseCombinedKey = FALSE;
	}

	// if Number or Symbol Key, affected by Shift
	else if(kIsNumberOrSymbolScanCode(bDownScanCode) == TRUE)
	{
		if(gs_stKeyboardManager.bShiftDown == TRUE)
			bUseCombinedKey = TRUE;
		else
			bUseCombinedKey = FALSE;
	}

	// if Num Pad, affected by Num Lock
	// exception 0xE0, Num Pad Code is overlapped with Extended Key Code
	// So, use Combined Key when 0xE0 not received
	else if ((kIsNumberPadScanCode(bDownScanCode) == TRUE) && (gs_stKeyboardManager.bExtendedCodeIn == FALSE))
	{
		if(gs_stKeyboardManager.bNumLockOn == TRUE)
			bUseCombinedKey = TRUE;
		else
			bUseCombinedKey = FALSE;
	}

	return bUseCombinedKey;
}


// Update combined key status, Sync LED status	(update when one key pressed)
void UpdateCombinationKeyStatusAndLED(BYTE bScanCode)
{
	BOOL bDown;						// Up/Down Status
	BYTE bDownScanCode;				// Scan Code in Down status
	BOOL bLEDStatusChanged = FALSE;	// LED Status


	// handle Key Down / Key Up status
	// if bit 7 is 1, it means Down. otherwise Up
	
	if(bScanCode & 0x80)	// status : up
	{
		bDown = FALSE;
		bDownScanCode = (bScanCode & 0x7F);
	}
	else	// status : down
	{
		bDown = TRUE;
		bDownScanCode = bScanCode;
	}
	

	// --------------- Search Combined Key ---------------

	// if Scan Code is Shift (42 or 54), update Shift status
	if(bDownScanCode == 42 || bDownScanCode == 54)
		gs_stKeyboardManager.bShiftDown = bDown;

	// if Scan Code is Caps Lock (58), update Caps Lock status and LED
	else if(bDownScanCode == 58 && bDown == TRUE)
	{
		gs_stKeyboardManager.bCapsLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}
	
	// if Scan Code is Num Lock (69), update Num Lock status and LED
	else if(bDownScanCode == 69 && bDown == TRUE)
	{
		gs_stKeyboardManager.bNumLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}
	
	// if Scan Code is Scroll Lock (70), update Scroll Lock status and LED
	else if(bDownScanCode == 60 && bDown == TRUE)
	{
		gs_stKeyboardManager.bScrollLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}

	// if LED is changed, send command to Keyborad to change LED status
	if(bLEDStatusChanged == TRUE)
	{
		kChangeKeyboardLED(gs_stKeyboardManager.bCapsLockOn, 
				gs_stKeyboardManager.bNumLockOn, gs_stKeyboardManager.bScrollLockOn);
	}
}


// Convert Scan Code to ASCII Code
BOOL kConvertScanCodeToASCIICode(BYTE bScanCode, BYTE* pbASCIICode, BOOL* pbFlags)
{
	BOOL bUseCombinedKey;

	// if Pause key received before, ignore rest scan code
	if(gs_stKeyboardManager.iSkipCountForPause > 0)
	{
		gs_stKeyboardManager.iSkipCountForPause--;
		return FALSE;
	}
	
	// handling Pause key
	if(bScanCode == 0xE1)
	{
		*pbASCIICode = KEY_PAUSE;
		*pbFlags	 = KEY_FLAGS_DOWN;
		
		gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
		
		return TRUE;
	}
	// handling Extended key
	else if(bScanCode == 0xE0)
	{
		gs_stKeyboardManager.bExtendedCodeIn = TRUE;
		
		return FALSE;
	}

	// should Combined Key be returned?
	bUseCombinedKey = kIsUseCombinedCode(bScanCode);

	// setting key value
	if(bUseCombinedKey == TRUE)
		*pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bCombinedCode;
	else
		*pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bNormalCode;
	

	// is next code Extended code?
	if(gs_stKeyboardManager.bExtendedCodeIn == TRUE)
	{
		*pbFlags = KEY_FLAGS_EXTENDEDKEY;
		gs_stKeyboardManager.bExtendedCodeIn = FALSE;
	}
	else
		*pbFlags = 0;

	// status is Down? or Up?
	if((bScanCode & 0x80) == 0)
		*pbFlags |= KEY_FLAGS_DOWN;
	
	// update Down/Up status of Extended Key
	UpdateCombinationKeyStatusAndLED(bScanCode);
	
	return TRUE;
}


// Initialize Keyboard
BOOL kInitializeKeyboard(void)
{
	// Initialize Queue
	kInitializeQueue(&gs_stKeyQueue, gs_vstKeyQueueBuffer, KEY_MAXQUEUECOUNT, sizeof(KEYDATA));


	// Initialize SpinLock
	kInitializeSpinLock(&(gs_stKeyboardManager.stSpinLock));


	// Activate Keyboard
	return kActivateKeyboard();
}


// Convert Scan Code to KEYDATA and insert into Queue
BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode)
{
	KEYDATA stData;
	BOOL	bResult = FALSE;
	BOOL	bPreviousInterrupt;

	stData.bScanCode = bScanCode;
	
	if(kConvertScanCodeToASCIICode(bScanCode, &(stData.bASCIICode), &(stData.bFlags)) == TRUE)
	{

		// =============== CRITICAL SECTION START ===============
		kLockForSpinLock(&(gs_stKeyboardManager.stSpinLock));
		// ======================================================
		
		bResult = kPutQueue(&gs_stKeyQueue, &stData);

		// =============== CRITICAL SECTION END ===============
		kUnlockForSpinLock(&(gs_stKeyboardManager.stSpinLock));
		// ====================================================
	}

	return bResult;
}


// Get KEYDATA from Queue
BOOL kGetKeyFromKeyQueue(KEYDATA* pstData)
{
	BOOL bResult;
	BOOL bPreviousInterrupt;

	// =============== CRITICAL SECTION START ===============
	kLockForSpinLock(&(gs_stKeyboardManager.stSpinLock));
	// ======================================================

	bResult = kGetQueue(&gs_stKeyQueue, pstData);

	// =============== CRITICAL SECTION END ===============
	kUnlockForSpinLock(&(gs_stKeyboardManager.stSpinLock));
	// ====================================================


	return bResult;
}


