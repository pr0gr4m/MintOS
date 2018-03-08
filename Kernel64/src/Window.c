#include "Window.h"
#include "Utility.h"
#include "Console.h"
#include "VBE.h"
#include "Font.h"
#include "Task.h"
#include "DynamicMemory.h"
#include "MultiProcessor.h"

static WINDOWPOOLMANAGER gs_stWindowPoolManager;
static WINDOWMANAGER gs_stWindowManager;

// window pool functions
static void kInitializeWindowPool(void)
{
	int i;
	void* pvWindowPoolAddress;

	kMemSet(&gs_stWindowPoolManager, 0, sizeof(gs_stWindowPoolManager));

	pvWindowPoolAddress = (void*)kAllocateMemory(sizeof(WINDOW) * WINDOW_MAXCOUNT);
	if (pvWindowPoolAddress == NULL)
	{
		kPrintf("Window Pool Allocate Fail\n");
		while (1);
	}

	gs_stWindowPoolManager.pstStartAddress = (WINDOW*)pvWindowPoolAddress;
	kMemSet(pvWindowPoolAddress, 0, sizeof(WINDOW) * WINDOW_MAXCOUNT);

	for (i = 0; i < WINDOW_MAXCOUNT; i++)
		gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID = i;

	gs_stWindowPoolManager.iMaxCount = WINDOW_MAXCOUNT;
	gs_stWindowPoolManager.iAllocatedCount = 1;

	kInitializeMutex(&(gs_stWindowPoolManager.stLock));
}

