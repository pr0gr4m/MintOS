#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "PIT.h"
#include "RTC.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Task.h"
#include "Synchronization.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"

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
	{ "createtask", "Create Task, ex)createtask 1(type) 10(count)", kCreateTestTask },
	{ "changepriority", "Change Task Priority, ex)changepriority 1(ID) 2(Priority)",
		kChangeTaskPriority },
	{ "tasklist", "Show Task List", kShowTaskList },
	{ "killtask", "End Task, ex)killtask 1(ID) or 0xffffffff(All)", kKillTask },
	{ "cpuload", "Show Processor Load", kCPULoad },
	{ "testmutex", "Test Mutex Function", kTestMutex },
	{ "testthread", "Test Thread And Process Function", kTestThread },
	{ "showmatrix", "Show Matrix Screen", kShowMatrix },
	{ "testpie", "Test PIE Calculation", kTestPIE },
	{ "dynamicmeminfo", "Show Dynamic Memory Information", kShowDynamicMemoryInformation },
	{ "testseqalloc", "Test Sequential Allocation & Free", kTestSequentialAllocation },
	{ "testranalloc", "Test Random Allocation & Free", kTestRandomAllocation },
	{ "hddinfo", "Show HDD Information", kShowHDDInformation },
	{ "readsector", "Read HDD Sector, ex) readsector 0(LBA) 10(count)", kReadSector },
	{ "writesector", "Write HDD Sector, ex) writesector 0(LBA) 10(count)", kWriteSector },
	{ "mounthdd", "Mount HDD", kMountHDD },
	{ "formathdd", "Format HDD", kFormatHDD },
	{ "filesysteminfo", "Show File System Information", kShowFileSystemInformation },
	{ "createfile", "Create File, ex) createfile a.txt", kCreateFileInRootDirectory },
	{ "deletefile", "Delete File, ex) deletefile a.txt", kDeleteFileInRootDirectory },
	{ "dir", "Show Directory", kShowRootDirectory },
	{ "writefile", "Write Data To File, ex) writefile a.txt", kWriteDataToFile },
	{ "readfile", "Read Data From File, ex) readfile a.txt", kReadDataFromFile },
	{ "testfileio", "Test File I/O Function", kTestFileIO },
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

static void kHelp(const char* pcCommandBuffer)
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

		if ((i != 0) && ((i % 20) == 0))
		{
			kPrintf("Press any key to continue... ('q' is quit) : ");
			if (kGetCh() == 'q')
			{
				kPrintf("\n");
				break;
			}
			kPrintf("\n");
		}
	}
}

static void kCls(const char* pcParameterBuffer)
{
	kClearScreen();
	kSetCursor(0, 1);
}

static void kShowTotalRAMSize(const char* pcParameterBuffer)
{
	kPrintf("Total RAM Size = %d MB \n", kGetTotalRAMSize());
}

static void kStringToDecimalHexTest(const char* pcParameterBuffer)
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

static void kShutdown(const char* pcParameterBuffer)
{
	kPrintf("System Shutdown Start...\n");

	kPrintf("Press any key to Reboot PC...");
	kGetCh();
	kReboot();
}

static void kSetTimer(const char* pcParameterBuffer)
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

static void kWaitUsingPIT(const char* pcParameterBuffer)
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

static void kReadTimeStampCounter(const char* pcParameterBuffer)
{
	kPrintf("Time Stamp Counter = %q \n", kReadTSC());
}

static void kMeasureProcessorSpeed(const char* pcParameterBuffer)
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

static void kShowDateAndTime(const char* pcParameterBuffer)
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


// define TCB and stack
static TCB gs_vstTask[2] = { 0, };
static QWORD gs_vstStack[1024] = { 0, };

static void kTestTask(void)
{
	int i = 0;
	while (1)
	{
		kPrintf("[%d] This message is from kTestTask. Press any key to switch"
				" kConsoleSHell~!\n", i++);
		kGetCh();

		kSwitchContext(&(gs_vstTask[1].stContext), &(gs_vstTask[0].stContext));
	}
}

