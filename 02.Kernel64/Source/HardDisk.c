#include "HardDisk.h"


static HDDMANAGER gs_stHDDManager;


BOOL kInitializeHDD(void)
{
	kInitializeMutex(&(gs_stHDDManager.stMutex));

	
	gs_stHDDManager.bPrimaryInterruptOccur   = FALSE;
	gs_stHDDManager.bSecondaryInterruptOccur = FALSE;

	
	// Activate HDD Controller Interrupt 
	//     by sending 0 to Digital Output Register (0x3F6/0x376) of Primary/Secondary PATA
	kOutPortByte(HDD_PORT_PRIMARYBASE   + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
	kOutPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);


	// Request HDD Information (Primary - Master)
	if( kReadHDDInformation(TRUE, TRUE, &(gs_stHDDManager.stHDDInformation)) == FALSE )
	{
		gs_stHDDManager.bHDDDetected = FALSE;
		gs_stHDDManager.bCanWrite	 = FALSE;
		return FALSE;
	}

	
	// can be used only in VMware, QEMU
	gs_stHDDManager.bHDDDetected = TRUE;

	if( kMemCmp(gs_stHDDManager.stHDDInformation.vwModelNumber, "VMware", 6) == 0 || 
		kMemCmp(gs_stHDDManager.stHDDInformation.vwModelNumber, "QEMU",   4) == 0 )
	{
		gs_stHDDManager.bCanWrite = TRUE;
	}
	else
	{
		gs_stHDDManager.bCanWrite = FALSE;
	}


	return TRUE;
}


static BYTE kReadHDDStatus(BOOL bPrimary)
{
	if(bPrimary == TRUE)
	{
		// receive from Primary PATA Port - Status Register (0x1F7)
		return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
	}
	
	// receive from Secondary PATA Port - Status Register (0x177)
	return kInPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_STATUS);
}


static BOOL kWaitForHDDNoBusy(BOOL bPrimary)
{
	QWORD qwStartTickCount;
	BYTE  bStatus;

	// Waiting Start
	qwStartTickCount = kGetTickCount();

	while( (kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME )
	{
		bStatus = kReadHDDStatus(bPrimary);

		// when no busy (if BUSY Bit (Bit 7) is not set..)
		if( (bStatus & HDD_STATUS_BUSY) != HDD_STATUS_BUSY )
		{
			return TRUE;
		}

		kSleep(1);
	}

	return FALSE;
}


static BOOL kWaitForHDDReady(BOOL bPrimary)
{
	QWORD qwStartTickCount;
	BYTE  bStatus;

	// Waiting Start
	qwStartTickCount = kGetTickCount();

	while( (kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME )
	{
		bStatus = kReadHDDStatus(bPrimary);

		// when ready (if READY Bit (bit 6) is set...)
		if( (bStatus & HDD_STATUS_READY) == HDD_STATUS_READY )
		{
			return TRUE;
		}

		kSleep(1);
	}

	return FALSE;
}



void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag)
{
	if(bPrimary == TRUE)
	{
		gs_stHDDManager.bPrimaryInterruptOccur = bFlag;
	}
	else
	{
		gs_stHDDManager.bSecondaryInterruptOccur = bFlag;
	}
}


static BOOL kWaitForHDDInterrupt(BOOL bPrimary)
{
	QWORD qwTickCount;

	// Waiting Start
	qwTickCount = kGetTickCount();

	while( (kGetTickCount() - qwTickCount) <= HDD_WAITTIME )
	{
		if( (bPrimary == TRUE) && (gs_stHDDManager.bPrimaryInterruptOccur == TRUE) )
		{
			return TRUE;
		}
		else if( (bPrimary == FALSE) && (gs_stHDDManager.bSecondaryInterruptOccur == TRUE) )
		{
			return TRUE;
		}
	}
	
	return FALSE;
}



BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation)
{
	WORD  wPortBase;
	QWORD qwLastTickCount;
	BYTE  bStatus;
	BYTE  bDriveFlag;
	int   i;
	WORD  wTemp;
	BOOL  bWaitResult;


	// set Base Address referring to bPrimary (Primary/Secondary)
	if(bPrimary == TRUE)
	{
		wPortBase = HDD_PORT_PRIMARYBASE;		// Primary PATA Base : 0x1F0
	}
	else
	{
		wPortBase = HDD_PORT_SECONDARYBASE;		// Secondary PATA Base : 0x170
	}


	////////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stHDDManager.stMutex));
	////////////////////////////////////////
	

	// if there's command executing, wait for No Busy
	if(kWaitForHDDNoBusy(bPrimary) == FALSE)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stHDDManager.stMutex));
		
		return FALSE;
	}
	
	
	// ============================================================
	// set registers related to LBA Address, Drive/Head
	// 		Drive, Head information needed
	// ============================================================
	
	// set Drive, Head Data
	if(bMaster == TRUE)
	{
		// Master Drive - set LBA Flag
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;
	}
	else
	{
		// Slave Drive - set LBA Flag, Slave Flag
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}

	// send bDriveFlag to Drive/Head Register (0x1F6 or 0x176)
	kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag);


	// ============================================================
	// send command and then wait for interrupt
	// ============================================================
	
	if(kWaitForHDDReady(bPrimary) == FALSE)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stHDDManager.stMutex));

		return FALSE;
	}

	// Init Interrupt Flag
	kSetHDDInterruptFlag(bPrimary, FALSE);

	// send Identify Command (0xEC) to Command Register (0x1F7 or 0x177)
	kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);

	// Wait for interrupt until completion
	//		Exit when No interrupt occured, or Error occured
	bWaitResult = kWaitForHDDInterrupt(bPrimary);
	bStatus = kReadHDDStatus(bPrimary);

	if( (bWaitResult == FALSE) || (bStatus & HDD_STATUS_ERROR == HDD_STATUS_ERROR) )
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stHDDManager.stMutex));

		return FALSE;
	}


	// ============================================================
	// receive data
	// ============================================================
	
	for(i=0; i<512/2; i++)
	{
		((WORD*)pstHDDInformation)[i] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
	}

	// swap Byte Ordering when String
	kSwapByteInWord(pstHDDInformation->vwModelNumber , sizeof(pstHDDInformation->vwModelNumber ) / 2);
	kSwapByteInWord(pstHDDInformation->vwSerialNumber, sizeof(pstHDDInformation->vwSerialNumber) / 2);


	////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stHDDManager.stMutex));
	////////////////////////////////////////
	
	return TRUE;
}