static WINDOW* kAllocateWindow(void)
{
	WINDOW* pstEmptyWindow;
	int i;

	kLock(&(gs_stWindowPoolManager.stLock));
	if (gs_stWindowPoolManager.iUseCount == gs_stWindowPoolManager.iMaxCount)
	{
		kUnlock(&(gs_stWindowPoolManager.stLock));
		return NULL;
	}

	for (i = 0; i < gs_stWindowPoolManager.iMaxCount; i++)
	{
		if ((gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0)
		{
			pstEmptyWindow = &(gs_stWindowPoolManager.pstStartAddress[i]);
			break;
		}
	}

	pstEmptyWindow->stLink.qwID =
		((QWORD)gs_stWindowPoolManager.iAllocatedCount << 32) | i;

	gs_stWindowPoolManager.iUseCount++;
	gs_stWindowPoolManager.iAllocatedCount++;
	if (gs_stWindowPoolManager.iAllocatedCount == 0)
		gs_stWindowPoolManager.iAllocatedCount = 1;

	kUnlock(&(gs_stWindowPoolManager.stLock));
	kInitializeMutex(&(pstEmptyWindow->stLock));
	return pstEmptyWindow;
}

static void kFreeWindow(QWORD qwID)
{
	int i;
	i = GETWINDOWOFFSET(qwID);
	kLock(&(gs_stWindowPoolManager.stLock));
	kMemSet(&(gs_stWindowPoolManager.pstStartAddress[i]), 0, sizeof(WINDOW));
	gs_stWindowPoolManager.pstStartAddress[i].stLink.qwID = i;

	gs_stWindowPoolManager.iUseCount--;
	kUnlock(&(gs_stWindowPoolManager.stLock));
}

// window and window manager functions
void kInitializeGUISystem(void)
{
	VBEMODEINFOBLOCK* pstModeInfo;
	QWORD qwBackgroundWindowID;
	EVENT* pstEventBuffer;

	kInitializeWindowPool();
	pstModeInfo = kGetVBEModeInfoBlock();
	gs_stWindowManager.pstVideoMemory = (COLOR*)
		((QWORD)pstModeInfo->dwPhysicalBasePointer & 0xFFFFFFFF);

	gs_stWindowManager.iMouseX = pstModeInfo->wXResolution / 2;
	gs_stWindowManager.iMouseY = pstModeInfo->wYResolution / 2;

	gs_stWindowManager.stScreenArea.iX1 = 0;
	gs_stWindowManager.stScreenArea.iY1 = 0;
	gs_stWindowManager.stScreenArea.iX2 = pstModeInfo->wXResolution - 1;
	gs_stWindowManager.stScreenArea.iY2 = pstModeInfo->wYResolution - 1;

	kInitializeMutex(&(gs_stWindowManager.stLock));
	kInitializeList(&(gs_stWindowManager.stWindowList));

	// Create Event Pool
	pstEventBuffer = (EVENT*)kAllocateMemory(sizeof(EVENT) *
			EVENTQUEUE_WINDOWMANAGERMAXCOUNT);
	if (pstEventBuffer == NULL)
	{
		kPrintf("Window Manager Event Queue Allocate Fail\n");
		while (1);
	}

	// issue
	gs_stWindowManager.pstEventBuffer = pstEventBuffer;

	kInitializeQueue(&(gs_stWindowManager.stEventQueue), pstEventBuffer,
			EVENTQUEUE_WINDOWMANAGERMAXCOUNT, sizeof(EVENT));

	gs_stWindowManager.bPreviousButtonStatus = 0;
	gs_stWindowManager.bWindowMoveMode = FALSE;
	gs_stWindowManager.qwMovingWindowID = WINDOW_INVALIDID;

	// Create Background Window
	qwBackgroundWindowID = kCreateWindow(0, 0, pstModeInfo->wXResolution,
			pstModeInfo->wYResolution, 0, WINDOW_BACKGROUNDWINDOWTITLE);
	gs_stWindowManager.qwBackgroundWindowID = qwBackgroundWindowID;
	kDrawRect(qwBackgroundWindowID, 0, 0, pstModeInfo->wXResolution - 1,
			pstModeInfo->wYResolution - 1, WINDOW_COLOR_SYSTEMBACKGROUND, TRUE);
	kShowWindow(qwBackgroundWindowID, TRUE);
}

WINDOWMANAGER* kGetWindowManager(void)
{
	return &gs_stWindowManager;
}

QWORD kGetBackgroundWindowID(void)
{
	return gs_stWindowManager.qwBackgroundWindowID;
}

void kGetScreenArea(RECT* pstScreenArea)
{
	kMemCpy(pstScreenArea, &(gs_stWindowManager.stScreenArea), sizeof(RECT));
}

QWORD kCreateWindow(int iX, int iY, int iWidth, int iHeight, DWORD dwFlags,
		const char* pcTitle)
{
	WINDOW* pstWindow;
	TCB* pstTask;
	QWORD qwActiveWindowID;
	EVENT stEvent;

	if ((iWidth <= 0) || (iHeight <= 0))
		return WINDOW_INVALIDID;

	if ((pstWindow = kAllocateWindow()) == NULL)
		return WINDOW_INVALIDID;

	pstWindow->pstWindowBuffer = (COLOR*)kAllocateMemory(iWidth * iHeight *
			sizeof(COLOR));
	if (pstWindow->pstWindowBuffer == NULL)
	{
		kFreeWindow(pstWindow->stLink.qwID);
		return WINDOW_INVALIDID;
	}

	pstWindow->pstEventBuffer = (EVENT*)kAllocateMemory(
			EVENTQUEUE_WINDOWMAXCOUNT * sizeof(EVENT));
	if (pstWindow->pstEventBuffer == NULL)
	{
		kFreeMemory(pstWindow->pstWindowBuffer);
		kFreeWindow(pstWindow->stLink.qwID);
		return WINDOW_INVALIDID;
	}

	// set window
	pstWindow->stArea.iX1 = iX;
	pstWindow->stArea.iY1 = iY;
	pstWindow->stArea.iX2 = iX + iWidth - 1;
	pstWindow->stArea.iY2 = iY + iHeight - 1;

	kMemCpy(pstWindow->vcWindowTitle, pcTitle, WINDOW_TITLEMAXLENGTH);
	pstWindow->vcWindowTitle[WINDOW_TITLEMAXLENGTH] = '\0';

	// init queue
	kInitializeQueue(&(pstWindow->stEventQueue), pstWindow->pstEventBuffer,
			EVENTQUEUE_WINDOWMAXCOUNT, sizeof(EVENT));

	pstTask = kGetRunningTask(kGetAPICID());
	pstWindow->qwTaskID = pstTask->stLink.qwID;

	pstWindow->dwFlags = dwFlags;

	kDrawWindowBackground(pstWindow->stLink.qwID);

	if (dwFlags & WINDOW_FLAGS_DRAWFRAME)
		kDrawWindowFrame(pstWindow->stLink.qwID);

	if (dwFlags & WINDOW_FLAGS_DRAWTITLE)
		kDrawWindowTitle(pstWindow->stLink.qwID, pcTitle, TRUE);

	kLock(&(gs_stWindowManager.stLock));

	qwActiveWindowID = kGetTopWindowID();

	kAddListToTail(&gs_stWindowManager.stWindowList, pstWindow);

	kUnlock(&(gs_stWindowManager.stLock));

	// send window event
	kUpdateScreenByID(pstWindow->stLink.qwID);
	kSetWindowEvent(pstWindow->stLink.qwID, EVENT_WINDOW_SELECT, &stEvent);
	kSendEventToWindow(pstWindow->stLink.qwID, &stEvent);

	// deselect previous top window
	if (qwActiveWindowID != gs_stWindowManager.qwBackgroundWindowID)
	{
		kUpdateWindowTitle(qwActiveWindowID, FALSE);
		kSetWindowEvent(qwActiveWindowID, EVENT_WINDOW_DESELECT, &stEvent);
		kSendEventToWindow(qwActiveWindowID, &stEvent);
	}

	return pstWindow->stLink.qwID;
}

BOOL kDeleteWindow(QWORD qwWindowID)
{
	WINDOW* pstWindow;
	RECT stArea;
	QWORD qwActiveWindowID;
	BOOL bActiveWindow;
	EVENT stEvent;

	kLock(&(gs_stWindowManager.stLock));

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
	{
		kUnlock(&(gs_stWindowManager.stLock));
		return FALSE;
	}

	// store area before delete
	kMemCpy(&stArea, &(pstWindow->stArea), sizeof(RECT));

	qwActiveWindowID = kGetTopWindowID();

	if (qwActiveWindowID == qwWindowID)
		bActiveWindow = TRUE;
	else
		bActiveWindow = FALSE;

	if (kRemoveList(&(gs_stWindowManager.stWindowList), qwWindowID) == NULL)
	{
		kUnlock(&(pstWindow->stLock));
		kUnlock(&(gs_stWindowManager.stLock));
		return FALSE;
	}

	kFreeMemory(pstWindow->pstWindowBuffer);
	pstWindow->pstWindowBuffer = NULL;

	kFreeMemory(pstWindow->pstEventBuffer);
	pstWindow->pstEventBuffer = NULL;

	kUnlock(&(pstWindow->stLock));

	kFreeWindow(qwWindowID);
	kUnlock(&(gs_stWindowManager.stLock));

	// update previous screen
	kUpdateScreenByScreenArea(&stArea);

	if (bActiveWindow == TRUE)
	{
		qwActiveWindowID = kGetTopWindowID();
		if (qwActiveWindowID != WINDOW_INVALIDID)
		{
			kUpdateWindowTitle(qwActiveWindowID, TRUE);
			kSetWindowEvent(qwActiveWindowID, EVENT_WINDOW_SELECT, &stEvent);
			kSendEventToWindow(qwActiveWindowID, &stEvent);
		}
	}

	return TRUE;
}

BOOL kDeleteAllWindowInTaskID(QWORD qwTaskID)
{
	WINDOW* pstWindow;
	WINDOW* pstNextWindow;

	kLock(&(gs_stWindowManager.stLock));

	pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
	while (pstWindow != NULL)
	{
		pstNextWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList),
				pstWindow);

		if ((pstWindow->stLink.qwID != gs_stWindowManager.qwBackgroundWindowID)
				&& (pstWindow->qwTaskID == qwTaskID))
			kDeleteWindow(pstWindow->stLink.qwID);

		pstWindow = pstNextWindow;
	}

	kUnlock(&(gs_stWindowManager.stLock));
}

