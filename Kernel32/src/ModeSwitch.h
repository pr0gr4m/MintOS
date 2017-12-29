#ifndef __MODE_SWITCH_H__
#define __MODE_SWITCH_H__

#include "Types.h"

void kReadCPUID(DWORD dwEAX, DWORD* pdwEAX, DWORD* pdwEBX, DWORD* pdwECX, DWORD* pdwEDX);
void kSwitchAndExecute64bitKernel(void);

#endif
