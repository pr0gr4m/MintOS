#ifndef __WINDOWMANAGERTASK_H__
#define __WINDOWMANAGERTASK_H__

#define WINDOWMANAGER_DATAACCUMULATECOUNT	20
#define WINDOWMANAGER_RESIZEMARKERSIZE		20
#define WINDOWMANAGER_COLOR_RESIZEMARKER	RGB(210, 20, 20)
#define WINDOWMANAGER_THICK_RESIZEMARKER	4

void kStartWindowManager(void);
void kStartGraphicModeTest(void);

BOOL kProcessMouseData(void);
BOOL kProcessKeyData(void);
BOOL kProcessEventQueueData(void);

void kDrawResizeMarker(const RECT* pstArea, BOOL bShowMarker);

#endif