WINDOW* kGetWindow(QWORD qwWindowID)
{
	WINDOW* pstWindow;

	if (GETWINDOWOFFSET(qwWindowID) >= WINDOW_MAXCOUNT)
		return NULL;

	pstWindow = &gs_stWindowPoolManager.pstStartAddress[GETWINDOWOFFSET(qwWindowID)];
	if (pstWindow->stLink.qwID == qwWindowID)
		return pstWindow;
	return NULL;
}

WINDOW* kGetWindowWithWindowLock(QWORD qwWindowID)
{
	WINDOW* pstWindow;
	BOOL bResult;

	if ((pstWindow = kGetWindow(qwWindowID)) == NULL)
		return NULL;
	
	kLock(&(pstWindow->stLock));
	if (kGetWindow(qwWindowID) != pstWindow)
	{
		kUnlock(&(pstWindow->stLock));
		return NULL;
	}

	return pstWindow;
}

BOOL kShowWindow(QWORD qwWindowID, BOOL bShow)
{
	WINDOW* pstWindow;
	RECT stWindowArea;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	if (bShow == TRUE)
		pstWindow->dwFlags |= WINDOW_FLAGS_SHOW;
	else
		pstWindow->dwFlags &= ~WINDOW_FLAGS_SHOW;

	kUnlock(&(pstWindow->stLock));

	if (bShow == TRUE)
	{
		kUpdateScreenByID(qwWindowID);
	}
	else
	{
		kGetWindowArea(qwWindowID, &stWindowArea);
		kUpdateScreenByScreenArea(&stWindowArea);
	}

	return TRUE;
}

BOOL kRedrawWindowByArea(const RECT* pstArea)
{
	WINDOW* pstWindow;
	WINDOW* pstTargetWindow = NULL;
	RECT stOverlappedArea;
	RECT stCursorArea;

	if (kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), pstArea,
				&stOverlappedArea) == FALSE)
		return FALSE;

	kLock(&(gs_stWindowManager.stLock));

	pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
	while (pstWindow != NULL)
	{
		if ((pstWindow->dwFlags & WINDOW_FLAGS_SHOW) &&
				(kIsRectangleOverlapped(&(pstWindow->stArea), &stOverlappedArea)
				 == TRUE))
		{
			kLock(&(pstWindow->stLock));
			kCopyWindowBufferToFrameBuffer(pstWindow, &stOverlappedArea);
			kUnlock(&(pstWindow->stLock));
		}

		pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList),
				pstWindow);
	}
	
	kUnlock(&(gs_stWindowManager.stLock));

	// Mouse
	kSetRectangleData(gs_stWindowManager.iMouseX, gs_stWindowManager.iMouseY,
			gs_stWindowManager.iMouseX + MOUSE_CURSOR_WIDTH,
			gs_stWindowManager.iMouseY + MOUSE_CURSOR_HEIGHT, &stCursorArea);

	if (kIsRectangleOverlapped(&stOverlappedArea, &stCursorArea) == TRUE)
		kDrawCursor(gs_stWindowManager.iMouseX, gs_stWindowManager.iMouseY);
}

static void kCopyWindowBufferToFrameBuffer(const WINDOW* pstWindow,
		const RECT* pstCopyArea)
{
	RECT stTempArea;
	RECT stOverlappedArea;
	int iOverlappedWidth;
	int iOverlappedHeight;
	int iScreenWidth;
	int iWindowWidth;
	int i;
	COLOR* pstCurrentVideoMemoryAddress;
	COLOR* pstCurrentWindowBufferAddress;

	if (kGetOverlappedRectangle(&(gs_stWindowManager.stScreenArea), pstCopyArea,
				&stTempArea) == FALSE)
		return;

	if (kGetOverlappedRectangle(&stTempArea, &(pstWindow->stArea),
				&stOverlappedArea) == FALSE)
		return;

	iScreenWidth = kGetRectangleWidth(&(gs_stWindowManager.stScreenArea));
	iWindowWidth = kGetRectangleWidth(&(pstWindow->stArea));
	iOverlappedWidth = kGetRectangleWidth(&stOverlappedArea);
	iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);

	pstCurrentVideoMemoryAddress = gs_stWindowManager.pstVideoMemory +
		stOverlappedArea.iY1 * iScreenWidth + stOverlappedArea.iX1;

	// calculate window buffer inner address
	pstCurrentWindowBufferAddress = pstWindow->pstWindowBuffer +
		(stOverlappedArea.iY1 - pstWindow->stArea.iY1) * iWindowWidth +
		(stOverlappedArea.iX1 - pstWindow->stArea.iX1);

	for (i = 0; i < iOverlappedHeight; i++)
	{
		kMemCpy(pstCurrentVideoMemoryAddress, pstCurrentWindowBufferAddress,
				iOverlappedWidth * sizeof(COLOR));

		pstCurrentVideoMemoryAddress += iScreenWidth;
		pstCurrentWindowBufferAddress += iWindowWidth;
	}
}

QWORD kFindWindowByPoint(int iX, int iY)
{
	QWORD qwWindowID;
	WINDOW* pstWindow;

	qwWindowID = gs_stWindowManager.qwBackgroundWindowID;

	kLock(&(gs_stWindowManager.stLock));

	pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
	do
	{
		pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList), pstWindow);
		if ((pstWindow != NULL) &&
				(pstWindow->dwFlags & WINDOW_FLAGS_SHOW) &&
				(kIsInRectangle(&(pstWindow->stArea), iX, iY) == TRUE))
		{
			qwWindowID = pstWindow->stLink.qwID;
		}
	} while (pstWindow != NULL);

	kUnlock(&(gs_stWindowManager.stLock));
	return qwWindowID;
}

