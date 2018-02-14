#include "Task.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Descriptor.h"
#include "Synchronization.h"
#include "Console.h"

static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

// Task Functions

static void kInitializeTCBPool(void)
{
	int i;

	kMemSet(&(gs_stTCBPoolManager), 0, sizeof(gs_stTCBPoolManager));

	gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
	kMemSet((void*)TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

	for (i = 0; i < TASK_MAXCOUNT; i++)
	{
		gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	}

	gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
	gs_stTCBPoolManager.iAllocatedCount = 1;
}

static TCB* kAllocateTCB(void)
{
	TCB* pstEmptyTCB;
	int i;

	if (gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount)
	{
		return NULL;
	}

	for (i = 0; i < gs_stTCBPoolManager.iMaxCount; i++)
	{
		if ((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0)
		{
			pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
			break;
		}
	}

	// set TCB
	pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount << 32) | i;
	gs_stTCBPoolManager.iUseCount++;
	gs_stTCBPoolManager.iAllocatedCount++;
	if (gs_stTCBPoolManager.iAllocatedCount == 0)
	{
		gs_stTCBPoolManager.iAllocatedCount = 1;
	}

	return pstEmptyTCB;
}

static void kFreeTCB(QWORD qwID)
{
	int i;

	i = GETTCBOFFSET(qwID);
	kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	gs_stTCBPoolManager.iUseCount--;
}

TCB* kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress)
{
	TCB* pstTask;
	void* pvStackAddress;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();
	pstTask = kAllocateTCB();
	if (pstTask == NULL)
	{
		kUnlockForSystemData(bPreviousFlag);
		return NULL;
	}
	kUnlockForSystemData(bPreviousFlag);

	pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE *
				GETTCBOFFSET(pstTask->stLink.qwID)));

	kSetupTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress,
			TASK_STACKSIZE);

	bPreviousFlag = kLockForSystemData();
	kAddTaskToReadyList(pstTask);
	kUnlockForSystemData(bPreviousFlag);

	return pstTask;
}

static void kSetupTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress,
		void* pvStackAddress, QWORD qwStackSize)
{
	kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));

	// set RSP, RBP
	pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress +
		qwStackSize;
	pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress +
		qwStackSize;

	// set segment selector
	pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;
	pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;

	// set RIP register
	pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;

	// activate interrupt by set IF bit
	pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x0200;

	// save ID, stack, flags
	pstTCB->pvStackAddress = pvStackAddress;
	pstTCB->qwStackSize = qwStackSize;
	pstTCB->qwFlags = qwFlags;
}



// Scheduler Functions

void kInitializeScheduler(void)
{
	int i;

	kInitializeTCBPool();

	for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++)
	{
		kInitializeList(&(gs_stScheduler.vstReadyList[i]));
		gs_stScheduler.viExecuteCount[i] = 0;
	}
	kInitializeList(&(gs_stScheduler.stWaitList));

	gs_stScheduler.pstRunningTask = kAllocateTCB();
	gs_stScheduler.pstRunningTask->qwFlags = TASK_FLAGS_HIGHEST;

	// Init for calc processor rate
	gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
	gs_stScheduler.qwProcessorLoad = 0;
}

void kSetRunningTask(TCB* pstTask)
{
	BOOL bPreviousFlag;
	bPreviousFlag = kLockForSystemData();
	gs_stScheduler.pstRunningTask = pstTask;
	kUnlockForSystemData(bPreviousFlag);
}

TCB* kGetRunningTask(void)
{
	BOOL bPreviousFlag;
	TCB* pstRunningTask;

	bPreviousFlag =  kLockForSystemData();
	pstRunningTask = gs_stScheduler.pstRunningTask;
	kUnlockForSystemData(bPreviousFlag);
	return pstRunningTask;
}

