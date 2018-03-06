#include "Types.h"
#include "Window.h"
#include "WindowManagerTask.h"
#include "VBE.h"
#include "Mouse.h"
#include "Task.h"
#include "MultiProcessor.h"
#include "Utility.h"

void kStartWindowManager(void)
{
	VBEMODEINFOBLOCK* pstVBEMode;
	int iRelativeX, iRelativeY;
	int iMouseX, iMouseY;
	BYTE bButton;
	QWORD qwWindowID;
	TCB* pstTask;
	char vcTempTitle[WINDOW_TITLEMAXLENGTH];
	int iWindowCount = 0;

	pstTask = kGetRunningTask(kGetAPICID());
	
	kInitializeGUISystem();

	kGetCursorPosition(&iMouseX, &iMouseY);
	kMoveCursor(iMouseX, iMouseY);

	while (1)
	{
		if (kGetMouseDataFromMouseQueue(&bButton, &iRelativeX, &iRelativeY) ==
				FALSE)
		{
			kSleep(0);
			continue;
		}

		kGetCursorPosition(&iMouseX, &iMouseY);

		iMouseX += iRelativeX;
		iMouseY += iRelativeY;

		if (bButton & MOUSE_LBUTTONDOWN)
		{
			// create window
			kSPrintf(vcTempTitle, "MINT64 OS Test Window %d", iWindowCount++);
			qwWindowID = kCreateWindow(iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2,
					400, 200, WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLE, vcTempTitle);

			kDrawText(qwWindowID, 10, WINDOW_TITLEBAR_HEIGHT + 10, RGB(0, 0, 0),
					WINDOW_COLOR_BACKGROUND, "This is real window~!!", 22);
			kDrawText(qwWindowID, 10, WINDOW_TITLEBAR_HEIGHT + 30, RGB(0, 0, 0),
					WINDOW_COLOR_BACKGROUND, "No more prototype~!!", 18);
			kShowWindow(qwWindowID, TRUE);
		}
		else if (bButton & MOUSE_RBUTTONDOWN)
		{
			kDeleteAllWindowInTaskID(pstTask->stLink.qwID);
			iWindowCount = 0;
		}

		kMoveCursor(iMouseX, iMouseY);
	}
}

void kStartGraphicModeTest(void)
{
	VBEMODEINFOBLOCK* pstVBEMode;
	int iX, iY;
	COLOR* pstVideoMemory;
	RECT stScreenArea;
	int iRelativeX, iRelativeY;
	BYTE bButton;

	pstVBEMode = kGetVBEModeInfoBlock();

	stScreenArea.iX1 = 0;
	stScreenArea.iY1 = 0;
	stScreenArea.iX2 = pstVBEMode->wXResolution - 1;
	stScreenArea.iY2 = pstVBEMode->wYResolution - 1;

	pstVideoMemory = (COLOR*)((QWORD)pstVBEMode->dwPhysicalBasePointer &
			0xFFFFFFFF);

	iX = pstVBEMode->wXResolution / 2;
	iY = pstVBEMode->wYResolution / 2;

	/*
	kInternalDrawRect(&stScreenArea, pstVideoMemory,
			stScreenArea.iX1, stScreenArea.iY1, stScreenArea.iX2, stScreenArea.iY2,
			RGB(232, 255, 232), TRUE);

	kDrawCursor(iX, iY);

	while (1)
	{
		if (kGetMouseDataFromMouseQueue(&bButton, &iRelativeX, &iRelativeY) ==
				FALSE)
		{
			kSleep(1);
			continue;
		}

		kInternalDrawRect(&stScreenArea, pstVideoMemory, iX, iY,
				iX + MOUSE_CURSOR_WIDTH, iY + MOUSE_CURSOR_HEIGHT,
				RGB(232, 255, 232), TRUE);

		iX += iRelativeX;
		iY += iRelativeY;

		if (iX < stScreenArea.iX1)
			iX = stScreenArea.iX1;
		else if (iX > stScreenArea.iX2)
			iX = stScreenArea.iX2;

		if (iY < stScreenArea.iY1)
			iY = stScreenArea.iY1;
		else if (iY > stScreenArea.iY2)
			iY = stScreenArea.iY2;

		if (bButton & MOUSE_LBUTTONDOWN)
			kDrawWindowFrame(iX - 10, iY - 10, 400, 200, "MINT64 OS Test Window");
		else if (bButton & MOUSE_RBUTTONDOWN)
			kInternalDrawRect(&stScreenArea, pstVideoMemory, 
					stScreenArea.iX1, stScreenArea.iY1, stScreenArea.iX2, stScreenArea.iY2,
					RGB(232, 255, 232), TRUE);

		kDrawCursor(iX, iY);
	}
	*/
}