QWORD kFindWindowByTitle(const char* pcTitle)
{
	QWORD qwWindowID;
	WINDOW* pstWindow;
	int iTitleLength;

	qwWindowID = WINDOW_INVALIDID;
	iTitleLength = kStrLen(pcTitle);

	kLock(&(gs_stWindowManager.stLock));

	pstWindow = kGetHeaderFromList(&(gs_stWindowManager.stWindowList));
	while (pstWindow != NULL)
	{
		if ((kStrLen(pstWindow->vcWindowTitle) == iTitleLength) &&
				(kMemCmp(pstWindow->vcWindowTitle, pcTitle, iTitleLength) == 0))
		{
			qwWindowID = pstWindow->stLink.qwID;
			break;
		}

		pstWindow = kGetNextFromList(&(gs_stWindowManager.stWindowList), pstWindow);
	}

	kUnlock(&(gs_stWindowManager.stLock));
	return qwWindowID;
}

BOOL kIsWindowExist(QWORD qwWindowID)
{
	if (kGetWindow(qwWindowID) == NULL)
		return FALSE;
	return TRUE;
}

QWORD kGetTopWindowID(void)
{
	WINDOW* pstActiveWindow;
	QWORD qwActiveWindowID;

	kLock(&(gs_stWindowManager.stLock));

	pstActiveWindow = (WINDOW*)kGetTailFromList(&(gs_stWindowManager.stWindowList));
	if (pstActiveWindow != NULL)
		qwActiveWindowID = pstActiveWindow->stLink.qwID;
	else
		qwActiveWindowID = WINDOW_INVALIDID;

	kUnlock(&(gs_stWindowManager.stLock));
	return qwActiveWindowID;
}

BOOL kMoveWindowToTop(QWORD qwWindowID)
{
	WINDOW* pstWindow;
	RECT stArea;
	DWORD dwFlags;
	QWORD qwTopWindowID;
	EVENT stEvent;
	
	qwTopWindowID = kGetTopWindowID();
	if (qwTopWindowID == qwWindowID)
		return TRUE;

	kLock(&(gs_stWindowManager.stLock));

	pstWindow = kRemoveList(&(gs_stWindowManager.stWindowList), qwWindowID);
	if (pstWindow != NULL)
	{
		kAddListToTail(&(gs_stWindowManager.stWindowList), pstWindow);
		kConvertRectScreenToClient(qwWindowID, &(pstWindow->stArea), &stArea);
		dwFlags = pstWindow->dwFlags;
	}
	kUnlock(&(gs_stWindowManager.stLock));

	if (pstWindow != NULL)
	{
		// update previous top
		kSetWindowEvent(qwTopWindowID, EVENT_WINDOW_DESELECT, &stEvent);
		kSendEventToWindow(qwTopWindowID, &stEvent);
		kUpdateWindowTitle(qwTopWindowID, FALSE);

		// update new top
		kSetWindowEvent(qwWindowID, EVENT_WINDOW_SELECT, &stEvent);
		kSendEventToWindow(qwWindowID, &stEvent);

		if (dwFlags & WINDOW_FLAGS_DRAWTITLE)
		{
			kUpdateWindowTitle(qwWindowID, TRUE);
			stArea.iY1 += WINDOW_TITLEBAR_HEIGHT;
			kUpdateScreenByWindowArea(qwWindowID, &stArea);
		}
		else
		{
			kUpdateScreenByID(qwWindowID);
		}

		return TRUE;
	}
	return FALSE;
}

BOOL kIsInTitleBar(QWORD qwWindowID, int iX, int iY)
{
	WINDOW* pstWindow;

	pstWindow = kGetWindow(qwWindowID);

	if ((pstWindow == NULL) ||
			((pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE) == 0))
		return FALSE;

	if ((pstWindow->stArea.iX1 <= iX) && (iX <= pstWindow->stArea.iX2) &&
			(pstWindow->stArea.iY1 <= iY) &&
			(iY <= pstWindow->stArea.iY1 + WINDOW_TITLEBAR_HEIGHT))
		return TRUE;

	return FALSE;
}

BOOL kIsInCloseButton(QWORD qwWindowID, int iX, int iY)
{
	WINDOW* pstWindow;

	pstWindow = kGetWindow(qwWindowID);

	if ((pstWindow == NULL) ||
			((pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE) == 0))
		return FALSE;

	if (((pstWindow->stArea.iX2 - WINDOW_XBUTTON_SIZE - 1) <= iX) && 
			(iX <= pstWindow->stArea.iX2 - 1) &&
			(pstWindow->stArea.iY1 + 1 <= iY) &&
			(iY <= pstWindow->stArea.iY1 + 1 + WINDOW_XBUTTON_SIZE))
		return TRUE;

	return FALSE;
}

BOOL kMoveWindow(QWORD qwWindowID, int iX, int iY)
{
	WINDOW* pstWindow;
	RECT stPreviousArea;
	int iWidth;
	int iHeight;
	EVENT stEvent;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	kMemCpy(&stPreviousArea, &(pstWindow->stArea), sizeof(RECT));

	iWidth = kGetRectangleWidth(&stPreviousArea);
	iHeight = kGetRectangleHeight(&stPreviousArea);
	kSetRectangleData(iX, iY, iX + iWidth - 1, iY + iHeight - 1,
			&(pstWindow->stArea));
	
	kUnlock(&(pstWindow->stLock));

	kUpdateScreenByScreenArea(&stPreviousArea);
	kUpdateScreenByID(qwWindowID);

	kSetWindowEvent(qwWindowID, EVENT_WINDOW_MOVE, &stEvent);
	kSendEventToWindow(qwWindowID, &stEvent);

	return TRUE;
}

