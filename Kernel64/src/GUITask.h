#ifndef __GUITASK_H__
#define __GUITASK_H__

#include "Types.h"

#define EVENT_USER_TESTMESSAGE		0x80000001

// system monitor task macro
#define SYSTEMMONITOR_PROCESSOR_WIDTH	150
#define SYSTEMMONITOR_PROCESSOR_MARGIN	20
#define SYSTEMMONITOR_PROCESSOR_HEIGHT	150
#define SYSTEMMONITOR_WINDOW_HEIGHT		310
#define SYSTEMMONITOR_MEMORY_HEIGHT		100
#define SYSTEMMONITOR_BAR_COLOR			RGB(55, 215, 47)

// base GUI task functions
void kBaseGUITask(void);
void kHelloWorldGUITask(void);

// system monitor task functions
void kSystemMonitorTask(void);
static void kDrawProcessorInformation(QWORD qwWindowID, int iX, int iY, BYTE bAPICID);
static void kDrawMemoryInformation(QWORD qwWindowID, int iY, int iWindowWidth);

#endif
