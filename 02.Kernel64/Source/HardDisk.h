#ifndef __HARDDISK_H__
#define __HARDDISK_H__

#include "Types.h"
#include "Synchronization.h"


// ********** Macro **********

// information of Primary PATA Port, Secondary PATA Port

#define HDD_PORT_PRIMARYBASE				0x1F0
#define HDD_PORT_SECONDARYBASE				0x170

// for port index

#define HDD_PORT_INDEX_DATA					0x00
#define HDD_PORT_INDEX_SECTORCOUNT			0x02
#define HDD_PORT_INDEX_SECTORNUMBER			0x03
#define HDD_PORT_INDEX_CYLINDERLSB			0x04
#define HDD_PORT_INDEX_CYLINDERMSB			0x05
#define HDD_PORT_INDEX_DRIVEANDHEAD			0x06
#define HDD_PORT_INDEX_STATUS				0x07
#define HDD_PORT_INDEX_COMMAND				0x07
#define HDD_PORT_INDEX_DIGITALOUTPUT		0x206

// for Command Register

#define HDD_COMMAND_READ					0x20
#define HDD_COMMAND_WRITE					0x30
#define HDD_COMMAND_IDENTIFY				0xEC

// for Status Register

#define HDD_STATUS_ERROR					0x01
#define HDD_STATUS_INDEX					0x02
#define HDD_STATUS_CORRECTDDATA				0x04
#define HDD_STATUS_DATAREQUEST				0x08
#define HDD_STATUS_SEEKCOMPLETE				0x10
#define HDD_STATUS_WRITEFAULT				0x20
#define HDD_STATUS_READY					0x40
#define HDD_STATUS_BUSY						0x80

// for Drive/Head Register

#define HDD_DRIVEANDHEAD_LBA				0xE0
#define HDD_DRIVEANDHEAD_SLAVE				0x10

// for Digital Output Register

#define HDD_DIGITALOUTPUT_RESET				0x04
#define HDD_DIGITALOUTPUT_DISABLEINTERRUPT	0x01


// Waiting time for response of HDD

#define HDD_WAITTIME						500

// Readable/Writable Sector Count at one time

#define HDD_MAXBULKSECTORCOUNT				256





// ********** Structure **********

#pragma pack(push, 1)

typedef struct kHDDInformationStruct
{
	// Configuration Value
	WORD wConfiguration;

	// Number of Cylinder
	WORD wNumberOfCylinder;				// important in CHS Address Mode
	WORD wReserved1;

	// Number of Head
	WORD wNumberOfHead;					// important in CHS Address Mode
	WORD wUnformattedBytesPerTrack;	
	WORD wUnformattedBytesPerSector;

	// Number of Sector per Cylider
	WORD wNumberOfSectorPerCylinder;	// important in CHS Address Mode
	WORD wInterSectorGap;
	WORD wBytesInPhaseLock;
	WORD wNumberOfVendorUniqueStatusWord;

	// HDD Serial Number
	WORD vwSerialNumber[10];
	WORD wControllerType;
	WORD wBufferSize;
	WORD wNumberOfECCBytes;
	WORD vwFirmwareRevision[4];

	// HDD Model Number
	WORD vwModelNumber[20];
	WORD vwReserved2[13];
	
	// Total Sector Count
	DWORD dwTotalSectors;				// important in LBA Address Mode
	WORD vwReserved3[196];
} HDDINFORMATION;

#pragma pack(pop)


typedef struct kHDDManangerStruct
{
	// HDD Existence, Readable/Writable
	BOOL bHDDDetected;
	BOOL bCanWrite;

	// Interrupt Occurence, Synchronization Object
	volatile BOOL bPrimaryInterruptOccur;			// Primary PATA Port
	volatile BOOL bSecondaryInterruptOccur;			// Secondary PATA Port
	MUTEX stMutex;									// Sync Obj

	// HDD Information
	HDDINFORMATION stHDDInformation;
} HDDMANAGER;





// ********** Function **********

BOOL kInitializeHDD(void);
BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
int  kReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
int  kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag);

static void kSwapByteInWord(WORD* pwData, int iWordCount);
static BYTE kReadHDDStatus(BOOL bPrimary);
static BOOL kIsHDDBusy(BOOL bPrimary);
static BOOL kIsHDDReady(BOOL bPrimary);
static BOOL kWaitForHDDNoBusy(BOOL bPrimary);
static BOOL kWaitForHDDReady(BOOL bPrimary);
static BOOL kWaitForHDDInterrupt(BOOL bPrimary);



#endif