static BOOL kUpdateWindowTitle(QWORD qwWindowID, BOOL bSelectedTitle)
{
	WINDOW* pstWindow;
	RECT stTitleBarArea;

	pstWindow = kGetWindow(qwWindowID);

	if ((pstWindow != NULL) &&
			(pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE))
	{
		kDrawWindowTitle(pstWindow->stLink.qwID, pstWindow->vcWindowTitle,
				bSelectedTitle);
		stTitleBarArea.iX1 = 0;
		stTitleBarArea.iY1 = 0;
		stTitleBarArea.iX2 = kGetRectangleWidth(&(pstWindow->stArea)) - 1;
		stTitleBarArea.iY2 = WINDOW_TITLEBAR_HEIGHT;

		kUpdateScreenByWindowArea(qwWindowID, &stTitleBarArea);
		
		return TRUE;
	}

	return FALSE;
}

// convert point functions

BOOL kGetWindowArea(QWORD qwWindowID, RECT* pstArea)
{
	WINDOW* pstWindow;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	kMemCpy(pstArea, &(pstWindow->stArea), sizeof(RECT));
	kUnlock(&(pstWindow->stLock));
	return TRUE;
}

// screen -> window
BOOL kConvertPointScreenToClient(QWORD qwWindowID, const POINT* pstXY,
		POINT* pstXYInWindow)
{
	RECT stArea;

	if (kGetWindowArea(qwWindowID, &stArea) == FALSE)
		return FALSE;

	pstXYInWindow->iX = pstXY->iX - stArea.iX1;
	pstXYInWindow->iY = pstXY->iY - stArea.iY1;
	return TRUE;
}

// window -> screen
BOOL kConvertPointClientToScreen(QWORD qwWindowID, const POINT* pstXY,
		POINT* pstXYInScreen)
{
	RECT stArea;

	if (kGetWindowArea(qwWindowID, &stArea) == FALSE)
		return FALSE;

	pstXYInScreen->iX = pstXY->iX + stArea.iX1;
	pstXYInScreen->iY = pstXY->iY + stArea.iY1;
	return TRUE;
}


// screen -> window
BOOL kConvertRectScreenToClient(QWORD qwWindowID, const RECT* pstArea,
		RECT* pstAreaInWindow)
{
	RECT stWindowArea;

	if (kGetWindowArea(qwWindowID, &stWindowArea) == FALSE)
		return FALSE;

	pstAreaInWindow->iX1 = pstArea->iX1 - stWindowArea.iX1;
	pstAreaInWindow->iY1 = pstArea->iY1 - stWindowArea.iY1;
	pstAreaInWindow->iX2 = pstArea->iX2 - stWindowArea.iX2;
	pstAreaInWindow->iY2 = pstArea->iY2 - stWindowArea.iY2;
	return TRUE;
}


// window -> screen
BOOL kConvertRectClientToScreen(QWORD qwWindowID, const RECT* pstArea,
		RECT* pstAreaInScreen)
{
	RECT stWindowArea;

	if (kGetWindowArea(qwWindowID, &stWindowArea) == FALSE)
		return FALSE;

	pstAreaInScreen->iX1 = pstArea->iX1 + stWindowArea.iX1;
	pstAreaInScreen->iY1 = pstArea->iY1 + stWindowArea.iY1;
	pstAreaInScreen->iX2 = pstArea->iX2 + stWindowArea.iX2;
	pstAreaInScreen->iY2 = pstArea->iY2 + stWindowArea.iY2;
	return TRUE;
}


// update screen functions
BOOL kUpdateScreenByID(QWORD qwWindowID)
{
	EVENT stEvent;
	WINDOW* pstWindow;

	pstWindow = kGetWindow(qwWindowID);
	if ((pstWindow == NULL) &&
			((pstWindow->dwFlags & WINDOW_FLAGS_SHOW) == 0))
		return FALSE;

	stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYID;
	stEvent.stWindowEvent.qwWindowID = qwWindowID;
	return kSendEventToWindowManager(&stEvent);
}

BOOL kUpdateScreenByWindowArea(QWORD qwWindowID, const RECT* pstArea)
{
	EVENT stEvent;
	WINDOW* pstWindow;

	pstWindow = kGetWindow(qwWindowID);
	if ((pstWindow == NULL) &&
			((pstWindow->dwFlags & WINDOW_FLAGS_SHOW) == 0))
		return FALSE;

	stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA;
	stEvent.stWindowEvent.qwWindowID = qwWindowID;
	kMemCpy(&(stEvent.stWindowEvent.stArea), pstArea, sizeof(RECT));

	return kSendEventToWindowManager(&stEvent);
}

BOOL kUpdateScreenByScreenArea(const RECT* pstArea)
{
	EVENT stEvent;

	stEvent.qwType = EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA;
	stEvent.stWindowEvent.qwWindowID = WINDOW_INVALIDID;
	kMemCpy(&(stEvent.stWindowEvent.stArea), pstArea, sizeof(RECT));

	return kSendEventToWindowManager(&stEvent);
}

// event functions

BOOL kSendEventToWindow(QWORD qwWindowID, const EVENT* pstEvent)
{
	WINDOW* pstWindow;
	BOOL bResult;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	bResult = kPutQueue(&(pstWindow->stEventQueue), pstEvent);
	kUnlock(&(pstWindow->stLock));

	return bResult;
}

BOOL kReceiveEventFromWindowQueue(QWORD qwWindowID, EVENT* pstEvent)
{
	WINDOW* pstWindow;
	BOOL bResult;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	bResult = kGetQueue(&(pstWindow->stEventQueue), pstEvent);
	kUnlock(&(pstWindow->stLock));

	return bResult;
}