static void kTestTask1(void)
{
	BYTE bData;
	int i = 0, j = 0, iX = 0, iY = 0, iMargin;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;

	pstRunningTask = kGetRunningTask();
	iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

	for (j = 0; j < 20000; j++)
	{
		switch (i)
		{
			case 0:
				if (++iX >= (CONSOLE_WIDTH - iMargin))
					i = 1;
				break;

			case 1:
				if (++iY >= (CONSOLE_HEIGHT - iMargin))
					i = 2;
				break;

			case 2:
				if (--iX < iMargin)
					i = 3;
				break;

			case 3:
				if (--iY < iMargin)
					i = 0;
				break;
		}

		pstScreen[iY * CONSOLE_WIDTH + iX].bCharacter = bData;
		pstScreen[iY * CONSOLE_WIDTH + iX].bAttribute = bData & 0x0F;
		bData++;

		//kSchedule();
	}
	kExitTask();
}

static void kTestTask2(void)
{
	int i = 0, iOffset;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;
	char vcData[4] = { '-', '\\', '|', '/' };

	pstRunningTask = kGetRunningTask();
	iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
	iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT -
		(iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	while (1)
	{
		pstScreen[iOffset].bCharacter = vcData[i % 4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
		i++;

		//kSchedule();
	}
}

static void kCreateTestTask(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcType[30];
	char vcCount[30];
	int i;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcType);
	kGetNextParameter(&stList, vcCount);

	switch (kAToI(vcType, 10))
	{
		case 1:
			for (i = 0; i < kAToI(vcCount, 10); i++)
			{
				if (kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, 
							(QWORD)kTestTask1) == NULL)
					break;
			}
			
			kPrintf("Task1 %d Created\n", i);
			break;

		case 2:
		default:
			for (i = 0; i < kAToI(vcCount, 10); i++)
			{
				if (kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, 
							(QWORD)kTestTask2) == NULL)
					break;
			}

			kPrintf("Task2 %d Created\n", i);
			break;
	}
}

static void kChangeTaskPriority(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcID[30];
	char vcPriority[30];
	QWORD qwID;
	BYTE bPriority;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcID);
	kGetNextParameter(&stList, vcPriority);

	if (kMemCmp(vcID, "0x", 2) == 0)
	{
		qwID = kAToI(vcID + 2, 16);
	}
	else
	{
		qwID = kAToI(vcID, 10);
	}

	bPriority = kAToI(vcPriority, 10);

	kPrintf("Change Task Priority ID [0x%q] Priority[%d] ", qwID, bPriority);
	if (kChangePriority(qwID, bPriority) == TRUE)
	{
		kPrintf("Success\n");
	}
	else
	{
		kPrintf("Fail\n");
	}
}

static void kShowTaskList(const char* pcParameterBuffer)
{
	int i;
	TCB* pstTCB;
	int iCount = 0;

	kPrintf("========== Task Total Count [%d] ==========\n", kGetTaskCount());

	for (i = 0; i < TASK_MAXCOUNT; i++)
	{
		pstTCB = kGetTCBInTCBPool(i);
		if ((pstTCB->stLink.qwID >> 32) != 0)
		{
			if ((iCount != 0) && ((iCount % 10) == 0))
			{
				kPrintf("Press any key to continue...('q' is quit) : ");
				if (kGetCh() == 'q')
				{
					kPrintf("\n");
					break;
				}
				kPrintf("\n");
			}

			kPrintf("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n", 1 + iCount++,
					pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags),
					pstTCB->qwFlags, kGetListCount(&(pstTCB->stChildThreadList)));
			kPrintf("    Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n",
					pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress,
					pstTCB->qwMemorySize);
		}
	}
}