static TCB* kGetNextTaskToRun(void)
{
	TCB* pstTarget = NULL;
	int iTaskCount, i, j;

	for (j = 0; j < 2; j++)
	{
		for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++)
		{
			iTaskCount = kGetListCount(&(gs_stScheduler.vstReadyList[i]));
			// Return current priority task if 
			// task count is more than execute count
			if (gs_stScheduler.viExecuteCount[i] < iTaskCount)
			{
				pstTarget = (TCB*)kRemoveListFromHeader(
						&(gs_stScheduler.vstReadyList[i]));
				gs_stScheduler.viExecuteCount[i]++;
				break;
			}
			else
			{
				gs_stScheduler.viExecuteCount[i] = 0;
			}
		}

		if (pstTarget != NULL)
			break;
	}

	return pstTarget;
}

static BOOL kAddTaskToReadyList(TCB* pstTask)
{
	BYTE bPriority;

	bPriority = GETPRIORITY(pstTask->qwFlags);
	if (bPriority >= TASK_MAXREADYLISTCOUNT)
		return FALSE;

	kAddListToTail(&(gs_stScheduler.vstReadyList[bPriority]), pstTask);
	return TRUE;
}

static TCB* kRemoveTaskFromReadyList(QWORD qwTaskID)
{
	TCB* pstTarget;
	BYTE bPriority;

	// Invalid task ID
	if (GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT)
		return NULL;

	// Check ID
	pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
	if (pstTarget->stLink.qwID != qwTaskID)
		return NULL;

	bPriority = GETPRIORITY(pstTarget->qwFlags);
	pstTarget = kRemoveList(&(gs_stScheduler.vstReadyList[bPriority]), qwTaskID);
	return pstTarget;
}

BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority)
{
	TCB* pstTarget;
	BOOL bPreviousFlag;

	if (bPriority > TASK_MAXREADYLISTCOUNT)
		return FALSE;

	bPreviousFlag = kLockForSystemData();

	// Just change priority if current running task
	pstTarget = gs_stScheduler.pstRunningTask;
	if (pstTarget->stLink.qwID == qwTaskID)
	{
		SETPRIORITY(pstTarget->qwFlags, bPriority);
	}
	// Seek from ready list and change if not current running task
	else
	{
		pstTarget = kRemoveTaskFromReadyList(qwTaskID);
		if (pstTarget == NULL)
		{
			// if not in ready list
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if (pstTarget != NULL)
			{
				SETPRIORITY(pstTarget->qwFlags, bPriority);
			}
		}
		else
		{
			// Re-insert to ready list after changing
			SETPRIORITY(pstTarget->qwFlags, bPriority);
			kAddTaskToReadyList(pstTarget);
		}
	}

	kUnlockForSystemData(bPreviousFlag);

	return TRUE;
}

void kSchedule(void)
{
	TCB* pstRunningTask, * pstNextTask;
	BOOL bPreviousFlag;

	if (kGetReadyTaskCount() < 1)
		return;

	bPreviousFlag = kLockForSystemData();
	// Get Next Task
	pstNextTask = kGetNextTaskToRun();
	if (pstNextTask == NULL)
	{
		kUnlockForSystemData(bPreviousFlag);
		return;
	}

	// Add Current Running Task to Ready List
	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
	{
		gs_stScheduler.qwSpendProcessorTimeInIdleTask +=
			TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
	}

	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

	if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)
	{
		kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
		kSwitchContext(NULL, &(pstNextTask->stContext));
	}
	else
	{
		kAddTaskToReadyList(pstRunningTask);
		kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
	}

	kUnlockForSystemData(bPreviousFlag);
}

// call if only interrupt/exception is occurred
BOOL kScheduleInInterrupt(void)
{
	TCB* pstRunningTask, * pstNextTask;
	char* pcContextAddress;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	pstNextTask = kGetNextTaskToRun();
	if (pstNextTask == NULL)
	{
		kUnlockForSystemData(bPreviousFlag);
		return FALSE;
	}

	// task switch
	// Overwrites the context savedby the interrupt handler with another context
	pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
	{
		gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
	}

	if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)
	{
		kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
	}
	else
	{
		kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
		kAddTaskToReadyList(pstRunningTask);
	}

	kUnlockForSystemData(bPreviousFlag);

	kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
	return TRUE;
}

