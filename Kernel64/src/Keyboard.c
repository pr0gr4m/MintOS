#include "Types.h"
#include "AssemblyUtility.h"
#include "Utility.h"
#include "Keyboard.h"
#include "Queue.h"
#include "Mouse.h"

/*
 * Functions about keyboard controller
 */

// return whether the output buffer(port 0x60) has received data
BOOL kIsOutputBufferFull(void)
{
	// If the output buffer status bit is set to 1 in the status register,
	// there is a data sent by the keyboard in the output buffer
	if (kInPortByte(0x64) & 0x01)
		return TRUE;
	return FALSE;
}

// return whether the input buffer(port 0x60) has data written by the processor
BOOL kIsInputBufferFull(void)
{
	// If the input buffer status bit is set to 1 in the value read from the status register,
	// the keyboard has not yet taken the data
	if (kInPortByte(0x64) & 0x02)
		return TRUE;
	return FALSE;
}

BOOL kWaitForACKAndPutOtherScanCode(void)
{
	int i, j;
	BYTE bData;
	BOOL bResult = FALSE;
	BOOL bMouseData;

	for (i = 0; i < 100; i++)
	{
		while (kWaitForOutputBufferFull() == FALSE);

		if (kIsMouseDataInOutputBuffer() == TRUE)
			bMouseData = TRUE;
		else
			bMouseData = FALSE;

		bData = kInPortByte(0x60);
		if (bData == 0xFA)
		{
			bResult = TRUE;
			break;
		}
		else
		{
			if (bMouseData == FALSE)
				kConvertScanCodeAndPutQueue(bData);
			else
				kAccumulateMouseDataAndPutQueue(bData);
		}
	}
	return bResult;
}

BOOL kWaitForInputBufferEmpty(void)
{
	int i;
	for (i = 0; i < 0xFFFF; i++)
	{
		if (kIsInputBufferFull() == FALSE)
			return TRUE;
	}
	return FALSE;
}

BOOL kWaitForOutputBufferFull(void)
{
	int i;
	for (i = 0; i < 0xFFFF; i++)
	{
		if (kIsOutputBufferFull() == TRUE)
			return TRUE;
	}
	return FALSE;
}

BOOL kActivateKeyboard(void)
{
	BOOL bPreviousInterrupt;
	BOOL bResult;

	// Disable Interrupt
	bPreviousInterrupt = kSetInterruptFlag(FALSE);

	// activate keyboard device to send keyboard activation command to control register
	kOutPortByte(0x64, 0xAE);

	while (kWaitForInputBufferEmpty() == FALSE);

	// send keyboard activation command to keyboard
	kOutPortByte(0x60, 0xF4);

	bResult = kWaitForACKAndPutOtherScanCode();

	kSetInterruptFlag(bPreviousInterrupt);

	return bResult;
}

// read key scan code from output buffer (port 0x60)
BYTE kGetKeyboardScanCode(void)
{
	while (kIsOutputBufferFull() == FALSE);
	return kInPortByte(0x60);
}

BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn)
{
	BOOL bPreviousInterrupt;
	BOOL bResult;
	BYTE bData;

	// Disable Interrupt
	bPreviousInterrupt = kSetInterruptFlag(FALSE);

	while (kWaitForInputBufferEmpty() == FALSE);

	// send LED status changed command to output buffer (port 0x60)
	kOutPortByte(0x60, 0xED);
	// wait for keyboard receive the command
	while (kWaitForInputBufferEmpty() == FALSE);

	bResult = kWaitForACKAndPutOtherScanCode();

	if (bResult == FALSE)
	{
		kSetInterruptFlag(bPreviousInterrupt);
		return FALSE;
	}

	kOutPortByte(0x60, (bCapsLockOn << 2) | (bNumLockOn << 1) | bScrollLockOn);
	// wait for keyboard receive the LED Data
	while (kWaitForInputBufferEmpty() == FALSE);

	bResult = kWaitForACKAndPutOtherScanCode();
	kSetInterruptFlag(bPreviousInterrupt);

	return bResult;
}

