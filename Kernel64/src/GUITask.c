#include "GUITask.h"
#include "Window.h"
#include "Utility.h"
#include "Console.h"

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
			iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT, "Hello World Window");
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