static void kKillTask(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcID[30];
	QWORD qwID;
	TCB* pstTCB;
	int i;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcID);

	if (kMemCmp(vcID, "0x", 2) == 0)
	{
		qwID = kAToI(vcID + 2, 16);
	}
	else
	{
		qwID = kAToI(vcID, 10);
	}

	if (qwID != 0xFFFFFFFF)
	{
		pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
		qwID = pstTCB->stLink.qwID;

		if (((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00))
		{
			kPrintf("Kill Task ID [0x%q] ", qwID);
			if (kEndTask(qwID) == TRUE)
			{
				kPrintf("Success\n");
			}
			else
			{
				kPrintf("Fail\n");
			}
		}
		else
		{
			kPrintf("Task does not exist or task is system task \n");
		}
	}
	else
	{
		for (i = 2; i < TASK_MAXCOUNT; i++)
		{
			pstTCB = kGetTCBInTCBPool(i);
			qwID = pstTCB->stLink.qwID;
			if (((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00))
			{
				kPrintf("Kill Task ID [0x%q] ", qwID);
				if (kEndTask(qwID) == TRUE)
				{
					kPrintf("Success\n");
				}
				else
				{
					kPrintf("Fail\n");
				}
			}
		}
	}
}

static void kCPULoad(const char* pcParameterBuffer)
{
	kPrintf("Processor Load : %d%%\n", kGetProcessorLoad());
}

static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

static void kPrintNumberTask(void)
{
	int i;
	int j;
	QWORD qwTickCount;

	qwTickCount = kGetTickCount();
	while ((kGetTickCount() - qwTickCount) < 50)
	{
		kSchedule();
	}

	for (i = 0; i < 5; i++)
	{
		kLock(&(gs_stMutex));
		kPrintf("Task ID [0x%Q] Value[%d]\n", kGetRunningTask()->stLink.qwID,
				gs_qwAdder);

		gs_qwAdder += 1;
		kUnlock(&(gs_stMutex));

		for (j = 0; j < 30000; j++);
	}

	qwTickCount = kGetTickCount();
	while ((kGetTickCount() - qwTickCount) < 1000)
	{
		kSchedule();
	}

	kExitTask();
}

static void kTestMutex(const char* pcParameterBuffer)
{
	int i;

	gs_qwAdder = 1;

	kInitializeMutex(&gs_stMutex);

	for (i = 0; i < 3; i++)
	{
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, 
				(QWORD)kPrintNumberTask);
	}
	kPrintf("Wait Until %d Task End... \n", i);
	kGetCh();
}

static void kCreateThreadTask(void)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2);
	}

	while (1)
	{
		kSleep(1);
	}
}

static void kTestThread(const char* pcParameterBuffer)
{
	TCB* pstProcess;

	pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xEEEEEEEE, 0x1000,
			(QWORD)kCreateThreadTask);
	if (pstProcess != NULL)
	{
		kPrintf("Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);
	}
	else
	{
		kPrintf("Process Create Fail\n");
	}
}

static volatile QWORD gs_qwRandomValue = 0;

QWORD kRandom(void)
{
	gs_qwRandomValue = (gs_qwRandomValue * 412153 + 5571031) >> 16;
	return gs_qwRandomValue;
}

static void kDropCharacterThread(void)
{
	int iX, iY;
	int i;
	char vcText[2] = { 0, };

	iX = kRandom() % CONSOLE_WIDTH;

	while (1)
	{
		kSleep(kRandom() % 20);

		if ((kRandom() % 20) < 15)
		{
			vcText[0] = ' ';
			for (i = 0; i < CONSOLE_HEIGHT - 1; i++)
			{
				kPrintStringXY(iX, i, vcText);
				kSleep(50);
			}
		}
		else
		{
			for (i = 0; i < CONSOLE_HEIGHT - 1; i++)
			{
				vcText[0] = i + kRandom();
				kPrintStringXY(iX, i, vcText);
				kSleep(50);
			}
		}
	}
}

static void kMatrixProcess(void)
{
	int i;

	for (i = 0; i < 300; i++)
	{
		if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0,
					(QWORD)kDropCharacterThread) == NULL)
			break;

		kSleep(kRandom() % 5 + 5);
	}

	kPrintf("%d Thread is created\n", i);
	kGetCh();
}