BOOL kSendEventToWindowManager(const EVENT* pstEvent)
{
	BOOL bResult = FALSE;

	if (kIsQueueFull(&(gs_stWindowManager.stEventQueue)) == FALSE)
	{
		kLock(&(gs_stWindowManager.stLock));
		bResult = kPutQueue(&(gs_stWindowManager.stEventQueue), pstEvent);
		kUnlock(&(gs_stWindowManager.stLock));
	}
	return bResult;
}

BOOL kReceiveEventFromWindowManagerQueue(EVENT* pstEvent)
{
	BOOL bResult = FALSE;

	if (kIsQueueEmpty(&(gs_stWindowManager.stEventQueue)) == FALSE)
	{
		kLock(&(gs_stWindowManager.stLock));
		bResult = kGetQueue(&(gs_stWindowManager.stEventQueue), pstEvent);
		kUnlock(&(gs_stWindowManager.stLock));
	}
	return bResult;
}

BOOL kSetMouseEvent(QWORD qwWindowID, QWORD qwEventType, int iMouseX, int iMouseY,
		BYTE bButtonStatus, EVENT* pstEvent)
{
	POINT stMouseXYInWindow;
	POINT stMouseXY;

	switch (qwEventType)
	{
		case EVENT_MOUSE_MOVE:
		case EVENT_MOUSE_LBUTTONDOWN:
		case EVENT_MOUSE_LBUTTONUP:
		case EVENT_MOUSE_RBUTTONDOWN:
		case EVENT_MOUSE_RBUTTONUP:
		case EVENT_MOUSE_MBUTTONDOWN:
		case EVENT_MOUSE_MBUTTONUP:

			stMouseXY.iX = iMouseX;
			stMouseXY.iY = iMouseY;

			if (kConvertPointScreenToClient(qwWindowID, &stMouseXY, &stMouseXYInWindow)
					== FALSE)
				return FALSE;

			pstEvent->qwType = qwEventType;
			pstEvent->stMouseEvent.qwWindowID = qwWindowID;
			pstEvent->stMouseEvent.bButtonStatus = bButtonStatus;
			kMemCpy(&(pstEvent->stMouseEvent.stPoint), &stMouseXYInWindow,
					sizeof(POINT));
			break;

		default:
			return FALSE;
	}
	return TRUE;
}

BOOL kSetWindowEvent(QWORD qwWindowID, QWORD qwEventType, EVENT* pstEvent)
{
	RECT stArea;

	switch (qwEventType)
	{
		case EVENT_WINDOW_SELECT:
		case EVENT_WINDOW_DESELECT:
		case EVENT_WINDOW_MOVE:
		case EVENT_WINDOW_RESIZE:
		case EVENT_WINDOW_CLOSE:

			pstEvent->qwType = qwEventType;
			pstEvent->stWindowEvent.qwWindowID = qwWindowID;
			if (kGetWindowArea(qwWindowID, &stArea) == FALSE)
				return FALSE;

			kMemCpy(&(pstEvent->stWindowEvent.stArea), &stArea, sizeof(RECT));
			break;

		default:
			return FALSE;
	}
	return TRUE;
}

void kSetKeyEvent(QWORD qwWindow, const KEYDATA* pstKeyData, EVENT* pstEvent)
{
	if (pstKeyData->bFlags & KEY_FLAGS_DOWN)
		pstEvent->qwType = EVENT_KEY_DOWN;
	else
		pstEvent->qwType = EVENT_KEY_UP;

	pstEvent->stKeyEvent.bASCIICode = pstKeyData->bASCIICode;
	pstEvent->stKeyEvent.bScanCode = pstKeyData->bScanCode;
	pstEvent->stKeyEvent.bFlags = pstKeyData->bFlags;
}


// drawing functions
BOOL kDrawWindowFrame(QWORD qwWindowID)
{
	WINDOW* pstWindow;
	RECT stArea;
	int iWidth;
	int iHeight;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	iWidth = kGetRectangleWidth(&(pstWindow->stArea));
	iHeight = kGetRectangleHeight(&(pstWindow->stArea));

	kSetRectangleData(0, 0, iWidth - 1, iHeight - 1, &stArea);

	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
			0, 0, iWidth - 1, iHeight - 1, WINDOW_COLOR_FRAME, FALSE);
	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
			1, 1, iWidth - 2, iHeight - 2, WINDOW_COLOR_FRAME, FALSE);

	kUnlock(&(pstWindow->stLock));
	return TRUE;
}

BOOL kDrawWindowBackground(QWORD qwWindowID)
{
	WINDOW* pstWindow;
	RECT stArea;
	int iWidth;
	int iHeight;
	int iX, iY;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	iWidth = kGetRectangleWidth(&(pstWindow->stArea));
	iHeight = kGetRectangleHeight(&(pstWindow->stArea));

	kSetRectangleData(0, 0, iWidth - 1, iHeight - 1, &stArea);

	if (pstWindow->dwFlags & WINDOW_FLAGS_DRAWTITLE)
		iY = WINDOW_TITLEBAR_HEIGHT;
	else
		iY = 0;

	if (pstWindow->dwFlags & WINDOW_FLAGS_DRAWFRAME)
		iX = 2;
	else
		iX = 0;

	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
			iX, iY, iWidth - 1 - iX, iHeight - 1 - iX, 
			WINDOW_COLOR_BACKGROUND, TRUE);

	kUnlock(&(pstWindow->stLock));
	return TRUE;
}

