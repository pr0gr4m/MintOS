#include "Types.h"
#include "Window.h"
#include "WindowManagerTask.h"
#include "VBE.h"
#include "Mouse.h"
#include "Task.h"
#include "MultiProcessor.h"
#include "Utility.h"
#include "GUITask.h"
#include "ApplicationPanelTask.h"

void kStartWindowManager(void)
{
	int iMouseX, iMouseY;
	BOOL bMouseDataResult;
	BOOL bKeyDataResult;
	BOOL bEventQueueResult;

	kInitializeGUISystem();

	kGetCursorPosition(&iMouseX, &iMouseY);
	kMoveCursor(iMouseX, iMouseY);

	kCreateTask(TASK_FLAGS_SYSTEM | TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0,
			(QWORD)kApplicationPanelGUITask, TASK_LOADBALANCINGID);

	while (1)
	{
		bMouseDataResult = kProcessMouseData();
		bKeyDataResult = kProcessKeyData();
		bEventQueueResult = FALSE;
		while (kProcessEventQueueData() == TRUE)
			bEventQueueResult = TRUE;

		if ((bMouseDataResult == FALSE) &&
				(bKeyDataResult == FALSE) &&
				(bEventQueueResult == FALSE))
		{
			kSleep(0);
		}
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


BOOL kProcessMouseData(void)
{
	QWORD qwWindowIDUnderMouse;
	BYTE bButtonStatus;
	int iRelativeX, iRelativeY;
	int iMouseX, iMouseY;
	int iPreviousMouseX, iPreviousMouseY;
	BYTE bChangedButton;
	RECT stWindowArea;
	EVENT stEvent;
	WINDOWMANAGER* pstWindowManager;
	char vcTempTitle[WINDOW_TITLEMAXLENGTH + 1];
	int i;

	pstWindowManager = kGetWindowManager();

	for (i = 0; i < WINDOWMANAGER_DATAACCUMULATECOUNT; i++)
	{
		if (kGetMouseDataFromMouseQueue(&bButtonStatus, &iRelativeX, &iRelativeY) ==
				FALSE)
		{
			if (i == 0)
				return FALSE;
			else
				break;
		}

		kGetCursorPosition(&iMouseX, &iMouseY);

		if (i == 0)
		{
			iPreviousMouseX = iMouseX;
			iPreviousMouseY = iMouseY;
		}

		iMouseX += iRelativeX;
		iMouseY += iRelativeY;

		kMoveCursor(iMouseX, iMouseY);
		kGetCursorPosition(&iMouseX, &iMouseY);

		bChangedButton = pstWindowManager->bPreviousButtonStatus ^ bButtonStatus;

		if (bChangedButton != 0)
			break;
	}

	qwWindowIDUnderMouse = kFindWindowByPoint(iMouseX, iMouseY);

	if (bChangedButton & MOUSE_LBUTTONDOWN)
	{
		if (bButtonStatus & MOUSE_LBUTTONDOWN)
		{
			if (qwWindowIDUnderMouse != pstWindowManager->qwBackgroundWindowID)
				kMoveWindowToTop(qwWindowIDUnderMouse);

			if (kIsInTitleBar(qwWindowIDUnderMouse, iMouseX, iMouseY) == TRUE)
			{
				if (kIsInCloseButton(qwWindowIDUnderMouse, iMouseX, iMouseY)
						== TRUE)
				{
					kSetWindowEvent(qwWindowIDUnderMouse, EVENT_WINDOW_CLOSE,
							&stEvent);
					kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);

					// Test
					kDeleteWindow(qwWindowIDUnderMouse);
				}
				else
				{
					pstWindowManager->bWindowMoveMode = TRUE;
					pstWindowManager->qwMovingWindowID = qwWindowIDUnderMouse;
				}
			}
			else
			{
				kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_LBUTTONDOWN,
						iMouseX, iMouseY, bButtonStatus, &stEvent);
				kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
			}
		}
		else	// left button up
		{
			if (pstWindowManager->bWindowMoveMode == TRUE)
			{
				pstWindowManager->bWindowMoveMode = FALSE;
				pstWindowManager->qwMovingWindowID = WINDOW_INVALIDID;
			}
			else
			{
				kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_LBUTTONUP,
						iMouseX, iMouseY, bButtonStatus, &stEvent);
				kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
			}
		}
	}
	else if (bChangedButton & MOUSE_RBUTTONDOWN)
	{
		if (bButtonStatus & MOUSE_RBUTTONDOWN)
		{
			kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_RBUTTONDOWN,
					iMouseX, iMouseY, bButtonStatus, &stEvent);
			kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);

			//kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, NULL, NULL,
			//		(QWORD)kHelloWorldGUITask, TASK_LOADBALANCINGID);
		}
		else
		{
			kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_RBUTTONUP,
					iMouseX, iMouseY, bButtonStatus, &stEvent);
			kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
		}
	}
	else if (bChangedButton & MOUSE_MBUTTONDOWN)
	{
		if (bButtonStatus & MOUSE_MBUTTONDOWN)
		{
			kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MBUTTONDOWN,
					iMouseX, iMouseY, bButtonStatus, &stEvent);
			kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
		}
		else
		{
			kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MBUTTONUP,
					iMouseX, iMouseY, bButtonStatus, &stEvent);
			kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
		}
	}
	else
	{
		kSetMouseEvent(qwWindowIDUnderMouse, EVENT_MOUSE_MOVE,
				iMouseX, iMouseY, bButtonStatus, &stEvent);
		kSendEventToWindow(qwWindowIDUnderMouse, &stEvent);
	}

	if (pstWindowManager->bWindowMoveMode == TRUE)
	{
		if (kGetWindowArea(pstWindowManager->qwMovingWindowID, &stWindowArea)
				== TRUE)
		{
			kMoveWindow(pstWindowManager->qwMovingWindowID,
					stWindowArea.iX1 + iMouseX - iPreviousMouseX,
					stWindowArea.iY1 + iMouseY - iPreviousMouseY);
		}
		else
		{
			pstWindowManager->bWindowMoveMode = FALSE;
			pstWindowManager->qwMovingWindowID = WINDOW_INVALIDID;
		}
	}

	pstWindowManager->bPreviousButtonStatus = bButtonStatus;
	return TRUE;
}