static void kShowMatrix(const char* pcParamterBuffer)
{
	TCB* pstProcess;
	pstProcess = kCreateTask(TASK_FLAGS_PROCESS | TASK_FLAGS_LOW,
			(void*)0xE00000, 0xE00000, (QWORD)kMatrixProcess);
	if (pstProcess != NULL)
	{
		kPrintf("Matrix Process [0x%Q] Create Success\n");

		while ((pstProcess->stLink.qwID >> 32) != 0)
			kSleep(100);
	}
	else
	{
		kPrintf("Matrix Process Create Fail\n");
	}
}

static void kFPUTestTask(void)
{
	double dValue1, dValue2;
	TCB* pstRunningTask;
	QWORD qwCount = 0;
	QWORD qwRandomValue;
	int i, iOffset;
	char vcData[4] = { '-', '\\', '|', '/' };
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;

	pstRunningTask = kGetRunningTask();

	iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
	iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT -
		(iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	while (1)
	{
		dValue1 = 1;
		dValue2 = 1;

		for (i = 0; i < 10; i++)
		{
			qwRandomValue = kRandom();
			dValue1 *= (double)qwRandomValue;
			dValue2 *= (double)qwRandomValue;

			kSleep(1);

			qwRandomValue = kRandom();
			dValue1 /= (double)qwRandomValue;
			dValue2 /= (double)qwRandomValue;
		}

		if (dValue1 != dValue2)
		{
			kPrintf("Value Is Not Same! [%f] != [%f]\n", dValue1, dValue2);
			break;
		}
		qwCount++;

		pstScreen[iOffset].bCharacter = vcData[qwCount % 4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
	}
}

static void kTestPIE(const char* pcParamterBuffer)
{
	double dResult;
	int i;

	kPrintf("PIE Calculation Test\n");
	kPrintf("Result: 355 / 133 = ");
	dResult = (double)355 / 113;
	kPrintf("%d.%d%d\n", (QWORD)dResult, ((QWORD)(dResult * 10) % 10),
			((QWORD)(dResult * 100) % 10));

	for (i = 0; i < 100; i++)
	{
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kFPUTestTask);
	}
}

static void kShowDynamicMemoryInformation(const char* pcParameterBuffer)
{
	QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;

	kGetDynamicMemoryInformation(&qwStartAddress, &qwTotalSize, &qwMetaSize,
			&qwUsedSize);

	kPrintf("========== Dynamic Memory Information ==========\n");
	kPrintf("Start Address: [0x%Q]\n", qwStartAddress);
	kPrintf("Total Size:    [0x%Q]byte, [%d]MB\n", qwTotalSize,
			qwTotalSize / 1024 / 1024);
	kPrintf("Meta Size:     [0x%Q]byte, [%d]KB\n", qwMetaSize,
			qwMetaSize / 1024);
	kPrintf("Used Size:     [0x%Q]byte, [%d]KB\n", qwUsedSize, qwUsedSize / 1024);
}

static void kTestSequentialAllocation(const char* pcParameterBuffer)
{
	DYNAMICMEMORY* pstMemory;
	long i, j, k;
	QWORD* pqwBuffer;

	kPrintf("========== Dynamic Memory Test ==========\n");
	pstMemory = kGetDynamicMemoryManager();

	for (i = 0; i < pstMemory->iMaxLevelCount; i++)
	{
		kPrintf("Block List [%d] Test Start\n", i);
		kPrintf("Allocation And Compare: ");

		for (j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++)
		{
			pqwBuffer = kAllocateMemory(DYNAMICMEMORY_MIN_SIZE << i);
			if (pqwBuffer == NULL)
			{
				kPrintf("\nAllocation Fail\n");
				return;
			}
			
			for (k = 0; k < (DYNAMICMEMORY_MIN_SIZE << i) / 8; k++)
				pqwBuffer[k] = k;

			for (k = 0; k < (DYNAMICMEMORY_MIN_SIZE << i) / 8; k++)
			{
				if (pqwBuffer[k] != k)
				{
					kPrintf("\nCompare Fail\n");
					return;
				}
			}
			kPrintf(".");
		}

		kPrintf("\nFree: ");
		for (j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++)
		{
			if (kFreeMemory((void*)(pstMemory->qwStartAddress +
							(DYNAMICMEMORY_MIN_SIZE << i) * j)) == FALSE)
			{
				kPrintf("\nFree Fail\n");
				return;
			}
			kPrintf(".");
		}
		kPrintf("\n");
	}
	kPrintf("Test Complete!!\n");
}

static void kRandomAllocationTask(void)
{
	TCB* pstTask;
	QWORD qwMemorySize;
	char vcBuffer[200];
	BYTE* pbAllocationBuffer;
	int i, j;
	int iY;

	pstTask = kGetRunningTask();
	iY = (pstTask->stLink.qwID) % 15 + 9;

	for (j = 0; j < 10; j++)
	{
		do
		{
			qwMemorySize = ((kRandom() % (32 * 1024)) + 1) * 1024;
			pbAllocationBuffer = kAllocateMemory(qwMemorySize);

			if (pbAllocationBuffer == 0)
				kSleep(1);
		} while (pbAllocationBuffer == 0);

		kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Allocation Success",
				pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);
		kSleep(200);

		kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Write...     ",
				pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);

		for (i = 0; i < qwMemorySize / 2; i++)
		{
			pbAllocationBuffer[i] = kRandom() & 0xFF;
			pbAllocationBuffer[i + (qwMemorySize / 2)] = pbAllocationBuffer[i];
		}
		kSleep(200);

		kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Verify...    ",
				pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);
		for (i = 0; i < qwMemorySize / 2; i++)
		{
			if (pbAllocationBuffer[i] != pbAllocationBuffer[i + (qwMemorySize / 2)])
			{
				kPrintf("Task ID [0x%Q] Verify Fail\n", pstTask->stLink.qwID);
				kExitTask();
			}
		}
		kFreeMemory(pbAllocationBuffer);
		kSleep(200);
	}
	kExitTask();
}

