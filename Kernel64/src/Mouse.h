#ifndef __MOUSE_H__ 
#define __MOUSE_H__

#include "Types.h"
#include "Synchronization.h"

#define MOUSE_MAXQUEUECOUNT		100

#define MOUSE_LBUTTONDOWN		0x01
#define MOUSE_RBUTTONDOWN		0x02
#define MOUSE_MBUTTONDOWN		0x04

#pragma pack(push, 1)

typedef struct kMousePacketStruct
{
	BYTE bButtonStatusAndFlag;
	BYTE bXMovement;
	BYTE bYMovement;
} MOUSEDATA;

#pragma pack(pop)

typedef struct kMouseManagerStruct
{
	SPINLOCK stSpinLock;
	int iByteCount;
	MOUSEDATA stCurrentData;
} MOUSEMANAGER;

BOOL kInitializeMouse(void);
BOOL kAccumulateMouseDataAndPutQueue(BYTE bMouseData);
BOOL kActivateMouse(void);
void kEnableMouseInterrupt(void);
BOOL kIsMouseDataInOutputBuffer(void);
BOOL kGetMouseDataFromMouseQueue(BYTE* pbButtonStatus, int* piRelativeX,
		int* piRelativeY);

#endif
