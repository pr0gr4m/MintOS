#include "GUITask.h"
#include "Window.h"
#include "Utility.h"
#include "Console.h"
#include "Font.h"
#include "MultiProcessor.h"
#include "MPConfigurationTable.h"
#include "DynamicMemory.h"
#include "Task.h"
#include "ConsoleShell.h"
#include "FileSystem.h"
#include "JPEG.h"

void kBaseGUITask(void)
{
	QWORD qwWindowID;
	int iMouseX, iMouseY;
	int iWindowWidth, iWindowHeight;
	EVENT stReceivedEvent;
	MOUSEEVENT* pstMouseEvent;
	KEYEVENT* pstKeyEvent;
	WINDOWEVENT* pstWindowEvent;

	if (kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI Mode~!\n");
		return;
	}

	kGetCursorPosition(&iMouseX, &iMouseY);

	iWindowWidth = 500;
	iWindowHeight = 200;

	qwWindowID = kCreateWindow(iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2,
			iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE, 
			"Hello World Window");
	if (qwWindowID == WINDOW_INVALIDID)
		return;

	while (1)
	{
		if (kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			kSleep(0);
			continue;
		}

		switch (stReceivedEvent.qwType)
		{
			case EVENT_MOUSE_MOVE:
			case EVENT_MOUSE_LBUTTONDOWN:
			case EVENT_MOUSE_LBUTTONUP:
			case EVENT_MOUSE_RBUTTONDOWN:
			case EVENT_MOUSE_RBUTTONUP:
			case EVENT_MOUSE_MBUTTONDOWN:
			case EVENT_MOUSE_MBUTTONUP:
				pstMouseEvent = &(stReceivedEvent.stMouseEvent);
				break;

			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:
				pstKeyEvent = &(stReceivedEvent.stKeyEvent);
				break;

			case EVENT_WINDOW_SELECT:
			case EVENT_WINDOW_DESELECT:
			case EVENT_WINDOW_MOVE:
			case EVENT_WINDOW_RESIZE:
			case EVENT_WINDOW_CLOSE:
				pstWindowEvent = &(stReceivedEvent.stWindowEvent);

				if (stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
				{
					kDeleteWindow(qwWindowID);
					return;
				}
				
				break;

			default:
				break;
		}
	}
}

void kHelloWorldGUITask(void)
{
	QWORD qwWindowID;
	int iMouseX, iMouseY;
	int iWindowWidth, iWindowHeight;
	EVENT stReceivedEvent;
	MOUSEEVENT* pstMouseEvent;
	KEYEVENT* pstKeyEvent;
	WINDOWEVENT* pstWindowEvent;
	int i, iY;
	char vcTempBuffer[50];
	static int s_iWindowCount = 0;
	RECT stButtonArea;
	QWORD qwFindWindowID;
	EVENT stSendEvent;
	char* vpcEventString[] = {
		"Unknown          ",
		"MOUSE_MOVE       ",
		"MOUSE_LBUTTONDOWN",
		"MOUSE_LBUTTONUP  ",
		"MOUSE_RBUTTONDOWN",
		"MOUSE_RBUTTONUP  ",
		"MOUSE_MBUTTONDOWN",
		"MOUSE_MBUTTONUP  ",
		"WINDOW_SELECT    ",
		"WINDOW_DESELECT  ",
		"WINDOW_MOVE      ",
		"WINDOW_RESIZE    ",
		"WINDOW_CLOSE     ",
		"KEY_DOWN         ",
		"KEY_UP           " };

	if (kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI Mode~!\n");
		return;
	}

	kGetCursorPosition(&iMouseX, &iMouseY);

	iWindowWidth = 500;
	iWindowHeight = 200;

	kSPrintf(vcTempBuffer, "Hello World Window %d", s_iWindowCount++);
	qwWindowID = kCreateWindow(iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2,
			iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT, vcTempBuffer);
	if (qwWindowID == WINDOW_INVALIDID)
		return;

	// event area
	iY = WINDOW_TITLEBAR_HEIGHT + 10;

	kDrawRect(qwWindowID, 10, iY + 8, iWindowWidth - 10, iY + 70, RGB(0, 0, 0),
			FALSE);
	kSPrintf(vcTempBuffer, "GUI Event Information[Window ID: 0x%Q]", qwWindowID);
	kDrawText(qwWindowID, 20, iY, RGB(0, 0, 0), RGB(255, 255, 255),
			vcTempBuffer, kStrLen(vcTempBuffer));

	// button area
	kSetRectangleData(10, iY + 80, iWindowWidth - 10, iWindowHeight - 10,
			&stButtonArea);
	kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND,
			"User Message Send Button(UP)", RGB(0, 0, 0));
	kShowWindow(qwWindowID, TRUE);


	while (1)
	{
		if (kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			kSleep(0);
			continue;
		}

		// clear previous area
		kDrawRect(qwWindowID, 11, iY + 20, iWindowWidth - 11, iY + 69,
				WINDOW_COLOR_BACKGROUND, TRUE);

		switch (stReceivedEvent.qwType)
		{
			case EVENT_MOUSE_MOVE:
			case EVENT_MOUSE_LBUTTONDOWN:
			case EVENT_MOUSE_LBUTTONUP:
			case EVENT_MOUSE_RBUTTONDOWN:
			case EVENT_MOUSE_RBUTTONUP:
			case EVENT_MOUSE_MBUTTONDOWN:
			case EVENT_MOUSE_MBUTTONUP:
				pstMouseEvent = &(stReceivedEvent.stMouseEvent);

				kSPrintf(vcTempBuffer, "Mouse Event: %s",
						vpcEventString[stReceivedEvent.qwType]);
				kDrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0),
						WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));
				
				kSPrintf(vcTempBuffer, "Data: X = %d, Y = %d, Button = %X",
						pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY,
						pstMouseEvent->bButtonStatus);
				kDrawText(qwWindowID, 20, iY + 40, RGB(0, 0, 0),
						WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

				if (stReceivedEvent.qwType == EVENT_MOUSE_LBUTTONDOWN)
				{
					if (kIsInRectangle(&stButtonArea, pstMouseEvent->stPoint.iX,
								pstMouseEvent->stPoint.iY) == TRUE)
					{
						kDrawButton(qwWindowID, &stButtonArea,
								RGB(79, 204, 11), "User Message Send Button(DOWN)",
								RGB(255, 255, 255));
						kUpdateScreenByID(qwWindowID);

						stSendEvent.qwType = EVENT_USER_TESTMESSAGE;
						stSendEvent.vqwData[0] = qwWindowID;
						stSendEvent.vqwData[1] = 0x1234;
						stSendEvent.vqwData[2] = 0x5678;

						for (i = 0; i < s_iWindowCount; i++)
						{
							kSPrintf(vcTempBuffer, "Hello World Window %d", i);
							qwFindWindowID = kFindWindowByTitle(vcTempBuffer);

							if ((qwFindWindowID != WINDOW_INVALIDID) &&
									(qwFindWindowID != qwWindowID))
								kSendEventToWindow(qwFindWindowID, &stSendEvent);
						}
					}
				}
				else if (stReceivedEvent.qwType == EVENT_MOUSE_LBUTTONUP)
				{
					kDrawButton(qwWindowID, &stButtonArea,
							WINDOW_COLOR_BACKGROUND, "User Message Send Button(UP)",
							RGB(0, 0, 0));
				}

				break;

			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:
				pstKeyEvent = &(stReceivedEvent.stKeyEvent);

				kSPrintf(vcTempBuffer, "Key Event: %s",
						vpcEventString[stReceivedEvent.qwType]);
				kDrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0),
						WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

				kSPrintf(vcTempBuffer, "Data: Key = %c, Flag = %X",
						pstKeyEvent->bASCIICode, pstKeyEvent->bFlags);
				kDrawText(qwWindowID, 20, iY + 40, RGB(0, 0, 0),
						WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));
				break;

			case EVENT_WINDOW_SELECT:
			case EVENT_WINDOW_DESELECT:
			case EVENT_WINDOW_MOVE:
			case EVENT_WINDOW_RESIZE:
			case EVENT_WINDOW_CLOSE:
				pstWindowEvent = &(stReceivedEvent.stWindowEvent);

				kSPrintf(vcTempBuffer, "Window Event: %s",
						vpcEventString[stReceivedEvent.qwType]);
				kDrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0),
						WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

				kSPrintf(vcTempBuffer, "Data: X1 = %d, Y1 = %d, X2 = %d, Y2 = %d",
						pstWindowEvent->stArea.iX1, pstWindowEvent->stArea.iY1,
						pstWindowEvent->stArea.iX2, pstWindowEvent->stArea.iY2);
				kDrawText(qwWindowID, 20, iY + 40, RGB(0, 0, 0),
						WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

				if (stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
				{
					kDeleteWindow(qwWindowID);
					return;
				}
				
				break;

			default:
				kSPrintf(vcTempBuffer, "Unknown Event: 0x%X", stReceivedEvent.qwType);
				kDrawText(qwWindowID, 20, iY + 20, RGB(0, 0, 0),
						WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));

				kSPrintf(vcTempBuffer, "Data0 = 0x%Q, Data1 = 0x%Q, Data2 = 0x%Q",
						stReceivedEvent.vqwData[0], stReceivedEvent.vqwData[1],
						stReceivedEvent.vqwData[2]);
				kDrawText(qwWindowID, 20, iY + 40, RGB(0, 0, 0),
						WINDOW_COLOR_BACKGROUND, vcTempBuffer, kStrLen(vcTempBuffer));
				break;
		}

		kShowWindow(qwWindowID, TRUE);
	}
}

