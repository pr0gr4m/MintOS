#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "Types.h"
#include "Synchronization.h"
#include "2DGraphics.h"
#include "List.h"
#include "Queue.h"
#include "Keyboard.h"

#define WINDOW_MAXCOUNT			2048

#define GETWINDOWOFFSET(x)		((x) & 0xFFFFFFFF)
#define WINDOW_TITLEMAXLENGTH	40
#define WINDOW_INVALIDID		0xFFFFFFFFFFFFFFFF

#define WINDOW_FLAGS_SHOW		0x00000001
#define WINDOW_FLAGS_DRAWFRAME	0x00000002
#define WINDOW_FLAGS_DRAWTITLE	0x00000004
#define WINDOW_FLAGS_RESIZABLE	0x00000008

#define WINDOW_FLAGS_DEFAULT	( WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | \
								WINDOW_FLAGS_DRAWTITLE )

#define WINDOW_TITLEBAR_HEIGHT	21
#define WINDOW_XBUTTON_SIZE		19
#define	WINDOW_WIDTH_MIN		( WINDOW_XBUTTON_SIZE * 2 + 30 )
#define WINDOW_HEIGHT_MIN		( WINDOW_TITLEBAR_HEIGHT + 30 )

#define WINDOW_COLOR_FRAME							RGB(109, 218, 22)
#define WINDOW_COLOR_BACKGROUND						RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARTEXT					RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARACTIVEBACKGROUND		RGB(79, 204, 11)
#define WINDOW_COLOR_TITLEBARINACTIVEBACKGROUND		RGB(55, 135, 11)
#define WINDOW_COLOR_TITLEBARBRIGHT1				RGB(183, 249, 171)
#define WINDOW_COLOR_TITLEBARBRIGHT2				RGB(150, 210, 140)
#define WINDOW_COLOR_TITLEBARUNDERLINE				RGB(46, 59, 30)
#define WINDOW_COLOR_BUTTONBRIGHT					RGB(229, 229, 229)
#define WINDOW_COLOR_BUTTONDARK						RGB(86, 86, 86)
#define WINDOW_COLOR_SYSTEMBACKGROUND				RGB(232, 255, 232)
#define WINDOW_COLOR_XBUTTONLINECOLOR				RGB(71, 199, 21)

#define WINDOW_BACKGROUNDWINDOWTITLE		"SYS_BACKGROUND"

#define MOUSE_CURSOR_WIDTH		20
#define MOUSE_CURSOR_HEIGHT		20

#define MOUSE_CURSOR_OUTERLINE	RGB(0, 0, 0)
#define MOUSE_CURSOR_OUTER		RGB(79, 204, 11)
#define MOUSE_CURSOR_INNER		RGB(232, 255, 232)

#define EVENTQUEUE_WINDOWMAXCOUNT			100
#define EVENTQUEUE_WINDOWMANAGERMAXCOUNT	WINDOW_MAXCOUNT

#define EVENT_UNKNOWN									0

#define EVENT_MOUSE_MOVE								1
#define EVENT_MOUSE_LBUTTONDOWN							2
#define EVENT_MOUSE_LBUTTONUP							3
#define EVENT_MOUSE_RBUTTONDOWN							4
#define EVENT_MOUSE_RBUTTONUP							5
#define EVENT_MOUSE_MBUTTONDOWN							6
#define EVENT_MOUSE_MBUTTONUP							7

#define EVENT_WINDOW_SELECT								8
#define EVENT_WINDOW_DESELECT							9
#define EVENT_WINDOW_MOVE								10
#define EVENT_WINDOW_RESIZE								11
#define EVENT_WINDOW_CLOSE								12

#define EVENT_KEY_DOWN									13
#define EVENT_KEY_UP									14

#define EVENT_WINDOWMANAGER_UPDATESCREENBYID			15
#define EVENT_WINDOWMANAGER_UPDATESCREENBYWINDOWAREA	16
#define EVENT_WINDOWMANAGER_UPDATESCREENBYSCREENAREA	17

#define WINDOW_OVERLAPPEDAREALOGMAXCOUNT				20