static void kTestRandomAllocation(const char* pcParameterBuffer)
{
	int i;

	for (i = 0; i < 1000; i++)
		kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, (QWORD)kRandomAllocationTask);
}

static void kShowHDDInformation(const char* pcParameterBuffer)
{
	HDDINFORMATION stHDD;
	char vcBuffer[100];

	if (kGetHDDInformation(&stHDD) == FALSE)
	{
		kPrintf("HDD Information Read Fail\n");
		return;
	}

	kPrintf("========== Primary Master HDD Information ==========\n");

	kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
	vcBuffer[sizeof(stHDD.vwModelNumber) - 1] = '\0';
	kPrintf("Model Number:\t %s\n", vcBuffer);

	kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
	vcBuffer[sizeof(stHDD.vwSerialNumber) - 1] = '\0';
	kPrintf("Serial Number:\t %s\n", vcBuffer);

	kPrintf("Head Count:\t %d\n", stHDD.wNumberOfHead);
	kPrintf("Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder);
	kPrintf("Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder);

	kPrintf("Total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors,
			stHDD.dwTotalSectors / 2 / 1024);
}

static void kReadSector(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcLBA[50], vcSectorCount[50];
	DWORD dwLBA;
	int iSectorCount;
	char* pcBuffer;
	int i, j;
	BYTE bData;
	BOOL bExit = FALSE;

	kInitializeParameter(&stList, pcParameterBuffer);
	if ((kGetNextParameter(&stList, vcLBA) == 0) ||
			(kGetNextParameter(&stList, vcSectorCount) == 0))
	{
		kPrintf("ex) readsector 0(LBA) 10(count)\n");
		return;
	}

	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);

	pcBuffer = kAllocateMemory(iSectorCount * 512);
	if (kReadHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) == iSectorCount)
	{
		kPrintf("LBA [%d], [%d] Sector Read Success!", dwLBA, iSectorCount);
		for (j = 0; j < iSectorCount; j++)
		{
			for (i = 0; i < 512; i++)
			{
				if (!((j == 0) && (i == 0)) && ((i % 256) == 0))
				{
					kPrintf("\nPress any key to continue... ('q' to quit) : ");
					if (kGetCh() == 'q')
					{
						bExit = TRUE;
						break;
					}
				}

				if ((i % 16) == 0)
				{
					kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i);
				}

				bData = pcBuffer[j * 512 + i] & 0xFF;
				if (bData < 16)
					kPrintf("0");
				kPrintf("%X ", bData);
			}

			if (bExit == TRUE)
				break;
		}
		kPrintf("\n");
	}
	else
		kPrintf("Read Fail\n");

	kFreeMemory(pcBuffer);
}