void kSystemMonitorTask(void)
{
	QWORD qwWindowID;
	int i;
	int iWindowWidth;
	int iProcessorCount;
	DWORD vdwLastCPULoad[MAXPROCESSORCOUNT];
	int viLastTaskCount[MAXPROCESSORCOUNT];
	QWORD qwLastTickCount;
	EVENT stReceivedEvent;
	WINDOWEVENT* pstWindowEvent;
	BOOL bChanged;
	RECT stScreenArea;
	QWORD qwLastDynamicMemoryUsedSize;
	QWORD qwDynamicMemoryUsedSize;
	QWORD qwTemp;

	if (kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI Mode.\n");
		return;
	}

	kGetScreenArea(&stScreenArea);

	iProcessorCount = kGetProcessorCount();
	iWindowWidth = iProcessorCount * (SYSTEMMONITOR_PROCESSOR_WIDTH +
			SYSTEMMONITOR_PROCESSOR_MARGIN) + SYSTEMMONITOR_PROCESSOR_MARGIN;

	// create system monitor window
	qwWindowID = kCreateWindow((stScreenArea.iX2 - iWindowWidth) / 2,
			(stScreenArea.iY2 - SYSTEMMONITOR_WINDOW_HEIGHT) / 2,
			iWindowWidth, SYSTEMMONITOR_WINDOW_HEIGHT, WINDOW_FLAGS_DEFAULT &
			~WINDOW_FLAGS_SHOW, "System Monitor");
	if (qwWindowID == WINDOW_INVALIDID)
		return;

	// draw processor area
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + 15, iWindowWidth - 5,
			WINDOW_TITLEBAR_HEIGHT + 15, RGB(0, 0, 0));
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + 16, iWindowWidth - 5,
			WINDOW_TITLEBAR_HEIGHT + 16, RGB(0, 0, 0));
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + 17, iWindowWidth - 5,
			WINDOW_TITLEBAR_HEIGHT + 17, RGB(0, 0, 0));
	kDrawText(qwWindowID, 9, WINDOW_TITLEBAR_HEIGHT + 8, RGB(0, 0, 0),
			WINDOW_COLOR_BACKGROUND, "Processor Information", 21);

	// draw memory area
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 50,
			iWindowWidth - 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 50,
			RGB(0, 0, 0));
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 51,
			iWindowWidth - 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 51,
			RGB(0, 0, 0));
	kDrawLine(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 52,
			iWindowWidth - 5, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 52,
			RGB(0, 0, 0));
	kDrawText(qwWindowID, 9, WINDOW_TITLEBAR_HEIGHT + SYSTEMMONITOR_PROCESSOR_HEIGHT + 43,
			RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "Memory Information", 18);

	kShowWindow(qwWindowID, TRUE);

	qwLastTickCount = 0;

	kMemSet(vdwLastCPULoad, 0, sizeof(vdwLastCPULoad));
	kMemSet(viLastTaskCount, 0, sizeof(viLastTaskCount));
	qwLastDynamicMemoryUsedSize = 0;

	while (1)
	{
		if (kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == TRUE)
		{
			switch (stReceivedEvent.qwType)
			{
				case EVENT_WINDOW_CLOSE:
					kDeleteWindow(qwWindowID);
					return;

				default:
					break;
			}
		}

		if ((kGetTickCount() - qwLastTickCount) < 500)
		{
			kSleep(1);
			continue;
		}

		qwLastTickCount = kGetTickCount();

		// print processor information
		for (i = 0; i < iProcessorCount; i++)
		{
			bChanged = FALSE;

			if (vdwLastCPULoad[i] != kGetProcessorLoad(i))
			{
				vdwLastCPULoad[i] = kGetProcessorLoad(i);
				bChanged = TRUE;
			}

			if (viLastTaskCount[i] != kGetTaskCount(i))
			{
				viLastTaskCount[i] = kGetTaskCount(i);
				bChanged = TRUE;
			}

			if (bChanged == TRUE)
			{
				kDrawProcessorInformation(qwWindowID, i * SYSTEMMONITOR_PROCESSOR_WIDTH +
						(i + 1) * SYSTEMMONITOR_PROCESSOR_MARGIN, WINDOW_TITLEBAR_HEIGHT + 28,
						i);
			}
		}

		kGetDynamicMemoryInformation(&qwTemp, &qwTemp, &qwTemp,
				&qwDynamicMemoryUsedSize);

		// print memory information
		if (qwDynamicMemoryUsedSize != qwLastDynamicMemoryUsedSize)
		{
			qwLastDynamicMemoryUsedSize = qwDynamicMemoryUsedSize;

			kDrawMemoryInformation(qwWindowID, WINDOW_TITLEBAR_HEIGHT +
					SYSTEMMONITOR_PROCESSOR_HEIGHT + 60, iWindowWidth);
		}
	}
}

