#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"
#include "Task.h"

BYTE kInPortByte(WORD wPort);
void kOutPortByte(WORD wPort, BYTE bData);
void kLoadGDTR(QWORD qwGDTRAddress);
void kLoadTR(WORD wTSSSegmentOffset);
void kLoadIDTR(QWORD qwIDTRAddress);
void kEnableInterrupt(void);
void kDisableInterrupt(void);
QWORD kReadRFLAGS(void);
QWORD kReadTSC(void);
void kSwitchContext(CONTEXT* pstCurrentContext, CONTEXT* pstNextContext);
void kHlt(void);
BOOL kTestAndSet(volatile BYTE* pbDestination, BYTE bCompare, BYTE bSource);
void kPause(void);

void kInitializeFPU(void);
void kSaveFPUContext(void* pvFPUContext);
void kLoadFPUContext(void* pvFPUContext);
void kSetTS(void);
void kClearTS(void);

WORD kInPortWord(WORD wPort);
void kOutPortWord(WORD wPort, WORD wData);

void kEnableGlobalLocalAPIC(void);

void kReadMSR(QWORD qwMSRAddress, QWORD* pqwRDX, QWORD* pqwRAX);
void kWriteMSR(QWORD qwMSRAddress, QWORD pqwRDX, QWORD pqwRAX);

#endif