void kEnableA20Gate(void)
{
	BYTE bOutputPortData;
	
	// send a command (0xD0) to the control register to read the
	// output port value of the keyboard controller
	kOutPortByte(0x64, 0xD0);

	while (kWaitForOutputBufferFull() == FALSE);
	bOutputPortData = kInPortByte(0x60);

	// set A20 Gate bit
	bOutputPortData |= 0x01;

	while (kWaitForInputBufferEmpty() == FALSE);
	
	// send output port setting command(0xD1) to the command register
	kOutPortByte(0x64, 0xD1);

	kOutPortByte(0x60, bOutputPortData);
}

void kReboot(void)
{
	while (kWaitForInputBufferEmpty() == FALSE);
	kOutPortByte(0x64, 0xD1);
	kOutPortByte(0x60, 0x00);
	while (1);
}


/*
 * Functions to convert scan code to ASCII code
 */

// keyboard manager to manage keyboard status
static KEYBOARDMANAGER gs_stKeyboardManager = { 0, };


static QUEUE gs_stKeyQueue;
static KEYDATA gs_vstKeyQueueBuffer[KEY_MAXQUEUECOUNT];


static KEYMAPPINGENTRY gs_vstKeyMappingTable[KEY_MAPPINGTABLEMAXCOUNT] =
{
	{ KEY_NONE, KEY_NONE },		// 0
	{ KEY_ESC, KEY_ESC },		// 1
	{ '1', '!' },				// 2
	{ '2', '@' },				// 3
	{ '3', '#' },				// 4
	{ '4', '$' },				// 5
	{ '5', '%' },				// 6
	{ '6', '^' },				// 7
	{ '7', '&' },				// 8
	{ '8', '*' },				// 9
	{ '9', '(' },				// 10
	{ '0', ')' },				// 11
	{ '-', '_' },				// 12
	{ '=', '+' },				// 13
	{ KEY_BACKSPACE, KEY_BACKSPACE },	// 14
	{ KEY_TAB, KEY_TAB },		// 15
	{ 'q', 'Q' },				// 16
	{ 'w', 'W' },				// 17
	{ 'e', 'E' },				// 18
	{ 'r', 'R' },				// 19
	{ 't', 'T' },				// 20
	{ 'y', 'Y' },				// 21
	{ 'u', 'U' },				// 22
	{ 'i', 'I' },				// 23
	{ 'o', 'O' },				// 24
	{ 'p', 'P' },				// 25
	{ '[', '{' },				// 26
	{ ']', '}' },				// 27
	{ '\n', '\n' },				// 28
	{ KEY_CTRL, KEY_CTRL },		// 29
	{ 'a', 'A' },				// 30
	{ 's', 'S' },				// 31
	{ 'd', 'D' },				// 32
	{ 'f', 'F' },				// 33
	{ 'g', 'G' },				// 34
	{ 'h', 'H' },				// 35
	{ 'j', 'J' },				// 36
	{ 'k', 'K' },				// 37
	{ 'l', 'L' },				// 38
	{ ';', ':' },				// 39
	{ '\'', '\"' },				// 40
	{ '`', '~' },				// 41
	{ KEY_LSHIFT, KEY_LSHIFT },	// 42
	{ '\\', '|' },				// 43
	{ 'z', 'Z' },				// 44
	{ 'x', 'X' },				// 45
	{ 'c', 'C' },				// 46
	{ 'v', 'V' },				// 47
	{ 'b', 'B' },				// 48
	{ 'n', 'N' },				// 49
	{ 'm', 'M' },				// 50
	{ ',', '<' },				// 51
	{ '.', '>' },				// 52
	{ '/', '?' },				// 53
	{ KEY_RSHIFT, KEY_RSHIFT },	// 54
	{ '*', '*' },				// 55
	{ KEY_LALT, KEY_LALT },		// 56
	{ ' ', ' ' },				// 57
	{ KEY_CAPSLOCK, KEY_CAPSLOCK },		// 58
	{ KEY_F1, KEY_F1 },			// 59
	{ KEY_F2, KEY_F2 },			// 60
	{ KEY_F3, KEY_F3 },			// 61
	{ KEY_F4, KEY_F4 },			// 62
	{ KEY_F5, KEY_F5 },			// 63
	{ KEY_F6, KEY_F6 },			// 64
	{ KEY_F7, KEY_F7 },			// 65
	{ KEY_F8, KEY_F8 },			// 66
	{ KEY_F9, KEY_F9 },			// 67
	{ KEY_F10, KEY_F10 },		// 68
	{ KEY_NUMLOCK, KEY_NUMLOCK },		// 69
	{ KEY_SCROLLLOCK, KEY_SCROLLLOCK },	// 70
	{ KEY_HOME, '7' },			// 71
	{ KEY_UP, '8' },			// 72
	{ KEY_PAGEUP, '9' },		// 73
	{ '-', '-' },				// 74
	{ KEY_LEFT, '4' },			// 75
	{ KEY_CENTER, '5' },		// 76
	{ KEY_RIGHT, '6' },			// 77
	{ '+', '+' },				// 78
	{ KEY_END, '1' },			// 79
	{ KEY_DOWN, '2' },			// 80
	{ KEY_PAGEDOWN, '3' },		// 81
	{ KEY_INS, '0' },			// 82
	{ KEY_DEL, '.' },			// 83
	{ KEY_NONE, KEY_NONE },		// 84
	{ KEY_NONE, KEY_NONE },		// 85
	{ KEY_NONE, KEY_NONE },		// 86
	{ KEY_F11, KEY_F11 },		// 87
	{ KEY_F12, KEY_F12 },		// 88
};