static void kDrawProcessorInformation(QWORD qwWindowID, int iX, int iY, BYTE bAPICID)
{
	char vcBuffer[100];
	RECT stArea;
	QWORD qwProcessorLoad;
	QWORD iUsageBarHeight;
	int iMiddleX;

	kSPrintf(vcBuffer, "Processor ID: %d", bAPICID);
	kDrawText(qwWindowID, iX + 10, iY, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND,
			vcBuffer, kStrLen(vcBuffer));

	kSPrintf(vcBuffer, "Task Count: %d   ", kGetTaskCount(bAPICID));
	kDrawText(qwWindowID, iX + 10, iY + 18, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND,
			vcBuffer, kStrLen(vcBuffer));

	// Processor Load Graph
	qwProcessorLoad = kGetProcessorLoad(bAPICID);
	if (qwProcessorLoad > 100)
		qwProcessorLoad = 100;

	// total graph
	kDrawRect(qwWindowID, iX, iY + 36, iX + SYSTEMMONITOR_PROCESSOR_WIDTH,
			iY + SYSTEMMONITOR_PROCESSOR_HEIGHT, RGB(0, 0, 0), FALSE);

	// usage = total * load / 100;
	iUsageBarHeight = (SYSTEMMONITOR_PROCESSOR_HEIGHT - 40) * qwProcessorLoad / 100;

	// usage graph
	kDrawRect(qwWindowID, iX + 2, 
			iY + (SYSTEMMONITOR_PROCESSOR_HEIGHT - iUsageBarHeight) - 2,
			iX + SYSTEMMONITOR_PROCESSOR_WIDTH - 2,
			iY + SYSTEMMONITOR_PROCESSOR_HEIGHT - 2, SYSTEMMONITOR_BAR_COLOR, TRUE);
	// blank graph
	kDrawRect(qwWindowID, iX + 2, iY + 38, iX + SYSTEMMONITOR_PROCESSOR_WIDTH - 2,
			iY + (SYSTEMMONITOR_PROCESSOR_HEIGHT - iUsageBarHeight) - 1,
			WINDOW_COLOR_BACKGROUND, TRUE);

	kSPrintf(vcBuffer, "Usage: %d%%", qwProcessorLoad);
	iMiddleX = (SYSTEMMONITOR_PROCESSOR_WIDTH -
			(kStrLen(vcBuffer) * FONT_ENGLISHWIDTH)) / 2;
	kDrawText(qwWindowID, iX + iMiddleX, iY + 80, RGB(0, 0, 0),
			WINDOW_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));

	kSetRectangleData(iX, iY, iX + SYSTEMMONITOR_PROCESSOR_WIDTH,
			iY + SYSTEMMONITOR_PROCESSOR_HEIGHT, &stArea);
	kUpdateScreenByWindowArea(qwWindowID, &stArea);
}