BOOL kProcessKeyData(void)
{
	KEYDATA stKeyData;
	EVENT stEvent;
	QWORD qwActiveWindowID;

	if (kGetKeyFromKeyQueue(&stKeyData) == FALSE)
		return FALSE;

	qwActiveWindowID = kGetTopWindowID();
	kSetKeyEvent(qwActiveWindowID, &stKeyData, &stEvent);
	return kSendEventToWindow(qwActiveWindowID, &stEvent);
}

BOOL kProcessEventQueueData(void)
{
	EVENT vstEvent[WINDOWMANAGER_DATAACCUMULATECOUNT];
	int iEventCount;
	WINDOWEVENT* pstWindowEvent;
	WINDOWEVENT* pstNextWindowEvent;
	QWORD qwWindowID;
	RECT stArea;
	RECT stOverlappedArea;
	int i, j;

	for (i = 0; i < WINDOWMANAGER_DATAACCUMULATECOUNT; i++)
	{
		if (kReceiveEventFromWindowManagerQueue(&(vstEvent[i])) == FALSE)
		{
			if (i == 0)
				return FALSE;
			else
				break;
		}

		pstWindowEvent = &(vstEvent[i].stWindowEvent);
		if (vstEvent[i].qwType == EVENT_WINDOWMANAGER_UPDATESCREENBYID)
		{
			if (kGetWindowArea(pstWindowEvent->qwWindowID, &stArea) == FALSE)
				kSetRectangleData(0, 0, 0, 0, &(pstWindowEvent->stArea));
			else
				kSetRectangleData(0, 0, kGetRectangleWidth(&stArea) - 1,
						kGetRectangleHeight(&stArea) - 1, &(pstWindowEvent->stArea));
		}
	}

	iEventCount = i;

	for (j = 0; j < iEventCount; j++)
	{
		pstWindowEvent = &(vstEvent[j].stWindowEvent);
		if ((vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYID) &&
				(vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA) &&
				(vstEvent[j].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA))
			continue;

		for (i = j + 1; i < iEventCount; i++)
		{
			pstNextWindowEvent = &(vstEvent[i].stWindowEvent);

			// except if event is not update screen event or does not match id
			if (((vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYID) &&
					(vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA) &&
					(vstEvent[i].qwType != EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA)) ||
					(pstWindowEvent->qwWindowID != pstNextWindowEvent->qwWindowID))
				continue;

			// except if area is not overlapped
			if (kGetOverlappedRectangle(&(pstWindowEvent->stArea),
						&(pstNextWindowEvent->stArea), &stOverlappedArea) == FALSE)
				continue;

			// integrate events
			if (kMemCmp(&(pstWindowEvent->stArea), &stOverlappedArea,
						sizeof(RECT)) == 0)
			{
				// next event involve current event
				kMemCpy(&(pstWindowEvent->stArea), &(pstNextWindowEvent->stArea),
						sizeof(RECT));
				vstEvent[i].qwType = EVENT_UNKNOWN;
			}
			else if (kMemCmp(&(pstNextWindowEvent->stArea), &stOverlappedArea,
						sizeof(RECT)) == 0)
			{
				// current event involve next event
				vstEvent[i].qwType = EVENT_UNKNOWN;
			}
		}
	}

	for (i = 0; i < iEventCount; i++)
	{
		pstWindowEvent = &(vstEvent[i].stWindowEvent);

		switch (vstEvent[i].qwType)
		{
			case EVENT_WINDOWMANAGER_UPDATESCREENBYID:
			case EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA:
				// update window
				if (kConvertRectClientToScreen(pstWindowEvent->qwWindowID,
							&(pstWindowEvent->stArea), &stArea) == TRUE)
					kRedrawWindowByArea(&stArea, pstWindowEvent->qwWindowID);
				break;

			case EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA:
				// update screen
				kRedrawWindowByArea(&(pstWindowEvent->stArea), WINDOW_INVALIDID);
				break;

			default:
				break;
		}
	}
	return TRUE;
}
