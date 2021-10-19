#include "PIC.h"


// initialize PIC
void kInitializePIC(void)
{
	// ----------------------------------------------
	// Master PIC Controll Initialization
	// ----------------------------------------------
	kOutPortByte(PIC_MASTER_PORT1, 0x11);					// ICW1 (port 0x20), IC4 bit on
	kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR);		// ICW2 (port 0x21), interrupt vector start - 0x20 (32)
	kOutPortByte(PIC_MASTER_PORT2, 0x04);					// ICW3 (port 0x21), Slave PIC connected on Pin 2 (bit)
	kOutPortByte(PIC_MASTER_PORT2, 0x01);					// ICW4 (port 0x21), uPM bit on


	// ----------------------------------------------
	// Slave PIC Controll Initialization
	// ----------------------------------------------
	kOutPortByte(PIC_SLAVE_PORT1, 0x11);					// ICW1 (port 0xA0), IC4 bit on
	kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR+8);	// ICW2 (port 0xA1), interrupt vector start - 0x28 (40)
	kOutPortByte(PIC_SLAVE_PORT2, 0x02);					// ICW3 (port 0xA1), PIN 2 connected with Master PIC (integer)
	kOutPortByte(PIC_SLAVE_PORT2, 0x01);					// ICW4 (port 0xA1), uPM bit on
}


// mask interrupt not to occur interrupt
void kMaskPICInterrupt(WORD wIRQBitmask)
{
	kOutPortByte(PIC_MASTER_PORT2, (BYTE)wIRQBitmask);			// OCW1 (port 0x21), IRQ 0 ~ IRQ 7
	kOutPortByte(PIC_SLAVE_PORT2 , (BYTE)(wIRQBitmask >> 8));	// OCW1 (port 0xA1), IRQ 8 ~ IRQ 15
}


// send End Of Interrupt to PIC
// 		Master PIC : EOI to Master
// 		Slave  PIC : EOI to Master and Slave
void kSendEOIToPIC(int iIRQNumber)
{
	kOutPortByte(PIC_MASTER_PORT1, 0x20);		// OCW2 (port 0x20)	
	
	if(iIRQNumber >= 8)
		kOutPortByte(PIC_SLAVE_PORT1, 0x20);
}