BOOL kIsAlphabetScanCode(BYTE bScanCode)
{
	if ('a' <= gs_vstKeyMappingTable[bScanCode].bNormalCode &&
		gs_vstKeyMappingTable[bScanCode].bNormalCode <= 'z')
		return TRUE;
	return FALSE;
}

BOOL kIsNumberOrSymbolScanCode(BYTE bScanCode)
{
	if (2 <= bScanCode && bScanCode <= 53 && kIsAlphabetScanCode(bScanCode) == FALSE)
		return TRUE;
	return FALSE;
}

BOOL kIsNumberPadScanCode(BYTE bScanCode)
{
	if (71 <= bScanCode && bScanCode <= 83)
		return TRUE;
	return FALSE;
}

BOOL kIsUseCombinedCode(BYTE bScanCode)
{
	BYTE bDownScanCode;
	BOOL bUseCombinedKey = FALSE;

	bDownScanCode = bScanCode & 0x7F;

	if (kIsAlphabetScanCode(bDownScanCode) == TRUE)
	{	// alphabet keys are affected by shift and caps lock
		if (gs_stKeyboardManager.bShiftDown ^ gs_stKeyboardManager.bCapsLockOn)
			bUseCombinedKey = TRUE;
		else
			bUseCombinedKey = FALSE;
	}
	else if (kIsNumberOrSymbolScanCode(bDownScanCode) == TRUE)
	{	// number and symbol keys are affected by shift
		if (gs_stKeyboardManager.bShiftDown == TRUE)
			bUseCombinedKey = TRUE;
		else
			bUseCombinedKey = FALSE;
	}
	else if (kIsNumberPadScanCode(bDownScanCode) == TRUE &&
			gs_stKeyboardManager.bExtendedCodeIn == FALSE)
	{	// number pad keys are affected by num lock
		// use combination codes only when no extended key code is received
		if (gs_stKeyboardManager.bNumLockOn == TRUE)
			bUseCombinedKey = TRUE;
		else
			bUseCombinedKey = FALSE;
	}
	
	return bUseCombinedKey;
}

