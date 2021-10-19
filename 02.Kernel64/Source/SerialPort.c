#include "SerialPort.h"
#include "Utility.h"


static SERIALMANAGER gs_stSerialManager;


// Initialize Serial Port
void kInitializeSerialPort(void)
{
	WORD wPortBaseAddress;
	

	// initialize Mutex
	kInitializeMutex(&(gs_stSerialManager.stLock));
	
	
	// set Base Address as COM1
	wPortBaseAddress = SERIAL_PORT_COM1;


	// send 0 to Interrupt Enable Register (0x3F9) to disable all interrupts
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_INTERRUPTENABLE, 0);


	// set Communication Speed as 115200
	// 		set DLAB bit (bit 7) on Line Control Register (0x3FB) to access Divisor Latch Register
	// 		* Transmit/Receive Register -> LSB Divisor Latch Register
	//		* Interrupt Enable Register -> MSB Divisor Latch Register
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_LINECONTROL, SERIAL_LINECONTROL_DLAB);

	// 		send low 8 bit to LSB Divisor Latch Register
	//		send high 8 bit to MSB Divisor Latch Register
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_DIVISORLATCHLSB, SERIAL_DIVISORLATCH_115200);
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_DIVISORLATCHMSB, SERIAL_DIVISORLATCH_115200 >> 8);


	// define how to perform I/O
	// 		Performing I/O : 8-bit in one frame, No Parity, 1 Stop bit
	// 		clear DLAB bit (bit 7) on Line Control Regiser (0x3FB) to access Data I/O, Interrupt Enable Register
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_LINECONTROL,
				SERIAL_LINECONTROL_8BIT | SERIAL_LINECONTROL_NOPARITY | SERIAL_LINECONTROL_1BITSTOP);


	// set FIFO Interrupt Occur Time to 14 bytes
	kOutPortByte(wPortBaseAddress + SERIAL_PORT_INDEX_FIFOCONTROL,
				SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO);
}


// Is Serial Transmitter Buffer empty?
static BOOL kIsSerialTransmitterBufferEmpty(void)
{
	BYTE bData;


	// Read Line Status Register (Port 0x3FD) and check TBE Bit (bit 1)
	// to check if Transmit FIFO empty or not
	bData = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);

	if( (bData & SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) == SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY )
	{
		return TRUE;
	}

	return FALSE;
}


// Send data to Serial Port
void kSendSerialData(BYTE* pbBuffer, int iSize)
{
	int iSentByte;
	int iTempSize;
	int j;


	///////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stSerialManager.stLock));
	///////////////////////////////
	
	
	iSentByte = 0;

	while(iSentByte < iSize)
	{
		// if there's Data in Transmit FIFO, wait for transmission completion	
		while(kIsSerialTransmitterBufferEmpty() == FALSE)
		{
			kSleep(0);
		}

		
		// iTempSize = MIN(Remaining bytes to send, Max FIFO size)
		iTempSize = MIN(iSize - iSentByte, SERIAL_FIFOMAXSIZE);

		for(j=0; j<iTempSize; j++)
		{
			kOutPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_TRANSMITBUFFER, pbBuffer[iSentByte + j]);
		}

		iSentByte += iTempSize;
	}


	///////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stSerialManager.stLock));
	///////////////////////////////
}


// Is there data in Receive FIFO?
static BOOL kIsSerialReceiveBufferFull(void)
{
	BYTE bData;

	
	// Read Line Status Register (Port 0x3FD) and check RxRD bit (bit 0)
	// to check if Data is in Receive FIFO or not
	bData = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);

	if( (bData & SERIAL_LINESTATUS_RECEIVEDATAREADY) == SERIAL_LINESTATUS_RECEIVEDATAREADY )
	{
		return TRUE;
	}

	return FALSE;
}


// Read data from Serial Port
int kReceiveSerialData(BYTE* pbBuffer, int iSize)
{
	int i;


	///////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stSerialManager.stLock));
	///////////////////////////////
	
	
	for(i=0; i<iSize; i++)
	{
		// if there's no Data in Buffer, pause
		if(kIsSerialReceiveBufferFull() == FALSE)
		{
			break;
		}

		pbBuffer[i] = kInPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_RECEIVEBUFFER);
	}
	

	///////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stSerialManager.stLock));
	///////////////////////////////


	// return read data count
	return i;
}


// Initialize Serial FIFO
void kClearSerialFIFO(void)
{
	///////////////////////////////
	// CRITICAL SECTION START
	kLock(&(gs_stSerialManager.stLock));
	///////////////////////////////
	
	
	// clear Transmit/Receive Buffer, and send configuration value to FIFO Control Register (0x3FA)
	// to occur interrupt when 14 Bytes filled in Buffer
	kOutPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_FIFOCONTROL,
				SERIAL_FIFOCONTROL_FIFOENABLE 		| SERIAL_FIFOCONTROL_14BYTEFIFO |
				SERIAL_FIFOCONTROL_CLEARRECEIVEFIFO | SERIAL_FIFOCONTROL_CLEARTRANSMITFIFO);


	///////////////////////////////
	// CRITICAL SECTION END
	kUnlock(&(gs_stSerialManager.stLock));
	///////////////////////////////
}