static void kDrawMemoryInformation(QWORD qwWindowID, int iY, int iWindowWidth)
{
	char vcBuffer[100];
	QWORD qwTotalRAMKByteSize;
	QWORD qwDynamicMemoryStartAddress;
	QWORD qwDynamicMemoryUsedSize;
	QWORD qwUsedPercent;
	QWORD qwTemp;
	int iUsageBarWidth;
	RECT stArea;
	int iMiddleX;

	qwTotalRAMKByteSize = kGetTotalRAMSize() * 1024;

	kSPrintf(vcBuffer, "Total Size: %d KB        ", qwTotalRAMKByteSize);
	kDrawText(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 10, iY + 3, RGB(0, 0, 0),
			WINDOW_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));

	kGetDynamicMemoryInformation(&qwDynamicMemoryStartAddress, &qwTemp,
			&qwTemp, &qwDynamicMemoryUsedSize);

	kSPrintf(vcBuffer, "Used Size: %d KB        ", (qwDynamicMemoryUsedSize +
				qwDynamicMemoryStartAddress) / 1024);
	kDrawText(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 10, iY + 21, RGB(0, 0, 0),
			WINDOW_COLOR_BACKGROUND, vcBuffer, kStrLen(vcBuffer));

	// draw graph

	kDrawRect(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN, iY + 40,
			iWindowWidth - SYSTEMMONITOR_PROCESSOR_MARGIN,
			iY + SYSTEMMONITOR_MEMORY_HEIGHT - 32, RGB(0, 0, 0), FALSE);

	// usage = (Dynamic Start + Dynamic Used) * 100 / total
	qwUsedPercent = (qwDynamicMemoryStartAddress + qwDynamicMemoryUsedSize) *
		100 / 1024 / qwTotalRAMKByteSize;
	if (qwUsedPercent > 100)
		qwUsedPercent = 100;

	iUsageBarWidth = (iWindowWidth - 2 * SYSTEMMONITOR_PROCESSOR_MARGIN) *
		qwUsedPercent / 100;

	// usage graph
	kDrawRect(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 2, iY + 42,
			SYSTEMMONITOR_PROCESSOR_MARGIN + 2 + iUsageBarWidth,
			iY + SYSTEMMONITOR_MEMORY_HEIGHT - 34, SYSTEMMONITOR_BAR_COLOR, TRUE);
	// blank graphc
	kDrawRect(qwWindowID, SYSTEMMONITOR_PROCESSOR_MARGIN + 2 + iUsageBarWidth, 
			iY + 42, iWindowWidth - SYSTEMMONITOR_PROCESSOR_MARGIN - 2,
			iY + SYSTEMMONITOR_MEMORY_HEIGHT - 34, WINDOW_COLOR_BACKGROUND, TRUE);

	kSPrintf(vcBuffer, "Usage: %d%%", qwUsedPercent);
	iMiddleX = (iWindowWidth - (kStrLen(vcBuffer) * FONT_ENGLISHWIDTH)) / 2;
	kDrawText(qwWindowID, iMiddleX, iY + 45, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND,
			vcBuffer, kStrLen(vcBuffer));

	kSetRectangleData(0, iY, iWindowWidth, iY + SYSTEMMONITOR_MEMORY_HEIGHT, &stArea);
	kUpdateScreenByWindowArea(qwWindowID, &stArea);
}


