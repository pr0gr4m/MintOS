#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"
#include "AssemblyUtility.h"
#include "Utility.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"
#include "MultiProcessor.h"
#include "LocalAPIC.h"
#include "VBE.h"
#include "ConsoleShell.h"
#include "2DGraphics.h"
#include "Mouse.h"
#include "MPConfigurationTable.h"
#include "WindowManagerTask.h"
#include "SystemCall.h"

void MainForApplicationProcessor(void);

void Main(void)
{
	int iCursorX, iCursorY;
	BYTE bButton;
	int iX, iY;

	if (*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0)
		MainForApplicationProcessor();

	*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) = 0;

	kInitializeConsole(0, 10);
	kPrintf("Switch To IA-32e Mode Success!\n");
	kPrintf("IA-32e C Language Kernel Start.............................[PASS]\n");
	kPrintf("Initialize Console.........................................[PASS]\n");

	kGetCursor(&iCursorX, &iCursorY);
	kPrintf("GDT Initialize And Switch For IA-32e Mode..................[    ]");
	kInitializeGDTTableAndTSS();
	kLoadGDTR(GDTR_STARTADDRESS);
	kSetCursor(60, iCursorY++);
	kPrintf("PASS\n");

	kPrintf("TSS Segment Load...........................................[    ]");
	kLoadTR(GDT_TSSSEGMENT);
	kSetCursor(60, iCursorY++);
	kPrintf("PASS\n");

	kPrintf("IDT Initialize.............................................[    ]");
	kInitializeIDTTables();
	kLoadIDTR(IDTR_STARTADDRESS);
	kSetCursor(60, iCursorY++);
	kPrintf("PASS\n");

	kPrintf("Total RAM Size Check.......................................[    ]");
	kCheckTotalRAMSize();
	kSetCursor(60, iCursorY++);
	kPrintf("PASS], %d MB\n", kGetTotalRAMSize());

	kPrintf("TCB Pool And Scheduler Initialize..........................[PASS]\n");
	iCursorY++;
	kInitializeScheduler();

	kPrintf("Dynamic Memory Initialize..................................[PASS]\n");
	iCursorY++;
	kInitializeDynamicMemory();

	kInitializePIT(MSTOCOUNT(1), 1);

	kPrintf("Keyboard Activate And Queue Initialize.....................[    ]");

	if (kInitializeKeyboard() == TRUE)
	{
		kSetCursor(60, iCursorY++);
		kPrintf("PASS\n");
		kChangeKeyboardLED(FALSE, FALSE, FALSE);
	}
	else
	{
		kSetCursor(60, iCursorY++);
		kPrintf("FAIL\n");
		while (1);
	}

	
	kPrintf("Mouse Activate And Queue Initialize........................[    ]");

	if (kInitializeMouse() == TRUE)
	{
		kEnableMouseInterrupt();
		kSetCursor(60, iCursorY++);
		kPrintf("PASS\n");
	}
	else
	{
		kSetCursor(60, iCursorY++);
		kPrintf("FAIL\n");
		while (1);
	}

	kPrintf("PIC Controller And Interrupt Initialize....................[    ]");
	kInitializePIC();
	kMaskPICInterrupt(0);
	kEnableInterrupt();
	kSetCursor(60, iCursorY++);
	kPrintf("PASS\n");

	/*
	kPrintf("HDD Initialize.............................................[    ]");
	kSetCursor(60, iCursorY++);
	if (kInitializeHDD() == TRUE)
	{
		kPrintf("PASS\n");
	}
	else
	{
		kPrintf("FAIL\n");
	}
	*/

	kPrintf("File System Initialize.....................................[    ]");
	if (kInitializeFileSystem() == TRUE)
	{
		kSetCursor(60, iCursorY++);
		kPrintf("PASS\n");
	}
	else
	{
		kSetCursor(60, iCursorY++);
		kPrintf("FAIL\n");
	}

	kPrintf("Serial Port Initialize.....................................[    ]");
	kInitializeSerialPort();
	kSetCursor(60, iCursorY++);
	kPrintf("PASS\n");

	kPrintf("Change To Multicore Processor Mode.........................[    ]");
	if (kChangeToMultiCoreMode() == TRUE)
	{
		kSetCursor(60, iCursorY++);
		kPrintf("PASS\n");
	}
	else
	{
		kSetCursor(60, iCursorY++);
		kPrintf("FAIL\n");
	}

	kPrintf("System Call MSR Initialize.................................[PASS]");
	iCursorY++;
	kInitializeSystemCall();

	kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0,
			(QWORD)kIdleTask, kGetAPICID());

	if (*(BYTE*)VBE_STARTGRAPHICMODEFLAGADDRESS == 0)	
		kStartConsoleShell();
	else
		kStartWindowManager();
}

void MainForApplicationProcessor(void)
{
	QWORD qwTickCount;

	// set GDT Table
	kLoadGDTR(GDTR_STARTADDRESS);

	// set TSS Descriptor
	kLoadTR(GDT_TSSSEGMENT + (kGetAPICID() * sizeof(GDTENTRY16)));

	// set IDT Table
	kLoadIDTR(IDTR_STARTADDRESS);

	kInitializeScheduler();

	kEnableSoftwareLocalAPIC();

	kSetTaskPriority(0);

	kInitializeLocalVectorTable();

	kEnableInterrupt();

	kInitializeSystemCall();

	//kPrintf("Application Processor [APIC ID : %d] Is Activated \n",
	//		kGetAPICID());

	kIdleTask();
}

