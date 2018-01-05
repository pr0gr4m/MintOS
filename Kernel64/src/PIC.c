#include "PIC.h"
#include "AssemblyUtility.h"

void kInitializePIC(void)
{
	// Init Master PIC
	// ICW1, IC4 bit 1
	kOutPortByte(PIC_MASTER_PORT1, 0x11);
	// ICW2, interrupt vector (0x20)
	kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR);
	// ICW3, connect pin to slave is pin 2 (bit 0x04)
	kOutPortByte(PIC_MASTER_PORT2, 0x04);
	// ICW4, uPM bit 1
	kOutPortByte(PIC_MASTER_PORT2, 0x01);

	// Init Slave PIC
	// ICW1, IC4 bit 1
	kOutPortByte(PIC_SLAVE_PORT1, 0x11);
	// ICW2, interrupt vector (0x20 + 8)
	kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8);
	// ICW3, connect pin to master is pin 2 (integer 0x02)
	kOutPortByte(PIC_SLAVE_PORT2, 0x02);
	// ICW4, uPM bit 1
	kOutPortByte(PIC_SLAVE_PORT2, 0x01);
}

void kMaskPICInterrupt(WORD wIRQBitmask)
{
	// set IMR
	kOutPortByte(PIC_MASTER_PORT2, (BYTE)wIRQBitmask);
	kOutPortByte(PIC_SLAVE_PORT2, (BYTE)(wIRQBitmask >> 8));
}

void kSendEOIToPIC(int iIRQNumber)
{
	// OCW2, EOI bit 1
	kOutPortByte(PIC_MASTER_PORT1, 0x20);

	// if slave PIC interrupt, send EOI to master and slave
	if (iIRQNumber >= 8)
	{
		kOutPortByte(PIC_SLAVE_PORT1, 0x20);
	}
}
