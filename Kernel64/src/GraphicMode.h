#ifndef __GRAPHICMODE_H__
#define __GRAPHICMODE_H__

#include "Types.h"
#include "2DGraphics.h"

void kStartGraphicModeTest();
void kGetRandomXY(int* piX, int* piY);
COLOR kGetRandomColor(void);
void kDrawWindowFrame(int iX, int iY, int iWidth, int iHeight, const char* pcTitle);

#endif