// console task functions

static CHARACTER gs_vstPreviousScreenBuffer[CONSOLE_WIDTH * CONSOLE_HEIGHT];

void kGUIConsoleShellTask(void)
{
	static QWORD s_qwWindowID = WINDOW_INVALIDID;
	int iWindowWidth, iWindowHeight;
	EVENT stReceivedEvent;
	KEYEVENT* pstKeyEvent;
	RECT stScreenArea;
	KEYDATA stKeyData;
	TCB* pstConsoleShellTask;
	QWORD qwConsoleShellTaskID;

	if (kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI Mode.\n");
		return;
	}

	if (s_qwWindowID != WINDOW_INVALIDID)
	{
		kMoveWindowToTop(s_qwWindowID);
		return;
	}

	kGetScreenArea(&stScreenArea);

	iWindowWidth = CONSOLE_WIDTH * FONT_ENGLISHWIDTH + 4;
	iWindowHeight = CONSOLE_HEIGHT * FONT_ENGLISHHEIGHT + WINDOW_TITLEBAR_HEIGHT + 2;

	// create window
	s_qwWindowID = kCreateWindow((stScreenArea.iX2 - iWindowWidth) / 2,
			(stScreenArea.iY2 - iWindowHeight) / 2, iWindowWidth, iWindowHeight,
			WINDOW_FLAGS_DEFAULT, "Console Shell for GUI");

	if (s_qwWindowID == WINDOW_INVALIDID)
		return;

	// create console shell task
	kSetConsoleShellExitFlag(FALSE);
	pstConsoleShellTask = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0,
			(QWORD)kStartConsoleShell, TASK_LOADBALANCINGID);
	if (pstConsoleShellTask == NULL)
	{
		kDeleteWindow(s_qwWindowID);
		return;
	}

	qwConsoleShellTaskID = pstConsoleShellTask->stLink.qwID;
	// init previous buffer
	kMemSet(gs_vstPreviousScreenBuffer, 0xFF, sizeof(gs_vstPreviousScreenBuffer));

	// event handling loop
	while (1)
	{
		kProcessConsoleBuffer(s_qwWindowID);

		if (kReceiveEventFromWindowQueue(s_qwWindowID, &stReceivedEvent) == FALSE)
		{
			kSleep(0);
			continue;
		}

		switch (stReceivedEvent.qwType)
		{
			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:
				pstKeyEvent = &(stReceivedEvent.stKeyEvent);
				stKeyData.bASCIICode = pstKeyEvent->bASCIICode;
				stKeyData.bFlags = pstKeyEvent->bFlags;
				stKeyData.bScanCode = pstKeyEvent->bScanCode;

				// put key to GUI key queue
				kPutKeyToGUIKeyQueue(&stKeyData);
				break;

			case EVENT_WINDOW_CLOSE:
				kSetConsoleShellExitFlag(TRUE);
				while (kIsTaskExist(qwConsoleShellTaskID) == TRUE)
					kSleep(1);

				kDeleteWindow(s_qwWindowID);
				s_qwWindowID = WINDOW_INVALIDID;
				return;

			default:
				break;
		}
	}
}