static void kSwapByteInWord(WORD* pwData, int iWordCount)
{
	int i;
	WORD wTemp;

	for(i=0; i<iWordCount; i++)
	{
		wTemp = pwData[i];
		pwData[i] = (wTemp >> 8) | (wTemp << 8);
	}
}



// Max readable sector at one time : 256 sector
// 		return count of read sector
int kReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer)
{
	WORD wPortBase;
	int  i, j;
	BYTE bDriveFlag;
	BYTE bStatus;
	long lReadCount = 0;
	BOOL bWaitResult;

	
	// Sector count range check, Max : 256
	// 		Unable to read when...
	// 		1. No HDD detected
	// 		2. iSectorCount (Param 4) is under 0
	// 		3. iSectorCount (Param 4) is over 256 (Max readable sector)
	// 		4. out of Logical Block Addressing Range
	if( (gs_stHDDManager.bHDDDetected == FALSE) ||
		(iSectorCount <= 0) || (256 < iSectorCount) ||
		( (dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors) )
	{
		return 0;
	}


	// set I/O Port Base Address by PATA Port
	if(bPrimary == TRUE)
	{
		wPortBase = HDD_PORT_PRIMARYBASE;
	}
	else
	{
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	
	////////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stHDDManager.stMutex));
	////////////////////////////////////////
	

	if(kWaitForHDDNoBusy(bPrimary) == FALSE)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stHDDManager.stMutex));

		return FALSE;
	}


	// ============================================================
	// set Data Register
	// 		LBA Mode : assign address in order Sector Number -> Cylinder Number -> Head Number
	// ============================================================	
	
	// Sector Count Register  (0x1F2 or 0x172) = iSectorCount
	// Sector Number Register (0x1F3 or 0x173) = LBA[ 7: 0]
	// Cylinder LSB Register  (0x1F4 or 0x174) = LBA[15: 8]
	// Cylinder MSB Register  (0x1F5 or 0x175) = LBA[23:16]
	// Drive/Head Register    (0x1F6 or 0x176) = LBA[27:24]
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT , iSectorCount);
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA		  );
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB , dwLBA >> 8  );
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB , dwLBA >> 16 );

	if(bMaster == TRUE)
	{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;
	}
	else
	{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}

	kOutPortByte( wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F) );


	// ============================================================
	// send Command
	// ============================================================
	
	if(kWaitForHDDReady(bPrimary) == FALSE)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stHDDManager.stMutex));
		
		return FALSE;
	}

	kSetHDDInterruptFlag(bPrimary, FALSE);

	// send Read command (0x20) to Command Register (0x1F7 or 0x177)
	kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ);


	// ============================================================
	// wait for interrupt and then receive Data
	// ============================================================

	for(i=0; i<iSectorCount; i++)
	{
		// if Error Occured, exit
		bStatus = kReadHDDStatus(bPrimary);

		if( (bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR )
		{	
			kPrintf("Error Occured\n");

			// CRITICAL SECTION END
			kUnlock(&(gs_stHDDManager.stMutex));

			return i;
		}

		// if DATAREQUEST bit is not set, wait for Data Reception 
		if( (bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST )
		{
			bWaitResult = kWaitForHDDInterrupt(bPrimary);
			kSetHDDInterruptFlag(bPrimary, FALSE);
			
			if(bWaitResult == FALSE)
			{
				kPrintf("Interrupt Not Occured\n");

				// CRITICAL SECTION END
				kUnlock(&(gs_stHDDManager.stMutex));

				return FALSE;
			}
		}


		// Read one sector
		for(j=0; j<512/2; j++)
		{
			((WORD*)pcBuffer)[lReadCount++] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
		}

	}	// SectorCount loop end

	
	////////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stHDDManager.stMutex));
	////////////////////////////////////////
	
	
	// Return read sector count
	return i;
}



// Max writable sector at one time : 256 sector
// 		return count of written sector
int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer)
{
	WORD wPortBase;
	WORD wTemp;
	int i, j;
	BYTE bDriveFlag;
	BYTE bStatus;
	long lReadCount = 0;
	BOOL bWaitResult;

	// Sector count range check, Max : 256
	// 		Unable to write when...
	// 		1. HDD is in non-writable state
	// 		2. iSectorCount (Param 4) is under 0
	// 		3. iSectorCount (Param 4) is over 256 (Max readable sector)
	// 		4. out of Logical Block Addressing Range
	if( (gs_stHDDManager.bCanWrite == FALSE) ||
		(iSectorCount <= 0) || (256 < iSectorCount) ||
		( (dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors ) )
	{
		return 0;
	}


	if(bPrimary == TRUE)
	{
		wPortBase = HDD_PORT_PRIMARYBASE;
	}
	else
	{
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	
	if(kWaitForHDDNoBusy(bPrimary) == FALSE)
	{
		return FALSE;
	}

	//////////////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stHDDManager.stMutex));
	//////////////////////////////////////
	
	
	
	// ==================================================================
	// Set Data Register
	// 		LBA Mode : assign address in order Sector Number -> Cylinder Number -> Head Number
	// ==================================================================
	
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT , iSectorCount);
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA		  );
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB , dwLBA >> 8  );
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB , dwLBA >> 16 );

	if(bMaster == TRUE)
	{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;
	}
	else
	{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}
		
	kOutPortByte( wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F) );

	// ==================================================================
	// Send command and wait until Sending data is available
	// ==================================================================
	
	if(kWaitForHDDReady(bPrimary) == FALSE)
	{
		// CRITICAL SECTION END
		kUnlock(&(gs_stHDDManager.stMutex));

		return FALSE;
	}

	kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE);

	while(1)
	{
		bStatus = kReadHDDStatus(bPrimary);

		// if error occured, exit
		if( (bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR )
		{
			// CRITICAL SECTION END
			kUnlock(&(gs_stHDDManager.stMutex));

			return 0;
		}

		// if DATAREQUEST is set, able to send data
		if( (bStatus & HDD_STATUS_DATAREQUEST) == HDD_STATUS_DATAREQUEST )
		{
			// exit Wait
			break;
		}

		kSleep(1);
	}


	// ==================================================================
	// Send data and then wait for Interrupt
	// ==================================================================
	
	for(i=0; i<iSectorCount; i++)
	{
		kSetHDDInterruptFlag(bPrimary, FALSE);

		// Write one sector
		for(j=0; j<512/2; j++)
		{
			kOutPortWord(wPortBase + HDD_PORT_INDEX_DATA, ((WORD*)pcBuffer)[lReadCount++]);
		}

		// if error occured, exit
		bStatus = kReadHDDStatus(bPrimary);

		if( (bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR )
		{
			// CRITICAL SECTION END
			kUnlock(&(gs_stHDDManager.stMutex));

			return i;
		}

		// if DATAREQUEST is not set, wait for completed Data handling (wait for interrupt)
		if( (bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST )
		{
			// wait for completed Data Handling (interrupt)
			bWaitResult = kWaitForHDDInterrupt(bPrimary);

			kSetHDDInterruptFlag(bPrimary, FALSE);

			if(bWaitResult == FALSE)
			{
				// CRITICAL SECTION END
				kUnlock(&(gs_stHDDManager.stMutex));

				return FALSE;
			}
		}
	}	// SectorCount loop end


	//////////////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stHDDManager.stMutex));
	//////////////////////////////////////
	

	// return written sector count
	return i;
}