BOOL kDrawWindowTitle(QWORD qwWindowID, const char* pcTitle, BOOL bSelectedTitle)
{
	WINDOW* pstWindow;
	RECT stArea;
	RECT stButtonArea;
	int iWidth;
	int iHeight;
	int iX, iY;
	COLOR stTitleBarColor;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	iWidth = kGetRectangleWidth(&(pstWindow->stArea));
	iHeight = kGetRectangleHeight(&(pstWindow->stArea));

	kSetRectangleData(0, 0, iWidth - 1, iHeight - 1, &stArea);

	// Draw Title Bar

	if (bSelectedTitle == TRUE)
		stTitleBarColor = WINDOW_COLOR_TITLEBARACTIVEBACKGROUND;
	else
		stTitleBarColor = WINDOW_COLOR_TITLEBARINACTIVEBACKGROUND;

	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
			0, 3, iWidth - 1, WINDOW_TITLEBAR_HEIGHT - 1,
			stTitleBarColor, TRUE);
	kInternalDrawText(&stArea, pstWindow->pstWindowBuffer,
			6, 3, WINDOW_COLOR_TITLEBARTEXT, stTitleBarColor,
			pcTitle, kStrLen(pcTitle));

	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			1, 1, iWidth - 1, 1, WINDOW_COLOR_TITLEBARBRIGHT1);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			1, 2, iWidth - 1, 2, WINDOW_COLOR_TITLEBARBRIGHT2);

	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			1, 2, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT1);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			2, 2, 2, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT2);

	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			2, WINDOW_TITLEBAR_HEIGHT - 2, iWidth - 2, WINDOW_TITLEBAR_HEIGHT - 2,
			WINDOW_COLOR_TITLEBARUNDERLINE);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			2, WINDOW_TITLEBAR_HEIGHT - 1, iWidth - 2, WINDOW_TITLEBAR_HEIGHT - 1,
			WINDOW_COLOR_TITLEBARUNDERLINE);

	kUnlock(&(pstWindow->stLock));

	// Draw Close Button
	stButtonArea.iX1 = iWidth - WINDOW_XBUTTON_SIZE - 1;
	stButtonArea.iY1 = 1;
	stButtonArea.iX2 = iWidth - 2;
	stButtonArea.iY2 = WINDOW_XBUTTON_SIZE - 1;
	kDrawButton(qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, "",
			WINDOW_COLOR_BACKGROUND);

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			iWidth - 2 - 18 + 4, 1 + 4, iWidth - 2 - 4,
			WINDOW_TITLEBAR_HEIGHT - 6, WINDOW_COLOR_XBUTTONLINECOLOR);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			iWidth - 2 - 18 + 5, 1 + 4, iWidth - 2 - 4,
			WINDOW_TITLEBAR_HEIGHT - 7, WINDOW_COLOR_XBUTTONLINECOLOR);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			iWidth - 2 - 18 + 4, 1 + 5, iWidth - 2 - 5,
			WINDOW_TITLEBAR_HEIGHT - 6, WINDOW_COLOR_XBUTTONLINECOLOR);

	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			iWidth - 2 - 18 + 4, 19 - 4, iWidth - 2 - 4, 1 + 4,
			WINDOW_COLOR_XBUTTONLINECOLOR);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			iWidth - 2 - 18 + 5, 19 - 4, iWidth - 2 - 4, 1 + 5,
			WINDOW_COLOR_XBUTTONLINECOLOR);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			iWidth - 2 - 18 + 4, 19 - 5, iWidth - 2 - 5, 1 + 4,
			WINDOW_COLOR_XBUTTONLINECOLOR);

	kUnlock(&(pstWindow->stLock));

	return TRUE;
}

BOOL kDrawButton(QWORD qwWindowID, RECT* pstButtonArea, COLOR stBackgroundColor,
		const char* pcText, COLOR stTextColor)
{
	WINDOW* pstWindow;
	RECT stArea;
	int iWindowWidth;
	int iWindowHeight;
	int iTextLength;
	int iTextWidth;
	int iButtonWidth;
	int iButtonHeight;
	int iTextX;
	int iTextY;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	iWindowWidth = kGetRectangleWidth(&(pstWindow->stArea));
	iWindowHeight = kGetRectangleHeight(&(pstWindow->stArea));

	kSetRectangleData(0, 0, iWindowWidth - 1, iWindowHeight - 1, &stArea);

	// background
	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX2,
			pstButtonArea->iY2, stBackgroundColor, TRUE);

	// text
	iButtonWidth = kGetRectangleWidth(pstButtonArea);
	iButtonHeight = kGetRectangleHeight(pstButtonArea);
	iTextLength = kStrLen(pcText);
	iTextWidth = iTextLength * FONT_ENGLISHWIDTH;

	iTextX = (pstButtonArea->iX1 + iButtonWidth / 2) - iTextWidth / 2;
	iTextY = (pstButtonArea->iY1 + iButtonHeight / 2) - FONT_ENGLISHHEIGHT / 2;
	kInternalDrawText(&stArea, pstWindow->pstWindowBuffer, iTextX, iTextY,
			stTextColor, stBackgroundColor, pcText, iTextLength);

	// button
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX2,
			pstButtonArea->iY1, WINDOW_COLOR_BUTTONBRIGHT);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1, pstButtonArea->iY1 + 1, pstButtonArea->iX2 - 1,
			pstButtonArea->iY1 + 1, WINDOW_COLOR_BUTTONBRIGHT);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1, pstButtonArea->iY1, pstButtonArea->iX1,
			pstButtonArea->iY2, WINDOW_COLOR_BUTTONBRIGHT);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1 + 1, pstButtonArea->iY1, pstButtonArea->iX1 + 1,
			pstButtonArea->iY2 - 1, WINDOW_COLOR_BUTTONBRIGHT);

	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1 + 1, pstButtonArea->iY2, pstButtonArea->iX2,
			pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX1 + 2, pstButtonArea->iY2 - 1, pstButtonArea->iX2,
			pstButtonArea->iY2 - 1, WINDOW_COLOR_BUTTONDARK);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX2, pstButtonArea->iY1 + 1, pstButtonArea->iX2,
			pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK);
	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer,
			pstButtonArea->iX2 - 1, pstButtonArea->iY1 + 2, pstButtonArea->iX2 - 1,
			pstButtonArea->iY2, WINDOW_COLOR_BUTTONDARK);

	kUnlock(&(pstWindow->stLock));

	return TRUE;
}