void kDecreaseProcessorTime(void)
{
	if (gs_stScheduler.iProcessorTime > 0)
	{
		gs_stScheduler.iProcessorTime--;
	}
}

BOOL kIsProcessorTimeExpired(void)
{
	if (gs_stScheduler.iProcessorTime <= 0)
		return TRUE;
	return FALSE;
}

BOOL kEndTask(QWORD qwTaskID)
{
	TCB* pstTarget;
	BYTE bPriority;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	pstTarget = gs_stScheduler.pstRunningTask;
	if (pstTarget->stLink.qwID == qwTaskID)
	{
		// set End Task bit and switch task
		// if current running task
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

		kUnlockForSystemData(bPreviousFlag);
		kSchedule();

		// Never reached..
		while (1);
	}
	else
	{
		pstTarget = kRemoveTaskFromReadyList(qwTaskID);
		if (pstTarget == NULL)
		{
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if (pstTarget != NULL)
			{
				pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
				SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
			}
			kUnlockForSystemData(bPreviousFlag);
			return FALSE;
		}

		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
		kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
	}
	kUnlockForSystemData(bPreviousFlag);
	return TRUE;
}

void kExitTask(void)
{
	kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

int kGetReadyTaskCount(void)
{
	int iTotalCount = 0;
	int i;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++)
	{
		iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
	}

	kUnlockForSystemData(bPreviousFlag);
	return iTotalCount;
}

int kGetTaskCount(void)
{
	int iTotalCount;
	BOOL bPreviousFlag;

	iTotalCount = kGetReadyTaskCount();

	bPreviousFlag = kLockForSystemData();
	iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;
	kUnlockForSystemData(bPreviousFlag);
	return iTotalCount;
}

TCB* kGetTCBInTCBPool(int iOffset)
{
	if ((iOffset < -1) || (iOffset > TASK_MAXCOUNT))
		return NULL;

	return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}

BOOL kIsTaskExist(QWORD qwID)
{
	TCB* pstTCB;

	pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
	if ((pstTCB == NULL) || (pstTCB->stLink.qwID != qwID))
		return FALSE;
	
	return TRUE;
}

QWORD kGetProcessorLoad(void)
{
	return gs_stScheduler.qwProcessorLoad;
}


// Idle Task Functions

void kIdleTask(void)
{
	TCB* pstTask;
	QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
	QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
	BOOL bPreviousFlag;
	QWORD qwTaskID;

	qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
	qwLastMeasureTickCount = kGetTickCount();

	while (1)
	{
		qwCurrentMeasureTickCount = kGetTickCount();
		qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

		if (qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0)
		{
			gs_stScheduler.qwProcessorLoad = 0;
		}
		else
		{
			gs_stScheduler.qwProcessorLoad = 100 -
				(qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) *
				100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount);
		}

		qwLastMeasureTickCount = qwCurrentMeasureTickCount;
		qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

		// Processor Sleep
		kHaltProcessorByLoad();

		if (kGetListCount(&(gs_stScheduler.stWaitList)) >= 0)
		{
			while (1)
			{
				bPreviousFlag = kLockForSystemData();
				pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
				if (pstTask == NULL)
				{
					kUnlockForSystemData(bPreviousFlag);
					break;
				}
				qwTaskID = pstTask->stLink.qwID;
				kFreeTCB(qwTaskID);
				kUnlockForSystemData(bPreviousFlag);
				
				kPrintf("IDLE: Task ID[0x%q] is completely ended.\n", qwTaskID);
			}
		}

		kSchedule();
	}
}

void kHaltProcessorByLoad(void)
{
	if (gs_stScheduler.qwProcessorLoad < 40)
	{
		kHlt();
		kHlt();
		kHlt();
	}
	else if (gs_stScheduler.qwProcessorLoad < 80)
	{
		kHlt();
		kHlt();
	}
	else if (gs_stScheduler.qwProcessorLoad < 95)
	{
		kHlt();
	}
}

