#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "PIT.h"
#include "RTC.h"
#include "Utility.h"

SHELLCOMMANDENTRY gs_vstCommandTable[] =
{
	{ "help", "Show Help", kHelp },
	{ "cls", "Clear Screen", kCls },
	{ "totalram", "Show Total RAM Size", kShowTotalRAMSize },
	{ "strtod", "String To Decimal/Hex Convert", kStringToDecimalHexTest },
	{ "shutdown", "Shutdown And Reboot OS", kShutdown },
	{ "settimer", "Set PIT Controller Counter0, ex) settimer 10(ms) 1(periodic)",
		kSetTimer },
	{ "wait", "Wait ms Using PIT, ex) wait 100(ms)", kWaitUsingPIT },
	{ "rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter },
	{ "cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed },
	{ "date", "Show Date And Time", kShowDateAndTime },
};

// main loop of shell
void kStartConsoleShell(void)
{
	char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
	int iCommandBufferIndex = 0;
	BYTE bKey;
	int iCursorX, iCursorY;

	kPrintf(CONSOLESHELL_PROMPTMESSAGE);

	while (1)
	{
		bKey = kGetCh();
		if (bKey == KEY_BACKSPACE)
		{
			if (iCommandBufferIndex > 0)
			{
				kGetCursor(&iCursorX, &iCursorY);
				kPrintStringXY(iCursorX - 1, iCursorY, " ");
				kSetCursor(iCursorX - 1, iCursorY);
				iCommandBufferIndex--;
			}
		}
		else if (bKey == KEY_ENTER)
		{
			kPrintf("\n");
			if (iCommandBufferIndex > 0)
			{
				vcCommandBuffer[iCommandBufferIndex] = '\0';
				kExecuteCommand(vcCommandBuffer);
			}
			kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
			kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
			iCommandBufferIndex = 0;
		}
		else if ((bKey == KEY_LSHIFT) || (bKey == KEY_RSHIFT) ||
				(bKey == KEY_CAPSLOCK) || (bKey == KEY_NUMLOCK) ||
				(bKey == KEY_SCROLLLOCK))
		{
			// Nothing
		}
		else
		{
			if (bKey == KEY_TAB)
				bKey = ' ';

			if (iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT)
			{
				vcCommandBuffer[iCommandBufferIndex++] = bKey;
				kPrintf("%c", bKey);
			}
		}
	}
}

void kExecuteCommand(const char* pcCommandBuffer)
{
	int i, iSpaceIndex;
	int iCommandBufferLength, iCommandLength;
	int iCount;

	iCommandBufferLength = kStrLen(pcCommandBuffer);
	for (iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++)
	{
		if (pcCommandBuffer[iSpaceIndex] == ' ')
			break;
	}

	iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);
	for (i = 0; i < iCount; i++)
	{
		iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);
		if ((iCommandLength == iSpaceIndex) &&
				(kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer,
						 iSpaceIndex) == 0))
		{
			// issue : void parameter
			gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1);
			break;
		}
	}
	if (i >= iCount)
	{
		kPrintf("'%s' is not found.\n", pcCommandBuffer);
	}
}

void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter)
{
	pstList->pcBuffer = pcParameter;
	pstList->iLength = kStrLen(pcParameter);
	pstList->iCurrentPosition = 0;
}

int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter)
{
	int i;
	int iLength;

	// return 0 when there is no parameter
	if (pstList->iLength <= pstList->iCurrentPosition)
		return 0;

	for (i = pstList->iCurrentPosition; i < pstList->iLength; i++)
	{
		if (pstList->pcBuffer[i] == ' ')
			break;
	}

	// Copy Parameter
	kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
	iLength = i - pstList->iCurrentPosition;
	pcParameter[iLength] = '\0';
	
	pstList->iCurrentPosition += iLength + 1;
	return iLength;
}

void kHelp(const char* pcCommandBuffer)
{
	int i;
	int iCount;
	int iCursorX, iCursorY;
	int iLength, iMaxCommandLength = 0;

	kPrintf("=================================================================\n");
	kPrintf("                        MINT64 Shell Help                        \n");
	kPrintf("=================================================================\n");

	iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

	for (i = 0; i < iCount; i++)
	{
		iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
		if (iLength > iMaxCommandLength)
		{
			iMaxCommandLength = iLength;
		}
	}

	for (i = 0; i < iCount; i++)
	{
		kPrintf("%s", gs_vstCommandTable[i].pcCommand);
		kGetCursor(&iCursorX, &iCursorY);
		kSetCursor(iMaxCommandLength, iCursorY);
		kPrintf("  - %s\n", gs_vstCommandTable[i].pcHelp);
	}
}