static void kProcessConsoleBuffer(QWORD qwWindowID)
{
	int i, j;
	CONSOLEMANAGER* pstManager;
	CHARACTER* pstScreenBuffer;
	CHARACTER* pstPreviousScreenBuffer;
	RECT stLineArea;
	BOOL bChanged;
	static QWORD s_qwLastTickCount = 0;
	BOOL bFullRedraw = FALSE;

	pstManager = kGetConsoleManager();
	pstScreenBuffer = pstManager->pstScreenBuffer;
	pstPreviousScreenBuffer = gs_vstPreviousScreenBuffer;

	if (kGetTickCount() - s_qwLastTickCount > 5000)
	{
		s_qwLastTickCount = kGetTickCount();
		bFullRedraw = TRUE;
	}

	for (j = 0; j < CONSOLE_HEIGHT; j++)
	{
		bChanged = FALSE;

		for (i = 0; i < CONSOLE_WIDTH; i++)
		{
			if ((pstScreenBuffer->bCharacter != pstPreviousScreenBuffer->bCharacter) ||
					(bFullRedraw == TRUE))
			{
				kDrawText(qwWindowID, i * FONT_ENGLISHWIDTH + 2,
						j * FONT_ENGLISHHEIGHT + WINDOW_TITLEBAR_HEIGHT,
						RGB(0, 255, 0), RGB(0, 0, 0),
						&(pstScreenBuffer->bCharacter), 1);

				kMemCpy(pstPreviousScreenBuffer, pstScreenBuffer, sizeof(CHARACTER));
				bChanged = TRUE;
			}

			pstScreenBuffer++;
			pstPreviousScreenBuffer++;
		}

		if (bChanged == TRUE)
		{
			kSetRectangleData(2, j * FONT_ENGLISHHEIGHT + WINDOW_TITLEBAR_HEIGHT,
					5 + FONT_ENGLISHWIDTH * CONSOLE_WIDTH, (j + 1) * FONT_ENGLISHHEIGHT +
					WINDOW_TITLEBAR_HEIGHT - 1, &stLineArea);
			kUpdateScreenByWindowArea(qwWindowID, &stLineArea);
		}
	}
}

// image viewer functions

