#include "ApplicationPanelTask.h"
#include "RTC.h"
#include "Task.h"
#include "GUITask.h"
#include "Utility.h"
#include "Console.h"

APPLICATIONENTRY gs_vstApplicationTable[] =
{
	{ "Base GUI Task", kBaseGUITask },
	{ "Hello World GUI Task", kHelloWorldGUITask },
	{ "System Monitor Task", kSystemMonitorTask },
	{ "Console Shell for GUI", kGUIConsoleShellTask },
	{ "Image Viewer Task", kImageViewerTask },
};

APPLICATIONPANELDATA gs_stApplicationPanelData;

void kApplicationPanelGUITask(void)
{
	EVENT stReceivedEvent;
	BOOL bApplicationPanelEventResult;
	BOOL bApplicationListEventResult;

	if (kIsGraphicMode() == FALSE)
	{
		kPrintf("This task can run only GUI Mode.\n");
		return;
	}

	if ((kCreateApplicationPanelWindow() == FALSE) ||
			(kCreateApplicationListWindow() == FALSE))
		return;

	while (1)
	{
		bApplicationPanelEventResult = kProcessApplicationPanelWindowEvent();
		bApplicationListEventResult = kProcessApplicationListWindowEvent();

		if ((bApplicationPanelEventResult == FALSE) &&
				(bApplicationListEventResult == FALSE))
			kSleep(0);
	}
}

static BOOL kCreateApplicationPanelWindow(void)
{
	WINDOWMANAGER* pstWindowManager;
	QWORD qwWindowID;

	pstWindowManager = kGetWindowManager();

	// create application panel window
	qwWindowID = kCreateWindow(0, 0, pstWindowManager->stScreenArea.iX2 + 1,
			APPLICATIONPANEL_HEIGHT, NULL, APPLICATIONPANEL_TITLE);
	if (qwWindowID == WINDOW_INVALIDID)
		return FALSE;

	kDrawRect(qwWindowID, 0, 0, pstWindowManager->stScreenArea.iX2,
			APPLICATIONPANEL_HEIGHT - 1, APPLICATIONPANEL_COLOR_OUTERLINE, FALSE);
	kDrawRect(qwWindowID, 1, 1, pstWindowManager->stScreenArea.iX2 - 1,
			APPLICATIONPANEL_HEIGHT - 2, APPLICATIONPANEL_COLOR_MIDDLELINE, FALSE);
	kDrawRect(qwWindowID, 2, 2, pstWindowManager->stScreenArea.iX2 - 2,
			APPLICATIONPANEL_HEIGHT - 3, APPLICATIONPANEL_COLOR_INNERLINE, FALSE);
	kDrawRect(qwWindowID, 3, 3, pstWindowManager->stScreenArea.iX2 - 3,
			APPLICATIONPANEL_HEIGHT - 4, APPLICATIONPANEL_COLOR_BACKGROUND, TRUE);

	// draw list button on left side
	kSetRectangleData(5, 5, 120, 25, &(gs_stApplicationPanelData.stButtonArea));
	kDrawButton(qwWindowID, &(gs_stApplicationPanelData.stButtonArea),
			APPLICATIONPANEL_COLOR_ACTIVE, "Application", RGB(255, 255, 255));

	// draw clock on right side
	kDrawDigitalClock(qwWindowID);
	kShowWindow(qwWindowID, TRUE);

	gs_stApplicationPanelData.qwApplicationPanelID = qwWindowID;
	return TRUE;
}

static void kDrawDigitalClock(QWORD qwWindowID)
{
	RECT stWindowArea;
	RECT stUpdateArea;
	static BYTE s_bPreviousHour, s_bPreviousMinute, s_bPreviousSecond;
	BYTE bHour, bMinute, bSecond;
	char vcBuffer[10] = "00:00 AM";

	kReadRTCTime(&bHour, &bMinute, &bSecond);

	if ((s_bPreviousSecond == bSecond) && (s_bPreviousMinute == bMinute) &&
			(s_bPreviousHour == bHour))
		return;

	s_bPreviousHour = bHour;
	s_bPreviousMinute = bMinute;
	s_bPreviousSecond = bSecond;

	if (bHour >= 12)
	{
		if (bHour > 12)
			bHour -= 12;
		vcBuffer[6] = 'P';
	}

	vcBuffer[0] = '0' + bHour / 10;
	vcBuffer[1] = '0' + bHour % 10;
	vcBuffer[3] = '0' + bMinute / 10;
	vcBuffer[4] = '0' + bMinute % 10;

	if ((bSecond % 2) == 1)
		vcBuffer[2] = ' ';
	else
		vcBuffer[2] = ':';

	// get panel area
	kGetWindowArea(qwWindowID, &stWindowArea);

	kSetRectangleData(stWindowArea.iX2 - APPLICATIONPANEL_CLOCKWIDTH - 13, 5,
			stWindowArea.iX2 - 5, 25, &stUpdateArea);
	kDrawRect(qwWindowID, stUpdateArea.iX1, stUpdateArea.iY1,
			stUpdateArea.iX2, stUpdateArea.iY2, APPLICATIONPANEL_COLOR_INNERLINE,
			FALSE);

	kDrawText(qwWindowID, stUpdateArea.iX1 + 4, stUpdateArea.iY1 + 3,
			RGB(255, 255, 255), APPLICATIONPANEL_COLOR_BACKGROUND, vcBuffer,
			kStrLen(vcBuffer));

	// update clock area
	kUpdateScreenByWindowArea(qwWindowID, &stUpdateArea);
}

