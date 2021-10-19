#include "PIT.h"


// Initialize PIT
void kInitializePIT(WORD wCount, BOOL bPeriodic)
{
	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);

	if(bPeriodic == TRUE)
		kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);


	kOutPortByte(PIT_PORT_COUNTER0, wCount);		// LSB
	kOutPortByte(PIT_PORT_COUNTER0, wCount >> 8);	// MSB
}


// return current value of Counter 0
WORD kReadCounter0(void)
{
	BYTE bHighByte, bLowByte;
	WORD wTemp = 0;


	// LATCH Command for Counter 0
	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

	// read twice (LSB -> MSB)
	bLowByte  = kInPortByte(PIT_PORT_COUNTER0);
	bHighByte = kInPortByte(PIT_PORT_COUNTER0);

	
	wTemp = bHighByte;
	wTemp = (wTemp << 8) | bLowByte;

	return wTemp;
}


// wait certain time by setting Counter 0
// 		PIT setting is changed when Function is called, so need to reconfigure PIT Controller after calling
// 		"Correct" measurement requires Interrupt deactivation
//		possible to measure up to 50ms (1,193,182(f) * 0.050(s) = 59,659.1(f))
void kWaitUsingDirectPIT(WORD wCount)
{
	WORD wLastCounter0;
	WORD wCurrentCounter0;

	kInitializePIT(0, TRUE);


	wLastCounter0 = kReadCounter0();

	while(1)
	{
		wCurrentCounter0 = kReadCounter0();

		if(((wLastCounter0 - wCurrentCounter0) & 0xFFFF) >= wCount)
			break;
	}
}


