#ifndef __WINDOWMANAGERTASK_H__
#define __WINDOWMANAGERTASK_H__

void kStartWindowManager(void);
void kStartGraphicModeTest(void);

BOOL kProcessMouseData(void);
BOOL kProcessKeyData(void);
BOOL kProcessEventQueueData(void);

#endif