// cursor data
BYTE gs_vwMouseBuffer[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT] =
{
	1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1, 0, 0,
	0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 3, 2, 1, 1, 2, 3, 2, 2, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 2, 2, 1, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0,
	0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0,
	0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
};


static void kDrawCursor(int iX, int iY)
{
	int i, j;
	BYTE* pbCurrentPos;

	pbCurrentPos = gs_vwMouseBuffer;

	for (j = 0; j < MOUSE_CURSOR_HEIGHT; j++)
	{
		for (i = 0; i < MOUSE_CURSOR_WIDTH; i++)
		{
			switch (*pbCurrentPos)
			{
				case 0:
					break;

				case 1:
					kInternalDrawPixel(&(gs_stWindowManager.stScreenArea), 
							gs_stWindowManager.pstVideoMemory, i + iX, j + iY,
							MOUSE_CURSOR_OUTERLINE);
					break;

				case 2:
					kInternalDrawPixel(&(gs_stWindowManager.stScreenArea), 
							gs_stWindowManager.pstVideoMemory, i + iX, j + iY,
							MOUSE_CURSOR_OUTER);
					break;

				case 3:
					kInternalDrawPixel(&(gs_stWindowManager.stScreenArea), 
							gs_stWindowManager.pstVideoMemory, i + iX, j + iY,
							MOUSE_CURSOR_INNER);
					break;
			}
			pbCurrentPos++;
		}
	}
}

void kMoveCursor(int iX, int iY)
{
	RECT stPreviousArea;

	if (iX < gs_stWindowManager.stScreenArea.iX1)
		iX = gs_stWindowManager.stScreenArea.iX1;
	else if (iX > gs_stWindowManager.stScreenArea.iX2)
		iX = gs_stWindowManager.stScreenArea.iX2;

	if (iY < gs_stWindowManager.stScreenArea.iY1)
		iY = gs_stWindowManager.stScreenArea.iY1;
	else if (iY > gs_stWindowManager.stScreenArea.iY2)
		iY = gs_stWindowManager.stScreenArea.iY2;

	kLock(&(gs_stWindowManager.stLock));

	stPreviousArea.iX1 = gs_stWindowManager.iMouseX;
	stPreviousArea.iY1 = gs_stWindowManager.iMouseY;
	stPreviousArea.iX2 = gs_stWindowManager.iMouseX + MOUSE_CURSOR_WIDTH - 1;
	stPreviousArea.iY2 = gs_stWindowManager.iMouseY + MOUSE_CURSOR_HEIGHT - 1;

	gs_stWindowManager.iMouseX = iX;
	gs_stWindowManager.iMouseY = iY;

	kUnlock(&(gs_stWindowManager.stLock));
	
	kRedrawWindowByArea(&stPreviousArea);
	kDrawCursor(iX, iY);
}

void kGetCursorPosition(int* piX, int* piY)
{
	*piX = gs_stWindowManager.iMouseX;
	*piY = gs_stWindowManager.iMouseY;
}

BOOL kDrawPixel(QWORD qwWindowID, int iX, int iY, COLOR stColor)
{
	WINDOW* pstWindow;
	RECT stArea;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
			pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);

	kInternalDrawPixel(&stArea, pstWindow->pstWindowBuffer, iX, iY,
			stColor);

	kUnlock(&pstWindow->stLock);
	return TRUE;
}

BOOL kDrawLine(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor)
{
	WINDOW* pstWindow;
	RECT stArea;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
			pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);

	kInternalDrawLine(&stArea, pstWindow->pstWindowBuffer, iX1, iY1,
			iX2, iY2, stColor);

	kUnlock(&pstWindow->stLock);
	return TRUE;
}

BOOL kDrawRect(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2,
		COLOR stColor, BOOL bFill)
{
	WINDOW* pstWindow;
	RECT stArea;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
			pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);

	kInternalDrawRect(&stArea, pstWindow->pstWindowBuffer, iX1, iY1,
			iX2, iY2, stColor, bFill);

	kUnlock(&pstWindow->stLock);
	return TRUE;
}

BOOL kDrawCircle(QWORD qwWindowID, int iX, int iY, int iRadius, COLOR stColor,
		BOOL bFill)
{
	WINDOW* pstWindow;
	RECT stArea;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
			pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);

	kInternalDrawCircle(&stArea, pstWindow->pstWindowBuffer, iX, iY,
			iRadius, stColor, bFill);

	kUnlock(&pstWindow->stLock);
	return TRUE;
}

BOOL kDrawText(QWORD qwWindowID, int iX, int iY, COLOR stTextColor, 
		COLOR stBackgroundColor, const char* pcString, int iLength)
{
	WINDOW* pstWindow;
	RECT stArea;

	pstWindow = kGetWindowWithWindowLock(qwWindowID);
	if (pstWindow == NULL)
		return FALSE;

	kSetRectangleData(0, 0, pstWindow->stArea.iX2 - pstWindow->stArea.iX1,
			pstWindow->stArea.iY2 - pstWindow->stArea.iY1, &stArea);

	kInternalDrawText(&stArea, pstWindow->pstWindowBuffer, iX, iY,
			stTextColor, stBackgroundColor, pcString, iLength);

	kUnlock(&pstWindow->stLock);
	return TRUE;
}


void kGetRandomXY(int* piX, int* piY)
{
	*piX = ABS(kRandom()) % 1000;
	*piY = ABS(kRandom()) % 700;
}

COLOR kGetRandomColor(void)
{
	int iR, iG, iB;
	iR = ABS(kRandom()) % 256;
	iG = ABS(kRandom()) % 256;
	iB = ABS(kRandom()) % 256;
	return RGB(iR, iG, iB);
}

