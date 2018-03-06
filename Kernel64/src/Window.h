#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "Types.h"
#include "Synchronization.h"
#include "2DGraphics.h"
#include "List.h"

#define WINDOW_MAXCOUNT			2048

#define GETWINDOWOFFSET(x)		((x) & 0xFFFFFFFF)
#define WINDOW_TITLEMAXLENGTH	40
#define WINDOW_INVALIDID		0xFFFFFFFFFFFFFFFF

#define WINDOW_FLAGS_SHOW		0x00000001
#define WINDOW_FLAGS_DRAWFRAME	0x00000002
#define WINDOW_FLAGS_DRAWTITLE	0x00000004

#define WINDOW_FLAGS_DEFAULT	( WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | \
								WINDOW_FLAGS_DRAWTITLE )

#define WINDOW_TITLEBAR_HEIGHT	21
#define WINDOW_XBUTTON_SIZE		19

#define WINDOW_COLOR_FRAME					RGB(109, 218, 22)
#define WINDOW_COLOR_BACKGROUND				RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARTEXT			RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARBACKGROUND		RGB(79, 204, 11)
#define WINDOW_COLOR_TITLEBARBRIGHT1		RGB(183, 249, 171)
#define WINDOW_COLOR_TITLEBARBRIGHT2		RGB(150, 210, 140)
#define WINDOW_COLOR_TITLEBARUNDERLINE		RGB(46, 59, 30)
#define WINDOW_COLOR_BUTTONBRIGHT			RGB(229, 229, 229)
#define WINDOW_COLOR_BUTTONDARK				RGB(86, 86, 86)
#define WINDOW_COLOR_SYSTEMBACKGROUND		RGB(232, 255, 232)
#define WINDOW_COLOR_XBUTTONLINECOLOR		RGB(71, 199, 21)

#define WINDOW_BACKGROUNDWINDOWTITLE		"SYS_BACKGROUND"

#define MOUSE_CURSOR_WIDTH		20
#define MOUSE_CURSOR_HEIGHT		20

#define MOUSE_CURSOR_OUTERLINE	RGB(0, 0, 0)
#define MOUSE_CURSOR_OUTER		RGB(79, 204, 11)
#define MOUSE_CURSOR_INNER		RGB(232, 255, 232)


typedef struct kWindowStruct
{
	LISTLINK stLink;
	MUTEX stLock;
	RECT stArea;
	COLOR* pstWindowBuffer;
	QWORD qwTaskID;
	DWORD dwFlags;
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
} WINDOWMANAGER;



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
BOOL kRedrawWindowByArea(const RECT* pstArea);
static void kCopyWindowBufferToFrameBuffer(const WINDOW* pstWindow,
		const RECT* pstCopyArea);

// drawing functions
BOOL kDrawWindowFrame(QWORD qwWindowID);
BOOL kDrawWindowBackground(QWORD qwWindowID);
BOOL kDrawWindowTitle(QWORD qwWindowID, const char* pcTitle);
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

#endif
