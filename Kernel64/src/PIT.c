#include "PIT.h"
#include "AssemblyUtility.h"

void kInitializePIT(WORD wCount, BOOL bPeriodic)
{
	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);

	if (bPeriodic == TRUE)
	{
		kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
	}

	// LSB -> MSB
	kOutPortByte(PIT_PORT_COUNTER0, wCount);
	kOutPortByte(PIT_PORT_COUNTER0, wCount >> 8);
}

WORD kReadCounter0(void)
{
	BYTE bHighByte, bLowByte;

	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

	bLowByte = kInPortByte(PIT_PORT_COUNTER0);
	bHighByte = kInPortByte(PIT_PORT_COUNTER0);

	return ((bHighByte << 8) | bLowByte);
}

void kWaitUsingDirectPIT(WORD wCount)
{
	WORD wLastCounter0;

	kInitializePIT(0, TRUE);

	wLastCounter0 = kReadCounter0();
	while (1)
	{
		if (((wLastCounter0 - kReadCounter0()) % 0xFFFF) >= wCount)
			break;
	}
}