void kUpdateCombinationKeyStatusAndLED(BYTE bScanCode)
{
	BOOL bDown;
	BYTE bDownScanCode;
	BOOL bLEDStatusChanged = FALSE;

	if (bScanCode & 0x80)
	{
		bDown = FALSE;
		bDownScanCode = bScanCode & 0x7F;
	}
	else
	{
		bDown = TRUE;
		bDownScanCode = bScanCode;
	}

	// Shift scan code
	if (bDownScanCode == 42 || bDownScanCode == 54)
	{
		gs_stKeyboardManager.bShiftDown = bDown;
	}
	// Caps Lock
	else if (bDownScanCode == 58 && bDown == TRUE)
	{
		gs_stKeyboardManager.bCapsLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}
	// Num Lock
	else if (bDownScanCode == 69 && bDown == TRUE)
	{
		gs_stKeyboardManager.bNumLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}
	// Scroll Lock
	else if (bDownScanCode == 70 && bDown == TRUE)
	{
		gs_stKeyboardManager.bScrollLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}

	if (bLEDStatusChanged == TRUE)
		kChangeKeyboardLED(gs_stKeyboardManager.bCapsLockOn,
				gs_stKeyboardManager.bNumLockOn, gs_stKeyboardManager.bScrollLockOn);
}

BOOL kConvertScanCodeToASCIICode(BYTE bScanCode, BYTE* pbASCIICode, BOOL* pbFlags)
{
	BOOL bUseCombinedKey;

	// If the Pause key was previously received, ignore the remaining Pause scan code
	if (gs_stKeyboardManager.iSkipCountForPause > 0)
	{
		gs_stKeyboardManager.iSkipCountForPause--;
		return FALSE;
	}

	// handle Pause key
	if (bScanCode == 0xE1)
	{
		*pbASCIICode = KEY_PAUSE;
		*pbFlags = KEY_FLAGS_DOWN;
		gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
		return TRUE;
	}
	// just set flag when extended key code is received
	else if (bScanCode == 0xE0)
	{
		gs_stKeyboardManager.bExtendedCodeIn = TRUE;
		return FALSE;
	}

	bUseCombinedKey = kIsUseCombinedCode(bScanCode);

	if (bUseCombinedKey == TRUE)
	{
		*pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bCombinedCode;
	}
	else
	{
		*pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bNormalCode;
	}

	if (gs_stKeyboardManager.bExtendedCodeIn == TRUE)
	{
		*pbFlags = KEY_FLAGS_EXTENDEDKEY;
		gs_stKeyboardManager.bExtendedCodeIn = FALSE;
	}
	else
	{
		*pbFlags = 0;
	}

	if ((bScanCode & 0x80) == 0)
	{
		*pbFlags |= KEY_FLAGS_DOWN;
	}

	kUpdateCombinationKeyStatusAndLED(bScanCode);
	return TRUE;
}

BOOL kInitializeKeyboard(void)
{
	kInitializeQueue(&gs_stKeyQueue, gs_vstKeyQueueBuffer, KEY_MAXQUEUECOUNT,
			sizeof(KEYDATA));

	kInitializeSpinLock(&(gs_stKeyboardManager.stSpinLock));

	return kActivateKeyboard();
}

BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode)
{
	KEYDATA stData;
	BOOL bResult = FALSE;

	stData.bScanCode = bScanCode;

	if (kConvertScanCodeToASCIICode(bScanCode, &(stData.bASCIICode),
				&(stData.bFlags)) == TRUE)
	{
		kLockForSpinLock(&(gs_stKeyboardManager.stSpinLock));
		bResult = kPutQueue(&gs_stKeyQueue, &stData);
		kUnlockForSpinLock(&(gs_stKeyboardManager.stSpinLock));
	}
	return bResult;
}

BOOL kGetKeyFromKeyQueue(KEYDATA* pstData)
{
	BOOL bResult;

	if (kIsQueueEmpty(&gs_stKeyQueue) == TRUE)
		return FALSE;

	kLockForSpinLock(&(gs_stKeyboardManager.stSpinLock));
	bResult = kGetQueue(&gs_stKeyQueue, pstData);
	kUnlockForSpinLock(&(gs_stKeyboardManager.stSpinLock));
	return bResult;
}
