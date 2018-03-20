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
#include "SerialPort.h"
#include "MPConfigurationTable.h"
#include "PIC.h"
#include "MultiProcessor.h"
#include "LocalAPIC.h"
#include "InterruptHandler.h"
#include "VBE.h"
#include "SystemCall.h"
#include "Loader.h"

SHELLCOMMANDENTRY gs_vstCommandTable[] =
{
	{ "help", "Show Help", kHelp },
	{ "cls", "Clear Screen", kCls },
	{ "totalram", "Show Total RAM Size", kShowTotalRAMSize },
	{ "shutdown", "Shutdown And Reboot OS", kShutdown },
	{ "cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed },
	{ "date", "Show Date And Time", kShowDateAndTime },
	{ "changepriority", "Change Task Priority, ex)changepriority 1(ID) 2(Priority)",
		kChangeTaskPriority },
	{ "tasklist", "Show Task List", kShowTaskList },
	{ "killtask", "End Task, ex)killtask 1(ID) or 0xffffffff(All)", kKillTask },
	{ "cpuload", "Show Processor Load", kCPULoad },
	{ "showmatrix", "Show Matrix Screen", kShowMatrix },
	{ "dynamicmeminfo", "Show Dynamic Memory Information", kShowDynamicMemoryInformation },
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
	{ "flush", "Flush File System Cache", kFlushCache },
	{ "download", "Download Data From Serial, ex) download a.txt", kDownloadFile },
	{ "showmpinfo", "Show MP Configuration Table Information", kShowMPConfigurationTable },
	{ "showirqintinmap", "Show IRQ->INTIN Mapping Table", kShowIRQINTINMappingTable },
	{ "showintproccount", "Show Interrupt Processing Count", kShowInterruptProcessingCount },
	{ "changeaffinity", "Change Task Affinity, ex) changeaffinity 1(ID) 0xFF(Affinity)",
		kChangeTaskAffinity },
	{ "vbemodeinfo", "Show VBE Mode Information", kShowVBEModeInfo },
	{ "testsystemcall", "Test System Call Operation", kTestSystemCall },
	{ "exec", "Execute Application Program, ex) exec a.elf argument",
		kExecuteApplicationProgram },
};

// main loop of shell
void kStartConsoleShell(void)
{
	char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
	int iCommandBufferIndex = 0;
	BYTE bKey;
	int iCursorX, iCursorY;
	CONSOLEMANAGER* pstConsoleManager;

	pstConsoleManager = kGetConsoleManager();

	kPrintf("\n");
	kPrintf(CONSOLESHELL_PROMPTMESSAGE);

	while (pstConsoleManager->bExit == FALSE)
	{
		bKey = kGetCh();

		if (pstConsoleManager->bExit == TRUE)
			break;

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
		else if ((bKey >= KEY_CTRL && bKey <= KEY_F12) || (bKey == KEY_ESC))
		{
			// Nothing
		}
		else if (bKey < 128)
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

static void kShutdown(const char* pcParameterBuffer)
{
	kPrintf("System Shutdown Start...\n");

	kPrintf("Cache Flush... ");
	if (kFlushFileSystemCache() == TRUE)
		kPrintf("[PASS]\n");
	else
		kPrintf("[FAIL]\n");
	kPrintf("Press any key to Reboot PC...");
	kGetCh();
	kReboot();
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
	int iTotalTaskCount = 0;
	char vcBuffer[20];
	int iRemainLength;
	int iProcessorCount;

	iProcessorCount = kGetProcessorCount();

	for (i = 0; i < iProcessorCount; i++)
		iTotalTaskCount += kGetTaskCount(i);

	kPrintf("========== Task Total Count [%d] ==========\n", iTotalTaskCount);

	if (iProcessorCount > 1)
	{
		for (i = 0; i < iProcessorCount; i++)
		{
			if ((i != 0) && ((i % 4) == 0))
				kPrintf("\n");

			kSPrintf(vcBuffer, "Core %d : %d", i, kGetTaskCount(i));
			kPrintf(vcBuffer);

			iRemainLength = 19 - kStrLen(vcBuffer);
			kMemSet(vcBuffer, ' ', iRemainLength);
			vcBuffer[iRemainLength] = '\0';
			kPrintf(vcBuffer);
		}

		kPrintf("\nPress any key to continue... ('q' to quit) : ");
		if (kGetCh() == 'q')
		{
			kPrintf("\n");
			return;
		}
		kPrintf("\n");
	}

	for (i = 0; i < TASK_MAXCOUNT; i++)
	{
		pstTCB = kGetTCBInTCBPool(i);
		if ((pstTCB->stLink.qwID >> 32) != 0)
		{
			if ((iCount != 0) && ((iCount % 6) == 0))
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
			kPrintf("    Core ID[0x%X] CPU Affinity[0x%X]\n", pstTCB->bAPICID, pstTCB->bAffinity);
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
	int i;
	char vcBuffer[50];
	int iRemainLength;

	kPrintf("========== Processor Load ==========\n");

	for (i = 0; i < kGetProcessorCount(); i++)
	{
		if ((i != 0) && ((i % 4) == 0))
			kPrintf("\n");

		kSPrintf(vcBuffer, "Core %d : %d%%", i, kGetProcessorLoad(i));
		kPrintf("%s", vcBuffer);

		iRemainLength = 19 - kStrLen(vcBuffer);
		kMemSet(vcBuffer, ' ', iRemainLength);
		vcBuffer[iRemainLength] = '\0';
		kPrintf(vcBuffer);
	}
	kPrintf("\n");
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
				vcText[0] = i + kRandom() % 128;
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
					(QWORD)kDropCharacterThread, TASK_LOADBALANCINGID) == NULL)
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
			(void*)0xE00000, 0xE00000, (QWORD)kMatrixProcess, TASK_LOADBALANCINGID);
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

/**
 *  루트 디렉터리에 빈 파일을 생성
 */
static void kCreateFileInRootDirectory( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    DWORD dwCluster;
    int i;
    FILE* pstFile;
    
    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    pstFile = fopen( vcFileName, "w" );
    if( pstFile == NULL )
    {
        kPrintf( "File Create Fail\n" );
        return;
    }
    fclose( pstFile );
    kPrintf( "File Create Success\n" );
}

/**
 *  루트 디렉터리에서 파일을 삭제
 */
static void kDeleteFileInRootDirectory( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    
    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }

    if( remove( vcFileName ) != 0 )
    {
        kPrintf( "File Not Found or File Opened\n" );
        return ;
    }
    
    kPrintf( "File Delete Success\n" );
}

/**
 *  루트 디렉터리의 파일 목록을 표시
 */
static void kShowRootDirectory( const char* pcParameterBuffer )
{
    DIR* pstDirectory;
    int i, iCount, iTotalCount;
    struct dirent* pstEntry;
    char vcBuffer[ 400 ];
    char vcTempValue[ 50 ];
    DWORD dwTotalByte;
    DWORD dwUsedClusterCount;
    FILESYSTEMMANAGER stManager;
    
    // 파일 시스템 정보를 얻음
    kGetFileSystemInformation( &stManager );
     
    // 루트 디렉터리를 엶
    pstDirectory = opendir( "/" );
    if( pstDirectory == NULL )
    {
        kPrintf( "Root Directory Open Fail\n" );
        return ;
    }
    
    // 먼저 루프를 돌면서 디렉터리에 있는 파일의 개수와 전체 파일이 사용한 크기를 계산
    iTotalCount = 0;
    dwTotalByte = 0;
    dwUsedClusterCount = 0;
    while( 1 )
    {
        // 디렉터리에서 엔트리 하나를 읽음
        pstEntry = readdir( pstDirectory );
        // 더이상 파일이 없으면 나감
        if( pstEntry == NULL )
        {
            break;
        }
        iTotalCount++;
        dwTotalByte += pstEntry->dwFileSize;

        // 실제로 사용된 클러스터의 개수를 계산
        if( pstEntry->dwFileSize == 0 )
        {
            // 크기가 0이라도 클러스터 1개는 할당되어 있음
            dwUsedClusterCount++;
        }
        else
        {
            // 클러스터 개수를 올림하여 더함
            dwUsedClusterCount += ( pstEntry->dwFileSize + 
                ( FILESYSTEM_CLUSTERSIZE - 1 ) ) / FILESYSTEM_CLUSTERSIZE;
        }
    }
    
    // 실제 파일의 내용을 표시하는 루프
    rewinddir( pstDirectory );
    iCount = 0;
    while( 1 )
    {
        // 디렉터리에서 엔트리 하나를 읽음
        pstEntry = readdir( pstDirectory );
        // 더이상 파일이 없으면 나감
        if( pstEntry == NULL )
        {
            break;
        }
        
        // 전부 공백으로 초기화 한 후 각 위치에 값을 대입
        kMemSet( vcBuffer, ' ', sizeof( vcBuffer ) - 1 );
        vcBuffer[ sizeof( vcBuffer ) - 1 ] = '\0';
        
        // 파일 이름 삽입
        kMemCpy( vcBuffer, pstEntry->d_name, 
                 kStrLen( pstEntry->d_name ) );

        // 파일 길이 삽입
        kSPrintf( vcTempValue, "%d Byte", pstEntry->dwFileSize );
        kMemCpy( vcBuffer + 30, vcTempValue, kStrLen( vcTempValue ) );

        // 파일의 시작 클러스터 삽입
        kSPrintf( vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex );
        kMemCpy( vcBuffer + 55, vcTempValue, kStrLen( vcTempValue ) + 1 );
        kPrintf( "    %s\n", vcBuffer );

        if( ( iCount != 0 ) && ( ( iCount % 20 ) == 0 ) )
        {
            kPrintf( "Press any key to continue... ('q' is exit) : " );
            if( kGetCh() == 'q' )
            {
                kPrintf( "\n" );
                break;
            }        
        }
        iCount++;
    }
    
    // 총 파일의 개수와 파일의 총 크기를 출력
    kPrintf( "\t\tTotal File Count: %d\n", iTotalCount );
    kPrintf( "\t\tTotal File Size: %d KByte (%d Cluster)\n", dwTotalByte / 1024, 
             dwUsedClusterCount );
    
    // 남은 클러스터 수를 이용해서 여유 공간을 출력
    kPrintf( "\t\tFree Space: %d KByte (%d Cluster)\n", 
             ( stManager.dwTotalClusterCount - dwUsedClusterCount ) * 
             FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount - 
             dwUsedClusterCount );
    
    // 디렉터리를 닫음
    closedir( pstDirectory );
}

/**
 *  파일을 생성하여 키보드로 입력된 데이터를 씀
 */
static void kWriteDataToFile( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    FILE* fp;
    int iEnterCount;
    BYTE bKey;
    
    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }
    
    // 파일 생성
    fp = fopen( vcFileName, "w" );
    if( fp == NULL )
    {
        kPrintf( "%s File Open Fail\n", vcFileName );
        return ;
    }
    
    // 엔터 키가 연속으로 3번 눌러질 때까지 내용을 파일에 씀
    iEnterCount = 0;
    while( 1 )
    {
        bKey = kGetCh();
        // 엔터 키이면 연속 3번 눌러졌는가 확인하여 루프를 빠져 나감
        if( bKey == KEY_ENTER )
        {
            iEnterCount++;
            if( iEnterCount >= 3 )
            {
                break;
            }
        }
        // 엔터 키가 아니라면 엔터 키 입력 횟수를 초기화
        else
        {
            iEnterCount = 0;
        }
        
        kPrintf( "%c", bKey );
        if( fwrite( &bKey, 1, 1, fp ) != 1 )
        {
            kPrintf( "File Wirte Fail\n" );
            break;
        }
    }
    
    kPrintf( "File Create Success\n" );
    fclose( fp );
}

/**
 *  파일을 열어서 데이터를 읽음
 */
static void kReadDataFromFile( const char* pcParameterBuffer )
{
    PARAMETERLIST stList;
    char vcFileName[ 50 ];
    int iLength;
    FILE* fp;
    int iEnterCount;
    BYTE bKey;
    
    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    kInitializeParameter( &stList, pcParameterBuffer );
    iLength = kGetNextParameter( &stList, vcFileName );
    vcFileName[ iLength ] = '\0';
    if( ( iLength > ( FILESYSTEM_MAXFILENAMELENGTH - 1 ) ) || ( iLength == 0 ) )
    {
        kPrintf( "Too Long or Too Short File Name\n" );
        return ;
    }
    
    // 파일 생성
    fp = fopen( vcFileName, "r" );
    if( fp == NULL )
    {
        kPrintf( "%s File Open Fail\n", vcFileName );
        return ;
    }
    
    // 파일의 끝까지 출력하는 것을 반복
    iEnterCount = 0;
    while( 1 )
    {
        if( fread( &bKey, 1, 1, fp ) != 1 )
        {
            break;
        }
        kPrintf( "%c", bKey );
        
        // 만약 엔터 키이면 엔터 키 횟수를 증가시키고 20라인까지 출력했다면 
        // 더 출력할지 여부를 물어봄
        if( bKey == KEY_ENTER )
        {
            iEnterCount++;
            
            if( ( iEnterCount != 0 ) && ( ( iEnterCount % 20 ) == 0 ) )
            {
                kPrintf( "Press any key to continue... ('q' is exit) : " );
                if( kGetCh() == 'q' )
                {
                    kPrintf( "\n" );
                    break;
                }
                kPrintf( "\n" );
                iEnterCount = 0;
            }
        }
    }
    fclose( fp );
}

static void kFlushCache(const char* pcParameterBuffer)
{
	QWORD qwTickCount;

	qwTickCount = kGetTickCount();
	kPrintf("Cache Flush... ");
	if (kFlushFileSystemCache() == TRUE)
		kPrintf("[PASS]\n");
	else
		kPrintf("[FAIL]\n");
	kPrintf("Total Time = %d ms\n", kGetTickCount() - qwTickCount);
}

static void kDownloadFile(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcFileName[50];
	int iFileNameLength;
	DWORD dwDataLength;
	FILE* fp;
	DWORD dwReceivedSize;
	DWORD dwTempSize;
	BYTE vbDataBuffer[SERIAL_FIFOMAXSIZE];
	QWORD qwLastReceivedTickCount;

	kInitializeParameter(&stList, pcParameterBuffer);
	iFileNameLength = kGetNextParameter(&stList, vcFileName);
	vcFileName[iFileNameLength] = '\0';
	if ((iFileNameLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) ||
			(iFileNameLength == 0))
	{
		kPrintf("Too Long or Too Short File Name\n");
		kPrintf("ex) download a.txt");
		return;
	}

	kClearSerialFIFO();

	kPrintf("Waiting For Data Length......");
	dwReceivedSize = 0;
	qwLastReceivedTickCount = kGetTickCount();
	while (dwReceivedSize < 4)
	{
		dwTempSize = kReceiveSerialData(((BYTE*)&dwDataLength) +
				dwReceivedSize, 4 - dwReceivedSize);
		dwReceivedSize += dwTempSize;

		if (dwTempSize == 0)
		{
			kSleep(1);

			if ((kGetTickCount() - qwLastReceivedTickCount) > 30000)
			{
				kPrintf("Time Out Occur!\n");
				return;
			}
		}
		else
		{
			qwLastReceivedTickCount = kGetTickCount();
		}
	}
	kPrintf("[%d] Byte\n", dwDataLength);

	kSendSerialData("A", 1);

	fp = fopen(vcFileName, "w");
	if (fp == NULL)
	{
		kPrintf("%s File Open Fail\n", vcFileName);
		return;
	}

	kPrintf("Data Receive Start: ");
	dwReceivedSize = 0;
	qwLastReceivedTickCount = kGetTickCount();
	while (dwReceivedSize < dwDataLength)
	{
		dwTempSize = kReceiveSerialData(vbDataBuffer, SERIAL_FIFOMAXSIZE);
		dwReceivedSize += dwTempSize;

		if (dwTempSize != 0)
		{
			if (((dwReceivedSize % SERIAL_FIFOMAXSIZE) == 0) ||
					((dwReceivedSize == dwDataLength)))
			{
				kSendSerialData("A", 1);
				kPrintf("#");
			}

			if (fwrite(vbDataBuffer, 1, dwTempSize, fp) != dwTempSize)
			{
				kPrintf("File Wrtie Error Occur\n");
				break;
			}

			qwLastReceivedTickCount = kGetTickCount();
		}
		else
		{
			kSleep(1);

			if ((kGetTickCount() - qwLastReceivedTickCount) > 10000)
			{
				kPrintf("Time Out Occur!\n");
				break;
			}
		}
	}

	if (dwReceivedSize != dwDataLength)
	{
		kPrintf("\nError Occur. Total Size [%d] Received Size [%d]\n",
				dwReceivedSize, dwDataLength);
	}
	else
	{
		kPrintf("\nReceive Complete. Total Size [%d] Byte\n", dwReceivedSize);
	}

	fclose(fp);
	kFlushFileSystemCache();
}

static void kShowMPConfigurationTable(const char* pcParameterBuffer)
{
	kPrintMPConfigurationTable();
}

static void kShowIRQINTINMappingTable(const char* pcParameterBuffer)
{
	kPrintIRQToINTINMap();
}

static void kShowInterruptProcessingCount(const char* pcParameterBuffer)
{
	INTERRUPTMANAGER* pstInterruptManager;
	int i, j, iProcessCount;
	char vcBuffer[20];
	int iRemainLength, iLineCount;

	kPrintf("====================  Interrupt Count ==================== \n");

	iProcessCount = kGetProcessorCount();

	// 4 core for 1 line, 15 row for 1 column
	for (i = 0; i < iProcessCount; i++)
	{
		if (i == 0)
			kPrintf("IRQ Num\t\t");
		else if ((i % 4) == 0)
			kPrintf("\n        \t\t");
		kSPrintf(vcBuffer, "Core %d", i);
		kPrintf(vcBuffer);

		iRemainLength = 15 - kStrLen(vcBuffer);
		kMemSet(vcBuffer, ' ', iRemainLength);
		vcBuffer[iRemainLength] = '\0';
		kPrintf(vcBuffer);
	}
	kPrintf("\n");

	iLineCount = 0;
	pstInterruptManager = kGetInterruptManager();
	for (i = 0; i < INTERRUPT_MAXVECTORCOUNT; i++)
	{
		for (j = 0; j < iProcessCount; j++)
		{
			if (j == 0)
			{
				if ((iLineCount != 0) && (iLineCount > 10))
				{
					kPrintf("\nPress any key to continue... ('q' to quit) : ");
					if (kGetCh() == 'q')
					{
						kPrintf("\n");
						return;
					}
					iLineCount = 0;
					kPrintf("\n");
				}
				kPrintf("----------------------------------------------------------\n");
				kPrintf("IRQ %d\t\t", i);
				iLineCount += 2;
			}
			else if ((j % 4) == 0)
			{
				kPrintf("\n        \t\t");
				iLineCount++;
			}

			kSPrintf(vcBuffer, "0x%Q", pstInterruptManager->vvqwCoreInterruptCount[j][i]);
			kPrintf(vcBuffer);
			iRemainLength = 15 - kStrLen(vcBuffer);
			kMemSet(vcBuffer, ' ', iRemainLength);
			vcBuffer[iRemainLength] = '\0';
			kPrintf(vcBuffer);
		}
		kPrintf("\n");
	}
}

static void kChangeTaskAffinity(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcID[30];
	char vcAffinity[30];
	QWORD qwID;
	BYTE bAffinity;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcID);
	kGetNextParameter(&stList, vcAffinity);

	if (kMemCmp(vcID, "0x", 2) == 0)
		qwID = kAToI(vcID + 2, 16);
	else
		qwID = kAToI(vcID, 10);

	if (kMemCmp(vcAffinity, "0x", 2) == 0)
		bAffinity = kAToI(vcAffinity + 2, 16);
	else
		bAffinity = kAToI(vcAffinity, 10);

	kPrintf("Change Task Affinity ID [0x%q] Affinity [0x%x] ", qwID, bAffinity);
	if (kChangeProcessorAffinity(qwID, bAffinity) == TRUE)
		kPrintf("Success\n");
	else
		kPrintf("Fail\n");
}

