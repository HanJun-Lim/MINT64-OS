#include "LocalAPIC.h"
#include "MPConfigurationTable.h"


// return Memory Mapped I/O Address of Local APIC
QWORD kGetLocalAPICBaseAddress(void)
{
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;

	// get Local APIC Memory Mapped I/O Address from MP Configuration Table Header
	pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;

	return pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
}


// Activate Local APIC 
// 		by setting APIC Software Enable/Disable Field in Spurious Interrupt Vector Register
void kEnableSoftwareLocalAPIC(void)
{
	QWORD qwLocalAPICBaseAddress;

	
	// get Local APIC Memory Mapped I/O Address from MP Configuration Table Header
	qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();

	
	// activate Local APIC by setting APIC Software Enable/Disable bit (bit 8)
	// in Spurious Interrupt Vector Register (0xFEE000F0 : Base Address + SVR Offset)
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_SVR) |= 0x100;
}



// Send End Of Interrupt to Local APIC
void kSendEOIToLocalAPIC(void)
{
	QWORD qwLocalAPICBaseAddress;


	// use Local APIC Memory Map Address stored in MP Configuration Table Header
	qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();

	
	// send EOI by setting 0x00 to EOI Register (0xFEE000B0)
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_EOI) = 0;
}


// Set Task Priority Register
void kSetTaskPriority(BYTE bPriority)
{
	QWORD qwLocalAPICBaseAddress;
	

	// use Local APIC Memory Map Address stored in MP Configuration Table Header
	qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();

	
	// send Priority value to Task Priority Register (0xFEE00080)
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_TASKPRIORITY) = bPriority;
}


void kInitializeLocalVectorTable(void)
{
	QWORD qwLocalAPICBaseAddress;
	DWORD dwTempValue;


	// use Local APIC Memory Map Address stored in MP Configuration Table Header
	qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();


	// disable Timer Interrupt by adding Mask bit to LVT Timer Register (0xFEE00320)
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_TIMER) |= APIC_INTERRUPT_MASK;

	// disable LINT0 Interrupt by adding Mask bit to LVT LINT0 Register (0xFEE00350)
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_LINT0) |= APIC_INTERRUPT_MASK;

	// make Non-Maskable Interrupt (NMI) occur by setting LVT LINT1 Register (0xFEE00360)
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_LINT1) =
		APIC_TRIGGERMODE_EDGE | APIC_POLARITY_ACTIVEHIGH | APIC_DELIVERYMODE_NMI;

	// disable Error Interrupt by adding Mask bit to LVT Error Regitser (0xFEE00370)
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ERROR) |= APIC_INTERRUPT_MASK;

	// disable Performance Monitoring Counter Interrupt 
	// by adding Mask bit to LVT Performance Monitoring Counter Register (0xFEE00340)
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_PERFORMANCEMONITORINGCOUNTER) |= APIC_INTERRUPT_MASK;

	// disable Thermal Sensor Interrupt by adding Mask bit to LVT Thermal Sensor Regitser (0xFEE00330)
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_THERMALSENSOR) |= APIC_INTERRUPT_MASK;
}