static void kWriteSector(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcLBA[50], vcSectorCount[50];
	DWORD dwLBA;
	int iSectorCount;
	char* pcBuffer;
	int i, j;
	BOOL bExit = FALSE;
	BYTE bData;
	static DWORD s_dwWriteCount = 0;

	kInitializeParameter(&stList, pcParameterBuffer);
	if ((kGetNextParameter(&stList, vcLBA) == 0) ||
			(kGetNextParameter(&stList, vcSectorCount) == 0))
	{
		kPrintf("ex) writesector 0(LBA) 10(count)\n");
		return;
	}

	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);

	s_dwWriteCount++;
	pcBuffer = kAllocateMemory(iSectorCount * 512);
	for (j = 0; j < iSectorCount; j++)
	{
		for (i = 0; i < 512; i+= 8)
		{
			*(DWORD*)&(pcBuffer[j * 512 + i]) = dwLBA + j;
			*(DWORD*)&(pcBuffer[j * 512 + i + 4]) = s_dwWriteCount;
		}
	}

	if (kWriteHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) != iSectorCount)
	{
		kPrintf("Write Fail\n");
		return;
	}

	kPrintf("LBA [%d], [%d] Sector Read Success!", dwLBA, iSectorCount);

	for (j = 0; j < iSectorCount; j++)
	{
		for (i = 0; i < 512; i++)
		{
			if (!((j == 0) && (i == 0)) && ((i % 256) == 0))
			{
				kPrintf("\nPress any key to continue... ('q' to quit) : ");
				if (kGetCh() == 'q')
				{
					bExit = TRUE;
					break;
				}
			}

			if ((i % 16) == 0)
			{
				kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i);
			}

			bData = pcBuffer[j * 512 + i] & 0xFF;
			if (bData < 16)
				kPrintf("0");
			kPrintf("%X ", bData);
		}

		if (bExit == TRUE)
			break;
	}
	kPrintf("\n");
	kFreeMemory(pcBuffer);
}

static void kMountHDD(const char* pcParameterBuffer)
{
	if (kMount() == FALSE)
	{
		kPrintf("HDD Mount Fail\n");
		return;
	}
	kPrintf("HDD Mount Success\n");
}

static void kFormatHDD(const char* pcParameterBuffer)
{
	if (kFormat() == FALSE)
	{
		kPrintf("HDD Format Fail\n");
		return;
	}
	kPrintf("HDD Format Success\n");
}

static void kShowFileSystemInformation(const char* pcParameterBuffer)
{
	FILESYSTEMMANAGER stManager;

	kGetFileSystemInformation(&stManager);

	kPrintf("========== File System Information ==========\n");
	kPrintf("Mounted:\t\t\t\t %d\n", stManager.bMounted);
	kPrintf("Reserved Sector Count:\t\t\t %d Sector\n", stManager.dwReservedSectorCount);
	kPrintf("Cluster Link Table Start Address:\t %d Sector\n",
			stManager.dwClusterLinkAreaStartAddress);
	kPrintf("Cluster Link Table Size:\t\t %d Sector\n", stManager.dwClusterLinkAreaSize);
	kPrintf("Data Area Start Address:\t\t %d Sector\n", stManager.dwDataAreaStartAddress);
	kPrintf("Total Cluster Count:\t\t\t %d Cluster\n", stManager.dwTotalClusterCount);
}

