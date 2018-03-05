#include "Mouse.h"
#include "Keyboard.h"
#include "Queue.h"
#include "AssemblyUtility.h"
#include "Utility.h"

static MOUSEMANAGER gs_stMouseManager = { 0, };

static QUEUE gs_stMouseQueue;
static MOUSEDATA gs_vstMouseQueueBuffer[MOUSE_MAXQUEUECOUNT];

BOOL kInitializeMouse(void)
{
	kInitializeQueue(&gs_stMouseQueue, gs_vstMouseQueueBuffer, MOUSE_MAXQUEUECOUNT,
			sizeof(MOUSEDATA));

	kInitializeSpinLock(&(gs_stMouseManager.stSpinLock));

	if (kActivateMouse() == TRUE)
	{
		kEnableMouseInterrupt();
		return TRUE;
	}
	return FALSE;
}

BOOL kActivateMouse(void)
{
	int i;
	BOOL bPreviousInterrupt;
	BOOL bResult;

	bPreviousInterrupt = kSetInterruptFlag(FALSE);

	// activate mouse device controller
	kOutPortByte(0x64, 0xA8);

	// send command to out to mouse
	kOutPortByte(0x64, 0xD4);

	while (kWaitForInputBufferEmpty() == FALSE);

	// activate mouse
	kOutPortByte(0x60, 0xF4);

	bResult = kWaitForACKAndPutOtherScanCode();

	kSetInterruptFlag(bPreviousInterrupt);
	return bResult;
}

void kEnableMouseInterrupt(void)
{
	BYTE bOutputPortData;
	int i;

	// send reading command byte command
	kOutPortByte(0x64, 0x20);

	while (kWaitForOutputBufferFull() == FALSE);
	bOutputPortData = kInPortByte(0x60);

	// enable mouse interrupt
	bOutputPortData |= 0x02;

	kOutPortByte(0x64, 0x60);

	while (kWaitForInputBufferEmpty() == FALSE);
	kOutPortByte(0x60, bOutputPortData);
}

BOOL kAccumulateMouseDataAndPutQueue(BYTE bMouseData)
{
	BOOL bResult = TRUE;

	switch (gs_stMouseManager.iByteCount)
	{
		case 0:
			gs_stMouseManager.stCurrentData.bButtonStatusAndFlag = bMouseData;
			gs_stMouseManager.iByteCount++;
			break;
			
		case 1:
			gs_stMouseManager.stCurrentData.bXMovement = bMouseData;
			gs_stMouseManager.iByteCount++;
			break;

		case 2:
			gs_stMouseManager.stCurrentData.bYMovement = bMouseData;
			gs_stMouseManager.iByteCount++;
			break;

		default:
			gs_stMouseManager.iByteCount = 0;
			break;
	}

	if (gs_stMouseManager.iByteCount >= 3)
	{
		kLockForSpinLock(&(gs_stMouseManager.stSpinLock));
		bResult = kPutQueue(&gs_stMouseQueue, &gs_stMouseManager.stCurrentData);
		kUnlockForSpinLock(&(gs_stMouseManager.stSpinLock));
		gs_stMouseManager.iByteCount = 0;
	}

	return bResult;
}

BOOL kGetMouseDataFromMouseQueue(BYTE* pbButtonStatus, int* piRelativeX,
		int* piRelativeY)
{
	MOUSEDATA stData;
	BOOL bResult;

	if (kIsQueueEmpty(&(gs_stMouseQueue)) == TRUE)
		return FALSE;

	kLockForSpinLock(&(gs_stMouseManager.stSpinLock));
	bResult = kGetQueue(&(gs_stMouseQueue), &stData);
	kUnlockForSpinLock(&(gs_stMouseManager.stSpinLock));

	if (bResult == FALSE)
		return FALSE;

	*pbButtonStatus = stData.bButtonStatusAndFlag & 0x7;

	*piRelativeX = stData.bXMovement & 0xFF;
	if (stData.bButtonStatusAndFlag & 0x10)
	{	// if x is negative
		*piRelativeX |= (0xFFFFFF00);
	}

	*piRelativeY = stData.bYMovement & 0xFF;
	if (stData.bButtonStatusAndFlag & 0x20)
	{	// if y is negative
		*piRelativeY |= (0xFFFFFF00);
	}

	// y direction and coordination is reversed
	*piRelativeY = -*piRelativeY;
	return TRUE;
}

BOOL kIsMouseDataInOutputBuffer(void)
{
	// read status register and check AUXB bit (bit 5)
	if (kInPortByte(0x64) & 0x20)
		return TRUE;
	return FALSE;
}

