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

void Main(void)
{
	int iCursorX, iCursorY;

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

	kPrintf("PIC Controller And Interrupt Initialize....................[    ]");
	kInitializePIC();
	kMaskPICInterrupt(0);
	kEnableInterrupt();
	kSetCursor(60, iCursorY++);
	kPrintf("PASS\n");

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

	kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0,
			(QWORD)kIdleTask);
	kStartConsoleShell();
}

