#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "Types.h"
#include "Synchronization.h"



// ---------------------------
// ---------- Macro ----------
// ---------------------------

// number of ignored Scan Code to handle PAUSE Key
#define KEY_SKIPCOUNTFORPAUSE		2

// Flags for Key Status
#define KEY_FLAGS_UP				0x00
#define KEY_FLAGS_DOWN				0x01
#define KEY_FLAGS_EXTENDEDKEY		0x02

// Macros for Scan Code Mapping Table
#define KEY_MAPPINGTABLEMAXCOUNT	89

// Some function key (Home, F1, etc..) have no allocated ASCII char
// therefore, MINT64 OS allocates value upper 0x80 for them
//
//		Key				ASCII Code
#define KEY_NONE		0x00
#define KEY_ENTER 		'\n'
#define	KEY_TAB			'\t'
#define KEY_ESC			0x1B
#define KEY_BACKSPACE	0x08

#define KEY_CTRL		0x81
#define KEY_LSHIFT		0x82
#define KEY_RSHIFT		0x83
#define KEY_PRINTSCREEN	0x84
#define KEY_LALT		0x85
#define KEY_CAPSLOCK	0x86
#define KEY_F1			0x87
#define KEY_F2			0x88
#define KEY_F3			0x89
#define KEY_F4			0x8A
#define KEY_F5			0x8B
#define KEY_F6			0x8C
#define KEY_F7			0x8D
#define KEY_F8			0x8E
#define KEY_F9			0x8F
#define KEY_F10			0x90
#define KEY_NUMLOCK		0x91
#define KEY_SCROLLLOCK	0x92
#define KEY_HOME		0x93
#define KEY_UP			0x94
#define KEY_PAGEUP		0x95
#define KEY_LEFT		0x96
#define KEY_CENTER		0x97
#define KEY_RIGHT		0x98
#define KEY_END			0x99
#define KEY_DOWN		0x9A
#define KEY_PAGEDOWN	0x9B
#define KEY_INS			0x9C
#define KEY_DEL			0x9D
#define KEY_F11			0x9E
#define KEY_F12			0x9F
#define KEY_PAUSE		0xA0

// Macro for Key Queue
#define KEY_MAXQUEUECOUNT	100		// Maximum size of Key Queue



// -------------------------------
// ---------- Structure ----------
// -------------------------------

#pragma pack(push, 1)


// Scan Code Table Entry

typedef struct kKeyMappingEntryStruct
{
	BYTE bNormalCode;
	BYTE bCombinedCode;
} KEYMAPPINGENTRY;


// Data of Key Queue

typedef struct kKeyDataStruct
{
	BYTE bScanCode;		// Scan Code from Keyboard
	BYTE bASCIICode;	// Converted Scan Code
	BYTE bFlags;		// Key Status (Key Down/Up/Extended)
} KEYDATA;

#pragma pack(pop)


// Manager Keyboard Status

typedef struct kKeyBoardManagerStruct
{
	// SpinLock for Synchronization
	SPINLOCK stSpinLock;

	// Combined Key Info
	BOOL bShiftDown;
	BOOL bCapsLockOn;
	BOOL bNumLockOn;
	BOOL bScrollLockOn;

	// handle Extended Key
	BOOL bExtendedCodeIn;
	int iSkipCountForPause;
} KEYBOARDMANAGER;



// ------------------------------
// ---------- Function ----------
// ------------------------------

// functions related to Keyboard Controller, Keyboard Controll

BOOL kIsOutputBufferFull(void);
BOOL kIsInputBufferFull(void);
BOOL kActivateKeyboard(void);
BYTE kGetKeyboardScanCode(void);
BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn);
void kEnableA20Gate(void);
void kReboot(void);


// functions related to Converting Scan Code to ASCII

BOOL kIsAlphabetScanCode(BYTE bScanCode);
BOOL kIsNumberOrSymbolScanCode(BYTE bScanCode);
BOOL kIsNumberPadScanCode(BYTE bScanCode);
BOOL kIsUseCombinedCode(BYTE bScanCode);
void UpdateCombinationKeyStatusAndLED(BYTE bScanCode);
BOOL kConvertScanCodeToASCIICode(BYTE bScanCode, BYTE* pbASCIICode, BOOL* pbFlags);


// Queue added

BOOL kInitializeKeyboard(void);
BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode);
BOOL kGetKeyFromKeyQueue(KEYDATA* pstData);
BOOL kWaitForACKAndPutOtherScanCode(void);


#endif