//struct

typedef struct kMouseEventStruct
{
	QWORD qwWindowID;

	POINT stPoint;
	BYTE bButtonStatus;
} MOUSEEVENT;

typedef struct kKeyEventStruct
{
	QWORD qwWindowID;

	BYTE bASCIICode;
	BYTE bScanCode;

	BYTE bFlags;
} KEYEVENT;

typedef struct kWindowEventStruct
{
	QWORD qwWindowID;

	RECT stArea;
} WINDOWEVENT;

// Integration Event
typedef struct kEventStruct
{
	QWORD qwType;	// event type

	union
	{
		MOUSEEVENT stMouseEvent;
		KEYEVENT stKeyEvent;
		WINDOWEVENT stWindowEvent;
		QWORD vqwData[3];	// use defined
	};
} EVENT;

typedef struct kWindowStruct
{
	LISTLINK stLink;
	MUTEX stLock;
	RECT stArea;
	COLOR* pstWindowBuffer;
	QWORD qwTaskID;
	DWORD dwFlags;

	QUEUE stEventQueue;
	EVENT* pstEventBuffer;

	char vcWindowTitle[WINDOW_TITLEMAXLENGTH + 1];
} WINDOW;

typedef struct kWindowPoolManagerStruct
{
	MUTEX stLock;

	WINDOW* pstStartAddress;
	int iMaxCount;
	int iUseCount;

	int iAllocatedCount;
} WINDOWPOOLMANAGER;

typedef struct kWindowManagerStruct
{
	MUTEX stLock;
	LIST stWindowList;

	int iMouseX;
	int iMouseY;

	RECT stScreenArea;
	COLOR* pstVideoMemory;

	QWORD qwBackgroundWindowID;

	QUEUE stEventQueue;
	EVENT* pstEventBuffer;

	BYTE bPreviousButtonStatus;

	QWORD qwMovingWindowID;
	BOOL bWindowMoveMode;

	BOOL bWindowResizeMode;
	QWORD qwResizingWindowID;
	RECT stResizingWindowArea;

	BYTE* pbDrawBitmap;
} WINDOWMANAGER;

typedef struct kDrawBitmapStruct
{
	RECT stArea;
	BYTE* pbBitmap;
} DRAWBITMAP;


// window pool functions
static void kInitializeWindowPool(void);
static WINDOW* kAllocateWindow(void);
static void kFreeWindow(QWORD qwID);

// window and window manager functions
void kInitializeGUISystem(void);
WINDOWMANAGER* kGetWindowManager(void);
QWORD kGetBackgroundWindowID(void);
void kGetScreenArea(RECT* pstScreenArea);
QWORD kCreateWindow(int iX, int iY, int iWidth, int iHeight, DWORD dwFlags,
		const char* pcTitle);
BOOL kDeleteWindow(QWORD qwWindowID);
BOOL kDeleteAllWindowInTaskID(QWORD qwTaskID);
WINDOW* kGetWindow(QWORD qwWindowID);
WINDOW* kGetWindowWithWindowLock(QWORD qwWindowID);
BOOL kShowWindow(QWORD qwWindowID, BOOL bShow);
BOOL kRedrawWindowByArea(const RECT* pstArea, QWORD qwDrawWindowID);
static void kCopyWindowBufferToFrameBuffer(const WINDOW* pstWindow,
		DRAWBITMAP* pstDrawBitmap);

QWORD kFindWindowByPoint(int iX, int iY);
QWORD kFindWindowByTitle(const char* pcTitle);
BOOL kIsWindowExist(QWORD qwWindowID);
QWORD kGetTopWindowID(void);
BOOL kMoveWindowToTop(QWORD qwWindowID);
BOOL kIsInTitleBar(QWORD qwWindowID, int iX, int iY);
BOOL kIsInCloseButton(QWORD qwWindowID, int iX, int iY);
BOOL kMoveWindow(QWORD qwWindowID, int iX, int iY);
static BOOL kUpdateWindowTitle(QWORD qwWindowID, BOOL bSelectedTitle);
BOOL kResizeWindow(QWORD qwWindowID, int iX, int iY, int iWidth, int iHeight);
BOOL kIsInResizeButton(QWORD qwWindowID, int iX, int iY);