static void kCreateFileInRootDirectory(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	DWORD dwCluster;
	DIRECTORYENTRY stEntry;
	int i;

	kInitializeParameter(&stList, pcParameterBuffer);
	iLength = kGetNextParameter(&stList, vcFileName);
	vcFileName[iLength] = '\0';
	if ((iLength > (sizeof(stEntry.vcFileName) - 1)) || (iLength == 0))
	{
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}

	dwCluster = kFindFreeCluster();
	if ((dwCluster == FILESYSTEM_LASTCLUSTER) ||
			(kSetClusterLinkData(dwCluster, FILESYSTEM_LASTCLUSTER) == FALSE))
	{
		kPrintf("Cluster Allocation Fail\n");
		return;
	}

	i = kFindFreeDirectoryEntry();
	if (i == -1)
	{
		kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
		kPrintf("Directory Entry is Full\n");
		return;
	}

	kMemCpy(stEntry.vcFileName, vcFileName, iLength + 1);
	stEntry.dwStartClusterIndex = dwCluster;
	stEntry.dwFileSize = 0;

	if (kSetDirectoryEntryData(i, &stEntry) == FALSE)
	{
		kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
		kPrintf("Directory Entry Set Fail\n");
	}
	kPrintf("File Create Success \n");
}

static void kDeleteFileInRootDirectory(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	DIRECTORYENTRY stEntry;
	int iOffset;

	kInitializeParameter(&stList, pcParameterBuffer);
	iLength = kGetNextParameter(&stList, vcFileName);
	vcFileName[iLength] = '\0';
	if ((iLength > (sizeof(stEntry.vcFileName) - 1)) || (iLength == 0))
	{
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}

	iOffset = kFindDirectoryEntry(vcFileName, &stEntry);
	if (iOffset == -1)
	{
		kPrintf("File Not Found\n");
		return;
	}

	if (kSetClusterLinkData(stEntry.dwStartClusterIndex, FILESYSTEM_FREECLUSTER)
			== FALSE)
	{
		kPrintf("Cluster Free Fail\n");
		return;
	}

	kMemSet(&stEntry, 0, sizeof(stEntry));

	if (kSetDirectoryEntryData(iOffset, &stEntry) == FALSE)
	{
		kPrintf("Root Directory Update Fail\n");
		return;
	}

	kPrintf("File Delete Success \n");
}

static void kShowRootDirectory(const char* pcParameterBuffer)
{
	BYTE* pbClusterBuffer;
	int i, iCount, iTotalCount;
	DIRECTORYENTRY* pstEntry;
	char vcBuffer[400];
	char vcTempValue[50];
	DWORD dwTotalByte;

	pbClusterBuffer = kAllocateMemory(FILESYSTEM_SECTORSPERCLUSTER * 512);

	if (kReadCluster(0, pbClusterBuffer) == FALSE)
	{
		kPrintf("Root Directory Read Fail\n");
		return;
	}

	pstEntry = (DIRECTORYENTRY*)pbClusterBuffer;
	iTotalCount = 0;
	dwTotalByte = 0;
	for (i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++)
	{
		if (pstEntry[i].dwStartClusterIndex == 0)
			continue;
		iTotalCount++;
		dwTotalByte += pstEntry[i].dwFileSize;
	}

	pstEntry = (DIRECTORYENTRY*)pbClusterBuffer;
	iCount = 0;
	for (i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++)
	{
		if (pstEntry[i].dwStartClusterIndex == 0)
			continue;

		kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);
		vcBuffer[sizeof(vcBuffer) - 1] = '\0';

		kMemCpy(vcBuffer, pstEntry[i].vcFileName,
				kStrLen(pstEntry[i].vcFileName));

		kSPrintf(vcTempValue, "%d Byte", pstEntry[i].dwFileSize);
		kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));
		kSPrintf(vcTempValue, "0x%X Cluster", pstEntry[i].dwStartClusterIndex);
		kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue) + 1);
		kPrintf("\t%s\n", vcBuffer);

		if ((iCount != 0) && ((iCount % 20) == 0))
		{
			kPrintf("Press any key to continue... ('q' to quit) : ");
			if (kGetCh() == 'q')
			{
				kPrintf("\n");
				break;
			}
		}
		iCount++;
	}

	kPrintf("\t Total File Count: %d\t Total File Size: %d Byte\n",
			iTotalCount, dwTotalByte);
	kFreeMemory(pbClusterBuffer);
}