void kCls(const char* pcParameterBuffer)
{
	kClearScreen();
	kSetCursor(0, 1);
}

void kShowTotalRAMSize(const char* pcParameterBuffer)
{
	kPrintf("Total RAM Size = %d MB \n", kGetTotalRAMSize());
}

void kStringToDecimalHexTest(const char* pcParameterBuffer)
{
	char vcParameter[100];
	int iLength;
	PARAMETERLIST stList;
	int iCount = 0;
	long lValue;

	kInitializeParameter(&stList, pcParameterBuffer);

	while (1)
	{
		iLength = kGetNextParameter(&stList, vcParameter);
		if (iLength == 0)
			break;

		kPrintf("Param %d = '%s', Length = %d, ",
				iCount + 1, vcParameter, iLength);

		if (kMemCmp(vcParameter, "0x", 2) == 0)
		{
			lValue = kAToI(vcParameter + 2, 16);
			kPrintf("HEX Value = %q \n", lValue);
		}
		else
		{
			lValue = kAToI(vcParameter, 10);
			kPrintf("Decimal Value = %d\n", lValue);
		}

		iCount++;
	}
}

void kShutdown(const char* pcParameterBuffer)
{
	kPrintf("System Shutdown Start...\n");

	kPrintf("Press any key to Reboot PC...");
	kGetCh();
	kReboot();
}

void kSetTimer(const char* pcParameterBuffer)
{
	char vcParameter[100];
	PARAMETERLIST stList;
	long lValue;
	BOOL bPeriodic;

	kInitializeParameter(&stList, pcParameterBuffer);

	if (kGetNextParameter(&stList, vcParameter) == 0)
	{
		kPrintf("ex) settimer 10(ms) 1(periodic) \n");
		return;
	}
	lValue = kAToI(vcParameter, 10);

	if (kGetNextParameter(&stList, vcParameter) == 0)
	{
		kPrintf("ex) settimer 10(ms) 1(periodic) \n");
		return;
	}
	bPeriodic = kAToI(vcParameter, 10);

	kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
	kPrintf("Time = %d ms, Periodic = %d Change Complete \n", lValue, bPeriodic);
}

void kWaitUsingPIT(const char* pcParameterBuffer)
{
	char vcParameter[100];
	int iLength;
	PARAMETERLIST stList;
	long lMillisecond;
	int i;

	kInitializeParameter(&stList, pcParameterBuffer);
	if (kGetNextParameter(&stList, vcParameter) == 0)
	{
		kPrintf("ex) wait 100(ms) \n");
		return;
	}

	lMillisecond = kAToI(pcParameterBuffer, 10);
	kPrintf("%d ms Sleep Start...\n", lMillisecond);

	kDisableInterrupt();
	for (i = 0; i < lMillisecond / 30; i++)
		kWaitUsingDirectPIT(MSTOCOUNT(30));
	kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
	kEnableInterrupt();

	kPrintf("%d ms Sleep Complete\n", lMillisecond);

	kInitializePIT(MSTOCOUNT(1), TRUE);
}

void kReadTimeStampCounter(const char* pcParameterBuffer)
{
	kPrintf("Time Stamp Counter = %q \n", kReadTSC());
}

void kMeasureProcessorSpeed(const char* pcParameterBuffer)
{
	int i;
	QWORD qwLastTSC, qwTotalTSC = 0;

	kPrintf("Now Measuring.");

	kDisableInterrupt();
	for (i = 0; i < 200; i++)
	{
		qwLastTSC = kReadTSC();
		kWaitUsingDirectPIT(MSTOCOUNT(50));
		qwTotalTSC += kReadTSC() - qwLastTSC;

		kPrintf(".");
	}
	kInitializePIT(MSTOCOUNT(1), TRUE);
	kEnableInterrupt();

	kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}

void kShowDateAndTime(const char* pcParameterBuffer)
{
	BYTE bSecond, bMinute, bHour;
	BYTE bDayOfWeek, bDayOfMonth, bMonth;
	WORD wYear;

	kReadRTCTime(&bHour, &bMinute, &bSecond);
	kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

	kPrintf("Date: %d/%d/%d %s, ", wYear, bMonth, bDayOfMonth,
			kConvertDayOfWeekToString(bDayOfWeek));
	kPrintf("Time: %d:%d:%d \n", bHour, bMinute, bSecond);
}
