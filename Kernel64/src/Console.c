#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"

CONSOLEMANAGER gs_stConsoleManager = { 0, };

void kInitializeConsole(int iX, int iY)
{
	kMemSet(&gs_stConsoleManager, 0, sizeof(gs_stConsoleManager));
	kSetCursor(iX, iY);
}

void kSetCursor(int iX, int iY)
{
	int iLinearValue;

	iLinearValue = iY * CONSOLE_WIDTH + iX;

	// send 0x0E to CRTC control address register
	// to select upper cursor register
	kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
	// send upper byte to CRTC control data register
	kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);

	// send 0x0F to CRTC control address register
	// to select lower cursor register
	kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
	// send lower byte to CRTC control data register
	kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xFF);

	gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;
}

void kGetCursor(int* piX, int* piY)
{
	*piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
	*piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}

void kPrintf(const char* pcFormatString, ...)
{
	va_list ap;
	char vcBuffer[1024];
	int iNextPrintOffset;

	va_start(ap, pcFormatString);
	kVSPrintf(vcBuffer, pcFormatString, ap);
	va_end(ap);

	iNextPrintOffset = kConsolePrintString(vcBuffer);
	kSetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}

int kConsolePrintString(const char* pcBuffer)
{
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i, j;
	int iLength;
	int iPrintOffset;

	iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;

	iLength = kStrLen(pcBuffer);
	for (i = 0; i < iLength; i++)
	{
		if (pcBuffer[i] == '\n')
		{
			iPrintOffset += (CONSOLE_WIDTH - (iPrintOffset % CONSOLE_WIDTH));
		}
		else if (pcBuffer[i] == '\t')
		{
			iPrintOffset += (8 - (iPrintOffset % 8));
		}
		else
		{
			pstScreen[iPrintOffset].bCharacter = pcBuffer[i];
			pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
			iPrintOffset++;
		}

		// scroll
		if (iPrintOffset >= (CONSOLE_HEIGHT * CONSOLE_WIDTH))
		{
			kMemCpy(CONSOLE_VIDEOMEMORYADDRESS,
					CONSOLE_VIDEOMEMORYADDRESS + CONSOLE_WIDTH * sizeof(CHARACTER),
					(CONSOLE_HEIGHT - 1) * CONSLE_WIDTH * sizeof(CHARACTER));
			for (j = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
					j < (CONSOLE_HEIGHT * CONSOLE_WIDTH); j++)
			{
				pstScreen[j].bCharacter = ' ';
				pstScreen[j].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
			}
			iPrintOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
		}
	}
	return iPrintOffset;
}

void kClearScreen(void)
{
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;

	for (i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++)
	{
		pstScreen[i].bCharacter = ' ';
		pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}

	kSetCursor(0, 0);
}

BYTE kGetCh(void)
{
	KEYDATA stData;

	while (1)
	{
		while (kGetKeyFromKeyQueue(&stData) == FALSE);

		if (stData.bFlags & KEY_FLAGS_DOWN)
			return stData.bASCIICode;
	}
}

void kPrintStringXY(int iX, int iY, const char* pcString)
{
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;

	pstScreen += (iY * 80) + iX;
	for (i = 0; pcString[i] != 0; i++)
	{
		pstScreen[i].bCharacter = pcString[i];
		pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}
}