static BOOL kProcessApplicationPanelWindowEvent(void)
{
	EVENT stReceivedEvent;
	MOUSEEVENT* pstMouseEvent;
	BOOL bProcessResult;
	QWORD qwApplicationPanelID;
	QWORD qwApplicationListID;

	qwApplicationPanelID = gs_stApplicationPanelData.qwApplicationPanelID;
	qwApplicationListID = gs_stApplicationPanelData.qwApplicationListID;
	bProcessResult = FALSE;

	while (1)
	{
		// draw clock
		kDrawDigitalClock(gs_stApplicationPanelData.qwApplicationPanelID);

		if (kReceiveEventFromWindowQueue(qwApplicationPanelID, &stReceivedEvent)
				== FALSE)
			break;

		bProcessResult = TRUE;

		switch (stReceivedEvent.qwType)
		{
			case EVENT_MOUSE_LBUTTONDOWN:
				pstMouseEvent = &(stReceivedEvent.stMouseEvent);

				if (kIsInRectangle(&(gs_stApplicationPanelData.stButtonArea),
							pstMouseEvent->stPoint.iX, pstMouseEvent->stPoint.iY) == FALSE)
					break;

				if (gs_stApplicationPanelData.bApplicationWindowVisible == FALSE)
				{
					kDrawButton(qwApplicationPanelID, &(gs_stApplicationPanelData.stButtonArea),
							APPLICATIONPANEL_COLOR_BACKGROUND, "Application",
							RGB(255, 255, 255));
					kUpdateScreenByWindowArea(qwApplicationPanelID,
							&(gs_stApplicationPanelData.stButtonArea));

					if (gs_stApplicationPanelData.iPreviousMouseOverIndex != -1)
					{
						kDrawApplicationListItem(
								gs_stApplicationPanelData.iPreviousMouseOverIndex, FALSE);
						gs_stApplicationPanelData.iPreviousMouseOverIndex = -1;
					}
					kMoveWindowToTop(gs_stApplicationPanelData.qwApplicationListID);
					kShowWindow(gs_stApplicationPanelData.qwApplicationListID, TRUE);
					gs_stApplicationPanelData.bApplicationWindowVisible = TRUE;
				}
				else
				{
					kDrawButton(qwApplicationPanelID, &(gs_stApplicationPanelData.stButtonArea),
							APPLICATIONPANEL_COLOR_ACTIVE, "Application",
							RGB(255, 255, 255));
					kUpdateScreenByWindowArea(qwApplicationPanelID,
							&(gs_stApplicationPanelData.stButtonArea));

					kShowWindow(qwApplicationListID, FALSE);
					gs_stApplicationPanelData.bApplicationWindowVisible = FALSE;
				}
				break;

			default:
				break;
		}
	}

	return bProcessResult;
}

static BOOL kCreateApplicationListWindow(void)
{
	int i, iCount;
	int iMaxNameLength, iNameLength;
	QWORD qwWindowID;
	int iX, iY;
	int iWindowWidth;

	iMaxNameLength = 0;
	iCount = sizeof(gs_vstApplicationTable) / sizeof(APPLICATIONENTRY);
	for (i = 0; i < iCount; i++)
	{
		iNameLength = kStrLen(gs_vstApplicationTable[i].pcApplicationName);
		if (iMaxNameLength < iNameLength)
			iMaxNameLength = iNameLength;
	}

	iWindowWidth = iMaxNameLength * FONT_ENGLISHWIDTH + 20;

	iX = gs_stApplicationPanelData.stButtonArea.iX1;
	iY = gs_stApplicationPanelData.stButtonArea.iY2 + 5;

	// create application list
	qwWindowID = kCreateWindow(iX, iY, iWindowWidth,
			iCount * APPLICATIONPANEL_LISTITEMHEIGHT + 1, NULL,
			APPLICATIONPANEL_LISTTITLE);
	if (qwWindowID == WINDOW_INVALIDID)
		return FALSE;

	gs_stApplicationPanelData.iApplicationListWidth = iWindowWidth;
	gs_stApplicationPanelData.bApplicationWindowVisible = FALSE;

	gs_stApplicationPanelData.qwApplicationListID = qwWindowID;
	gs_stApplicationPanelData.iPreviousMouseOverIndex = -1;

	for (i = 0; i < iCount; i++)
		kDrawApplicationListItem(i, FALSE);

	kMoveWindow(qwWindowID, gs_stApplicationPanelData.stButtonArea.iX1,
			gs_stApplicationPanelData.stButtonArea.iY2 + 5);
	return TRUE;
}

