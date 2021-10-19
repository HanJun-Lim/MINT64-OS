#ifndef __VBE_H__
#define __VBE_H__

#include "Types.h"


// =============== Macro ===============

// The Address where Mode Information Block saved
#define VBE_MODEINFOBLOCKADDRESS			0x7E00

// The Address where Graphic Mode starting Flag saved (Variable position in BootLoader.asm)
#define VBE_STARTGRAPHICMODEFLAGADDRESS		0x7C0A



// =============== Structure ===============

#pragma pack(push, 1)

// 256 byte
typedef struct kVBEInfoBlockStruct
{
	// ----- Below field supported in all VBE version -----
	
	WORD  wModeAttribute;
	BYTE  bWinAAttribute;
	BYTE  bWinBAttribute;
	WORD  wWinGranulity;
	WORD  wWinSize;
	WORD  wWinASegment;
	WORD  wWinBSegment;
	DWORD dwWinFuncPtr;
	WORD  wBytesPerScanLine;

	// ----- Below field supported in VBE 1.2 or later version -----

	WORD  wXResolution;
	WORD  wYResolution;
	BYTE  bXCharSize;
	BYTE  bYCharSize;
	BYTE  bNumberOfPlane;
	BYTE  bBitsPerPixel;
	BYTE  bNumberOfBanks;
	BYTE  bMemoryModel;
	BYTE  bBankSize;
	BYTE  bNumberOfImagePages;
	BYTE  bReserved;

	// fields related to "Direct Color"
	BYTE  bRedMaskSize;
	BYTE  bRedFieldPosition;
	BYTE  bGreenMaskSize;
	BYTE  bGreenFieldPosition;
	BYTE  bBlueMaskSize;
	BYTE  bBlueFieldPosition;
	BYTE  bReservedMaskSize;
	BYTE  bReservedFieldPosition;
	BYTE  bDirectColorModeInfo;

	// ----- Below field supported in VBE 2.0 or later version -----
	
	DWORD dwPhysicalBasePointer;
	DWORD dwReserved1;
	DWORD dwREserved2;

	// ----- Below field supported in VBE 3.0 or later version -----
	
	WORD  wLinearBytesPerScanLine;
	BYTE  bBankNumberOfImagePages;
	BYTE  bLinearNumberOfImagePages;

	// fields related to "Direct Color" when Linear Frame Buffer Mode
	
	BYTE  bLinearRedMaskSize;
	BYTE  bLinearRedFieldPosition;
	BYTE  bLinearGreenMaskSize;
	BYTE  bLinearGreenFieldPosition;
	BYTE  bLinearBlueMaskSize;
	BYTE  bLinearBlueFieldPosition;
	BYTE  bLinearReservedMaskSize;
	BYTE  bLinearReservedFieldPosition;
	DWORD dwMaxPixelClock;
	BYTE  vbReserved[189];
} VBEMODEINFOBLOCK;

#pragma pack(pop)



// =============== Function ===============

VBEMODEINFOBLOCK* kGetVBEModeInfoBlock(void);



#endif