void kImageViewerTask(void)
{
	QWORD qwWindowID;
	int iMouseX, iMouseY;
	int iWindowWidth, iWindowHeight;
	int iEditBoxWidth;
	RECT stEditBoxArea;
	RECT stButtonArea;
	RECT stScreenArea;
	EVENT stReceivedEvent;
	EVENT stSendEvent;
	char vcFileName[FILESYSTEM_MAXFILENAMELENGTH + 1];
	int iFileNameLength;
	MOUSEEVENT* pstMouseEvent;
	KEYEVENT* pstKeyEvent;
	POINT stScreenXY, stClientXY;

	if (kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI Mode.\n");
		return;
	}

	kGetScreenArea(&stScreenArea);

	iWindowWidth = FONT_ENGLISHWIDTH * FILESYSTEM_MAXFILENAMELENGTH + 165;
	iWindowHeight = 35 + WINDOW_TITLEBAR_HEIGHT + 5;

	qwWindowID = kCreateWindow((stScreenArea.iX2 - iWindowWidth) / 2,
			(stScreenArea.iY2 - iWindowHeight) / 2, iWindowWidth, iWindowHeight,
			WINDOW_FLAGS_DEFAULT & ~WINDOW_FLAGS_SHOW, "Image Viewer");
	if (qwWindowID == WINDOW_INVALIDID)
		return;

	kDrawText(qwWindowID, 5, WINDOW_TITLEBAR_HEIGHT + 6, RGB(0, 0, 0),
			WINDOW_COLOR_BACKGROUND, "FILE NAME", 9);
	iEditBoxWidth = FONT_ENGLISHWIDTH * FILESYSTEM_MAXFILENAMELENGTH + 4;
	kSetRectangleData(85, WINDOW_TITLEBAR_HEIGHT + 5, 85 + iEditBoxWidth,
			WINDOW_TITLEBAR_HEIGHT + 25, &stEditBoxArea);
	kDrawRect(qwWindowID, stEditBoxArea.iX1, stEditBoxArea.iY1,
			stEditBoxArea.iX2, stEditBoxArea.iY2, RGB(0, 0, 0), FALSE);

	// blank file name
	iFileNameLength = 0;
	kMemSet(vcFileName, 0, sizeof(vcFileName));
	kDrawFileName(qwWindowID, &stEditBoxArea, vcFileName, iFileNameLength);

	// button
	kSetRectangleData(stEditBoxArea.iX2 + 10, stEditBoxArea.iY1,
			stEditBoxArea.iX2 + 70, stEditBoxArea.iY2, &stButtonArea);
	kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "Show",
			RGB(0, 0, 0));

	kShowWindow(qwWindowID, TRUE);

	// event loop
	while (1)
	{
		if (kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			kSleep(0);
			continue;
		}

		switch (stReceivedEvent.qwType)
		{
			case EVENT_MOUSE_LBUTTONDOWN:
				pstMouseEvent = &(stReceivedEvent.stMouseEvent);

				if (kIsInRectangle(&stButtonArea, pstMouseEvent->stPoint.iX,
							pstMouseEvent->stPoint.iY) == TRUE)
				{
					// push button
					kDrawButton(qwWindowID, &stButtonArea, RGB(79, 204, 11), "Show",
							RGB(255, 255, 255));
					kUpdateScreenByWindowArea(qwWindowID, &stButtonArea);

					if (kCreateImageViewerWindowAndExecute(qwWindowID, vcFileName)
							== FALSE)
					{
						kSleep(200);
					}

					kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND,
							"Show", RGB(0, 0, 0));
					kUpdateScreenByWindowArea(qwWindowID, &stButtonArea);
				}

				break;

			case EVENT_KEY_DOWN:
				pstKeyEvent = &(stReceivedEvent.stKeyEvent);

				if ((pstKeyEvent->bASCIICode == KEY_BACKSPACE) &&
						(iFileNameLength > 0))
				{
					vcFileName[iFileNameLength] = '\0';
					iFileNameLength--;

					kDrawFileName(qwWindowID, &stEditBoxArea, vcFileName,
							iFileNameLength);
				}
				else if ((pstKeyEvent->bASCIICode == KEY_ENTER) &&
						(iFileNameLength > 0))
				{
					stClientXY.iX = stButtonArea.iX1 + 1;
					stClientXY.iY = stButtonArea.iY1 + 1;
					kConvertPointClientToScreen(qwWindowID, &stClientXY, &stScreenXY);

					// push button event
					kSetMouseEvent(qwWindowID, EVENT_MOUSE_LBUTTONDOWN,
							stScreenXY.iX + 1, stScreenXY.iY + 1, 0, &stSendEvent);
					kSendEventToWindow(qwWindowID, &stSendEvent);
				}
				else if (pstKeyEvent->bASCIICode == KEY_ESC)
				{
					kSetWindowEvent(qwWindowID, EVENT_WINDOW_CLOSE, &stSendEvent);
					kSendEventToWindow(qwWindowID, &stSendEvent);
				}
				else if ((pstKeyEvent->bASCIICode <= 128) &&
						(pstKeyEvent->bASCIICode != KEY_BACKSPACE) &&
						(iFileNameLength < FILESYSTEM_MAXFILENAMELENGTH))
				{
					vcFileName[iFileNameLength++] = pstKeyEvent->bASCIICode;

					kDrawFileName(qwWindowID, &stEditBoxArea, vcFileName,
							iFileNameLength);
				}
				
				break;

			case EVENT_WINDOW_CLOSE:
				if (stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
				{
					kDeleteWindow(qwWindowID);
					return;
				}
				break;

			default:
				break;
		}
	}
}

static void kDrawFileName(QWORD qwWindowID, RECT* pstArea, char* pcFileName,
		int iNameLength)
{
	// background
	kDrawRect(qwWindowID, pstArea->iX1 + 1, pstArea->iY1 + 1, pstArea->iX2 - 1,
			pstArea->iY2 - 1, WINDOW_COLOR_BACKGROUND, TRUE);

	// file name
	kDrawText(qwWindowID, pstArea->iX1 + 2, pstArea->iY1 + 2, RGB(0, 0, 0),
			WINDOW_COLOR_BACKGROUND, pcFileName, iNameLength);

	if (iNameLength < FILESYSTEM_MAXFILENAMELENGTH)
	{
		kDrawText(qwWindowID, pstArea->iX1 + 2 + FONT_ENGLISHWIDTH * iNameLength,
				pstArea->iY1 + 2, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "_", 1);
	}

	kUpdateScreenByWindowArea(qwWindowID, pstArea);
}