static void kDrawApplicationListItem(int iIndex, BOOL bMouseOver)
{
	QWORD qwWindowID;
	int iWindowWidth;
	COLOR stColor;
	RECT stItemArea;

	qwWindowID = gs_stApplicationPanelData.qwApplicationListID;
	iWindowWidth = gs_stApplicationPanelData.iApplicationListWidth;

	if (bMouseOver == TRUE)
		stColor = APPLICATIONPANEL_COLOR_ACTIVE;
	else
		stColor = APPLICATIONPANEL_COLOR_BACKGROUND;

	// list item edge
	kSetRectangleData(0, iIndex * APPLICATIONPANEL_LISTITEMHEIGHT,
			iWindowWidth - 1, (iIndex + 1) * APPLICATIONPANEL_LISTITEMHEIGHT,
			&stItemArea);
	kDrawRect(qwWindowID, stItemArea.iX1, stItemArea.iY1, stItemArea.iX2,
			stItemArea.iY2, APPLICATIONPANEL_COLOR_INNERLINE, FALSE);

	// list item inner
	kDrawRect(qwWindowID, stItemArea.iX1 + 1, stItemArea.iY1 + 1,
			stItemArea.iX2 - 1, stItemArea.iY2 - 1, stColor, TRUE);

	kDrawText(qwWindowID, stItemArea.iX1 + 10, stItemArea.iY1 + 2,
			RGB(255, 255, 255), stColor,
			gs_vstApplicationTable[iIndex].pcApplicationName,
			kStrLen(gs_vstApplicationTable[iIndex].pcApplicationName));

	kUpdateScreenByWindowArea(qwWindowID, &stItemArea);
}

static BOOL kProcessApplicationListWindowEvent(void)
{
	EVENT stReceivedEvent;
	MOUSEEVENT* pstMouseEvent;
	BOOL bProcessResult;
	QWORD qwApplicationPanelID;
	QWORD qwApplicationListID;
	int iMouseOverIndex;
	EVENT stEvent;

	qwApplicationPanelID = gs_stApplicationPanelData.qwApplicationPanelID;
	qwApplicationListID = gs_stApplicationPanelData.qwApplicationListID;
	bProcessResult = FALSE;

	while (1)
	{
		if (kReceiveEventFromWindowQueue(qwApplicationListID, &stReceivedEvent)
				== FALSE)
			break;

		bProcessResult = TRUE;

		switch (stReceivedEvent.qwType)
		{
			case EVENT_MOUSE_MOVE:
				pstMouseEvent = &(stReceivedEvent.stMouseEvent);

				iMouseOverIndex = kGetMouseOverItemIndex(pstMouseEvent->stPoint.iY);

				if ((iMouseOverIndex == gs_stApplicationPanelData.iPreviousMouseOverIndex) ||
						(iMouseOverIndex == -1))
					break;

				// update previous
				if (gs_stApplicationPanelData.iPreviousMouseOverIndex != -1)
					kDrawApplicationListItem(
							gs_stApplicationPanelData.iPreviousMouseOverIndex, FALSE);

				// update current
				kDrawApplicationListItem(iMouseOverIndex, TRUE);

				gs_stApplicationPanelData.iPreviousMouseOverIndex = iMouseOverIndex;
				break;

			case EVENT_MOUSE_LBUTTONDOWN:
				pstMouseEvent = &(stReceivedEvent.stMouseEvent);

				iMouseOverIndex = kGetMouseOverItemIndex(pstMouseEvent->stPoint.iY);
				if (iMouseOverIndex == -1)
					break;

				kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0,
						(QWORD)gs_vstApplicationTable[iMouseOverIndex].pvEntryPoint,
						TASK_LOADBALANCINGID);

				// send event to application panel button to close
				kSetMouseEvent(qwApplicationPanelID, EVENT_MOUSE_LBUTTONDOWN,
						gs_stApplicationPanelData.stButtonArea.iX1 + 1,
						gs_stApplicationPanelData.stButtonArea.iY1 + 1,
						NULL, &stEvent);
				kSendEventToWindow(qwApplicationPanelID, &stEvent);
				break;

			default:
				break;
		}
	}

	return bProcessResult;
}

static int kGetMouseOverItemIndex(int iMouseY)
{
	int iCount;
	int iItemIndex;

	iCount = sizeof(gs_vstApplicationTable) / sizeof(APPLICATIONENTRY);
	iItemIndex = iMouseY / APPLICATIONPANEL_LISTITEMHEIGHT;

	if ((iItemIndex < 0) || (iItemIndex >= iCount))
		return -1;

	return iItemIndex;
}


