#ifndef __GRAPHICMODE_H__
#define __GRAPHICMODE_H__

#include "Types.h"
#include "2DGraphics.h"

#define MOUSE_CURSOR_WIDTH		20
#define MOUSE_CURSOR_HEIGHT		20

#define MOUSE_CURSOR_OUTERLINE	RGB(0, 0, 0)
#define MOUSE_CURSOR_OUTER		RGB(79, 204, 11)
#define MOUSE_CURSOR_INNER		RGB(232, 255, 232)

void kStartGraphicModeTest();
void kGetRandomXY(int* piX, int* piY);
COLOR kGetRandomColor(void);
void kDrawWindowFrame(int iX, int iY, int iWidth, int iHeight, const char* pcTitle);

// mouse
void kDrawCursor(RECT* pstArea, COLOR* pstVideoMemory, int iX, int iY);

#endif