static void kShowVBEModeInfo(const char* pcParameterBuffer)
{
	VBEMODEINFOBLOCK* pstModeInfo;

	pstModeInfo = kGetVBEModeInfoBlock();
	kPrintf("VESA %x\n", pstModeInfo->wWinGranulity);
	kPrintf("X Resolution: %d\n", pstModeInfo->wXResolution);
	kPrintf("Y Resolution: %d\n", pstModeInfo->wYResolution);
	kPrintf("Bits Per Pixel: %d\n", pstModeInfo->bBitsPerPixel);

	kPrintf("Red Mask Size: %d, Field Position: %d\n", pstModeInfo->bRedMaskSize,
			pstModeInfo->bRedFieldPosition);
	kPrintf("Green Mask Size: %d, Field Position: %d\n", pstModeInfo->bGreenMaskSize,
			pstModeInfo->bGreenFieldPosition);
	kPrintf("Blue Mask Size: %d, Field Position: %d\n", pstModeInfo->bBlueMaskSize,
			pstModeInfo->bBlueFieldPosition);
	kPrintf("Physical Base Pointer: 0x%X\n", pstModeInfo->dwPhysicalBasePointer);

	kPrintf("Linear Red Mask Size: %d, Field Position: %d\n",
			pstModeInfo->bLinearRedMaskSize, pstModeInfo->bLinearRedFieldPosition);
	kPrintf("Linear Green Mask Size: %d, Field Position: %d\n",
			pstModeInfo->bLinearGreenMaskSize, pstModeInfo->bLinearGreenFieldPosition);
	kPrintf("Linear Blue Mask Size: %d, Field Position: %d\n",
			pstModeInfo->bLinearBlueMaskSize, pstModeInfo->bLinearBlueFieldPosition);
}

static void kTestSystemCall(const char* pcParameterBuffer)
{
	BYTE* pbUserMemory;

	if ((pbUserMemory = kAllocateMemory(0x1000)) == NULL)
		return;

	kMemCpy(pbUserMemory, kSystemCallTestTask, 0x1000);
	kCreateTask(TASK_FLAGS_USERLEVEL | TASK_FLAGS_PROCESS,
			pbUserMemory, 0x1000, (QWORD)pbUserMemory, TASK_LOADBALANCINGID);
}


static void kExecuteApplicationProgram(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcFileName[512];
	char vcArgumentString[1024];
	QWORD qwID;

	kInitializeParameter(&stList, pcParameterBuffer);
	if (kGetNextParameter(&stList, vcFileName) == 0)
	{
		kPrintf("ex) exec a.elf argument\n");
		return;
	}

	if (kGetNextParameter(&stList, vcArgumentString) == 0)
		vcArgumentString[0] = '\0';

	kPrintf("Execute Program... File [%s], Argument [%s]\n", vcFileName,
			vcArgumentString);

	qwID = kExecuteProgram(vcFileName, vcArgumentString, TASK_LOADBALANCINGID);
	kPrintf("Task ID = 0x%Q\n", qwID);
}