// convert point functions
BOOL kGetWindowArea(QWORD qwWindowID, RECT* pstArea);
// screen -> window
BOOL kConvertPointScreenToClient(QWORD qwWindowID, const POINT* pstXY,
		POINT* pstXYInWindow);
// window -> screen
BOOL kConvertPointClientToScreen(QWORD qwWindowID, const POINT* pstXY,
		POINT* pstXYInScreen);
// screen -> window
BOOL kConvertRectScreenToClient(QWORD qwWindowID, const RECT* pstArea,
		RECT* pstAreaInWindow);
// window -> screen
BOOL kConvertRectClientToScreen(QWORD qwWindowID, const RECT* pstArea,
		RECT* pstAreaInScreen);

// update screen functions
BOOL kUpdateScreenByID(QWORD qwWindowID);
BOOL kUpdateScreenByWindowArea(QWORD qwWindowID, const RECT* pstArea);
BOOL kUpdateScreenByScreenArea(const RECT* pstArea);

// event functions
BOOL kSendEventToWindow(QWORD qwWindowID, const EVENT* pstEvent);
BOOL kReceiveEventFromWindowQueue(QWORD qwWindowID, EVENT* pstEvent);
BOOL kSendEventToWindowManager(const EVENT* pstEvent);
BOOL kReceiveEventFromWindowManagerQueue(EVENT* pstEvent);
BOOL kSetMouseEvent(QWORD qwWindowID, QWORD qwEventType, int iMouseX, int iMouseY,
		BYTE bButtonStatus, EVENT* pstEvent);
BOOL kSetWindowEvent(QWORD qwWindowID, QWORD qwEventType, EVENT* pstEvent);
void kSetKeyEvent(QWORD qwWindow, const KEYDATA* pstKeyData, EVENT* pstEvent);

// drawing functions
BOOL kDrawWindowFrame(QWORD qwWindowID);
BOOL kDrawWindowBackground(QWORD qwWindowID);
BOOL kDrawWindowTitle(QWORD qwWindowID, const char* pcTitle, BOOL bSelectedTitle);
BOOL kDrawButton(QWORD qwWindowID, RECT* pstButtonArea, COLOR stBackgroundColor,
		const char* pcText, COLOR stTextColor);
BOOL kDrawPixel(QWORD qwWindowID, int iX, int iY, COLOR stColor);
BOOL kDrawLine(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2, COLOR stColor);
BOOL kDrawRect(QWORD qwWindowID, int iX1, int iY1, int iX2, int iY2,
		COLOR stColor, BOOL bFill);
BOOL kDrawCircle(QWORD qwWindowID, int iX, int iY, int iRadius, COLOR stColor,
		BOOL bFill);
BOOL kDrawText(QWORD qwWindowID, int iX, int iY, COLOR stTextColor, 
		COLOR stBackgroundColor, const char* pcString, int iLength);
static void kDrawCursor(int iX, int iY);
void kMoveCursor(int iX, int iY);
void kGetCursorPosition(int* piX, int* piY);

void kGetRandomXY(int* piX, int* piY);
COLOR kGetRandomColor(void);

// update bitmap functions
BOOL kCreateDrawBitmap(const RECT* pstArea, DRAWBITMAP* pstDrawBitmap);
static BOOL kFillDrawBitmap(DRAWBITMAP* pstDrawBitmap, RECT* pstArea, BOOL bFill);
BOOL kGetStartPositionInDrawBitmap(const DRAWBITMAP* pstDrawBitmap,
		int iX, int iY, int* piByteOffset, int* piBitOffset);
BOOL kIsDrawBitmapAllOff(const DRAWBITMAP* pstDrawBitmap);

BOOL kBitBlt(QWORD qwWindowID, int iX, int iY, COLOR* pstBuffer, int iWidth, int iHeight);
void kDrawBackgroundImage(void);

#endif