static BOOL kCreateImageViewerWindowAndExecute(QWORD qwMainWindowID,
		const char* pcFileName)
{
	DIR* pstDirectory;
	struct dirent* pstEntry;
	DWORD dwFileSize = 0;
	RECT stScreenArea;
	QWORD qwWindowID = WINDOW_INVALIDID;
	WINDOW* pstWindow = NULL;
	BYTE* pbFileBuffer = NULL;
	COLOR* pstOutputBuffer = NULL;
	int iWindowWidth;
	FILE* fp = NULL;
	JPEG* pstJpeg;
	EVENT stReceivedEvent;
	KEYEVENT* pstKeyEvent;
	BOOL bExit;

	pstDirectory = opendir("/");

	// search file
	while (1)
	{
		pstEntry = readdir(pstDirectory);
		if (pstEntry == NULL)
			break;

		if ((kStrLen(pstEntry->d_name) == kStrLen(pcFileName)) &&
				(kMemCmp(pstEntry->d_name, pcFileName, kStrLen(pcFileName)) == 0))
		{
			dwFileSize = pstEntry->dwFileSize;
			break;
		}
	}
	
	closedir(pstDirectory);

	if (dwFileSize == 0)
	{
		kPrintf("[ImageViewer] %s file doesn't exist or size is zero\n", pcFileName);
		return FALSE;
	}

	if ((fp = fopen(pcFileName, "rb")) == NULL)
	{
		kPrintf("[ImageViewer] %s file open fail\n", pcFileName);
		return FALSE;
	}

	pbFileBuffer = (BYTE*)kAllocateMemory(dwFileSize);
	pstJpeg = (JPEG*)kAllocateMemory(sizeof(JPEG));
	if ((pbFileBuffer == NULL) || (pstJpeg == NULL))
	{
		kPrintf("[ImageViewer] Buffer allocation fail\n");
		kFreeMemory(pbFileBuffer);
		kFreeMemory(pstJpeg);
		fclose(fp);
		return FALSE;
	}

	if ((fread(pbFileBuffer, 1, dwFileSize, fp) != dwFileSize) ||
			(kJPEGInit(pstJpeg, pbFileBuffer, dwFileSize) == FALSE))
	{
		kPrintf("[ImageViewer] Read fail or file is not JPEG\n");
		kFreeMemory(pbFileBuffer);
		kFreeMemory(pstJpeg);
		fclose(fp);
		return FALSE;
	}
	fclose(fp);

	pstOutputBuffer = kAllocateMemory(pstJpeg->width * pstJpeg->height * sizeof(COLOR));
	if ((pstOutputBuffer != NULL) &&
			(kJPEGDecode(pstJpeg, pstOutputBuffer) == TRUE))
	{
		kGetScreenArea(&stScreenArea);
		qwWindowID = kCreateWindow((stScreenArea.iX2 - pstJpeg->width) / 2,
				(stScreenArea.iY2 - pstJpeg->height) / 2, pstJpeg->width,
				pstJpeg->height + WINDOW_TITLEBAR_HEIGHT,
				WINDOW_FLAGS_DEFAULT & ~WINDOW_FLAGS_SHOW | WINDOW_FLAGS_RESIZABLE, 
				pcFileName);
	}

	if ((qwWindowID == WINDOW_INVALIDID) || (pstOutputBuffer == NULL))
	{
		kPrintf("[ImageViewer] Window create fail or output buffer allocation fail\n");
		kFreeMemory(pbFileBuffer);
		kFreeMemory(pstJpeg);
		kFreeMemory(pstOutputBuffer);
		kDeleteWindow(qwWindowID);
		return FALSE;
	}

	// copy image to window
	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow != NULL)
	{
		iWindowWidth = kGetRectangleWidth(&(pstWindow->stArea));
		kMemCpy(pstWindow->pstWindowBuffer + (WINDOW_TITLEBAR_HEIGHT * 
					iWindowWidth), pstOutputBuffer, pstJpeg->width * 
				pstJpeg->height * sizeof(COLOR));
		kUnlock(&(pstWindow->stLock));
	}

	kFreeMemory(pbFileBuffer);
	kShowWindow(qwWindowID, TRUE);

	// hide file input window
	kShowWindow(qwMainWindowID, FALSE);

	bExit = FALSE;
	while (bExit == FALSE)
	{
		if (kReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			kSleep(0);
			continue;
		}

		switch (stReceivedEvent.qwType)
		{
			case EVENT_KEY_DOWN:
				pstKeyEvent = &(stReceivedEvent.stKeyEvent);

				if (pstKeyEvent->bASCIICode == KEY_ESC)
				{
					kDeleteWindow(qwWindowID);
					kShowWindow(qwMainWindowID, TRUE);
					bExit = TRUE;
				}

				break;

			case EVENT_WINDOW_RESIZE:
				kBitBlt(qwWindowID, 0, WINDOW_TITLEBAR_HEIGHT, pstOutputBuffer,
						pstJpeg->width, pstJpeg->height);
				kShowWindow(qwWindowID, TRUE);

				break;

			case EVENT_WINDOW_CLOSE:

				if (stReceivedEvent.qwType == EVENT_WINDOW_CLOSE)
				{
					kDeleteWindow(qwWindowID);
					kShowWindow(qwMainWindowID, TRUE);
					bExit = TRUE;
				}

				break;

			default:
				break;
		}
	}

	kFreeMemory(pstJpeg);
	kFreeMemory(pstOutputBuffer);
	return TRUE;
}
