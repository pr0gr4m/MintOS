#include "Task.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Descriptor.h"
#include "Console.h"
#include "MultiProcessor.h"
#include "MPConfigurationTable.h"
#include "DynamicMemory.h"

static SCHEDULER gs_vstScheduler[MAXPROCESSORCOUNT];
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
	kInitializeSpinLock(&gs_stTCBPoolManager.stSpinLock);
}

static TCB* kAllocateTCB(void)
{
	TCB* pstEmptyTCB;
	int i;

	kLockForSpinLock(&gs_stTCBPoolManager.stSpinLock);

	if (gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount)
	{
		kUnlockForSpinLock(&gs_stTCBPoolManager.stSpinLock);
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

	kUnlockForSpinLock(&gs_stTCBPoolManager.stSpinLock);
	return pstEmptyTCB;
}

static void kFreeTCB(QWORD qwID)
{
	int i;

	i = GETTCBOFFSET(qwID);
	kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
	kLockForSpinLock(&gs_stTCBPoolManager.stSpinLock);
	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	gs_stTCBPoolManager.iUseCount--;
	kUnlockForSpinLock(&gs_stTCBPoolManager.stSpinLock);
}

TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize,
		QWORD qwEntryPointAddress, BYTE bAffinity)
{
	TCB* pstTask, * pstProcess;
	void* pvStackAddress;
	BYTE bCurrentAPICID;

	bCurrentAPICID = kGetAPICID();

	if ((pstTask = kAllocateTCB()) == NULL)
		return NULL;

	// allocate stack
	if ((pvStackAddress = kAllocateMemory(TASK_STACKSIZE)) == NULL)
	{
		kFreeTCB(pstTask->stLink.qwID);
		return NULL;
	}

	kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	pstProcess = kGetProcessByThread(kGetRunningTask(bCurrentAPICID));
	if (pstProcess == NULL)
	{
		kFreeTCB(pstTask->stLink.qwID);
		kFreeMemory(pvStackAddress);
		kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		return NULL;
	}

	if (qwFlags & TASK_FLAGS_THREAD)
	{
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
		pstTask->qwMemorySize = pstProcess->qwMemorySize;

		// not TCB link but thread link
		kAddListToTail(&(pstProcess->stChildThreadList), &(pstTask->stThreadLink));
	}
	else
	{
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pvMemoryAddress;
		pstTask->qwMemorySize = qwMemorySize;
	}

	pstTask->stThreadLink.qwID = pstTask->stLink.qwID;

	kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	kSetupTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress,
			TASK_STACKSIZE);

	kInitializeList(&(pstTask->stChildThreadList));

	pstTask->bFPUUsed = FALSE;

	pstTask->bAPICID = bCurrentAPICID;
	pstTask->bAffinity = bAffinity;

	kAddTaskToSchedulerWithLoadBalancing(pstTask);

	return pstTask;
}

static void kSetupTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress,
		void* pvStackAddress, QWORD qwStackSize)
{
	kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));

	// set RSP, RBP
	pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress +
		qwStackSize - 8;
	pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress +
		qwStackSize - 8;

	// set kExitTask to return address
	*(QWORD *)((QWORD)pvStackAddress + qwStackSize - 8) = (QWORD)kExitTask;

	// set segment selector
	if ((qwFlags & TASK_FLAGS_USERLEVEL) == 0)
	{
		// kernel segment descriptor
		pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT | SELECTOR_RPL_0;
		pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
		pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
		pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
		pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
		pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT | SELECTOR_RPL_0;
	}
	else
	{
		// user segment descriptor
		pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_USERCODESEGMENT | SELECTOR_RPL_3;
		pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
		pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
		pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
		pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
		pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_USERDATASEGMENT | SELECTOR_RPL_3;
	}

	// set RIP register
	pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;

	// activate interrupt by set IF bit and allow user access IO port
	// by set IOPL bit
	pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x3200;

	// save ID, stack, flags
	pstTCB->pvStackAddress = pvStackAddress;
	pstTCB->qwStackSize = qwStackSize;
	pstTCB->qwFlags = qwFlags;
}



// Scheduler Functions

void kInitializeScheduler(void)
{
	int i, j;
	BYTE bCurrentAPICID;
	TCB* pstTask;

	bCurrentAPICID = kGetAPICID();
	if (bCurrentAPICID == 0)
	{
		kInitializeTCBPool();

		for (j = 0; j < MAXPROCESSORCOUNT; j++)
		{
			for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++)
			{
				kInitializeList(&(gs_vstScheduler[j].vstReadyList[i]));
				gs_vstScheduler[j].viExecuteCount[i] = 0;
			}
			kInitializeList(&(gs_vstScheduler[j].stWaitList));
			kInitializeSpinLock(&(gs_vstScheduler[j].stSpinLock));
		}
	}

	pstTask = kAllocateTCB();
	gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstTask;

	pstTask->bAPICID = bCurrentAPICID;
	pstTask->bAffinity = bCurrentAPICID;

	if (bCurrentAPICID == 0)
	{
		pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
	}
	else
	{
		pstTask->qwFlags = TASK_FLAGS_LOWEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM |
			TASK_FLAGS_IDLE;
	}

	pstTask->qwParentProcessID = pstTask->stLink.qwID;
	pstTask->pvMemoryAddress = (void *)0x100000;
	pstTask->qwMemorySize = 0x500000;
	pstTask->pvStackAddress = (void *)0x600000;
	pstTask->qwStackSize = 0x100000;

	// Init for calc processor rate
	gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask = 0;
	gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 0;

	// Init for FPU
	gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID = TASK_INVALIDID;
}

void kSetRunningTask(BYTE bAPICID, TCB* pstTask)
{
	kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	gs_vstScheduler[bAPICID].pstRunningTask = pstTask;
	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
}

TCB* kGetRunningTask(BYTE bAPICID)
{
	TCB* pstRunningTask;

	kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	pstRunningTask = gs_vstScheduler[bAPICID].pstRunningTask;
	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	return pstRunningTask;
}

static TCB* kGetNextTaskToRun(BYTE bAPICID)
{
	TCB* pstTarget = NULL;
	int iTaskCount, i, j;

	for (j = 0; j < 2; j++)
	{
		for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++)
		{
			iTaskCount = kGetListCount(&(gs_vstScheduler[bAPICID].vstReadyList[i]));
			// Return current priority task if 
			// task count is more than execute count
			if (gs_vstScheduler[bAPICID].viExecuteCount[i] < iTaskCount)
			{
				pstTarget = (TCB*)kRemoveListFromHeader(
						&(gs_vstScheduler[bAPICID].vstReadyList[i]));
				gs_vstScheduler[bAPICID].viExecuteCount[i]++;
				break;
			}
			else
			{
				gs_vstScheduler[bAPICID].viExecuteCount[i] = 0;
			}
		}

		if (pstTarget != NULL)
			break;
	}

	return pstTarget;
}

static BOOL kAddTaskToReadyList(BYTE bAPICID, TCB* pstTask)
{
	BYTE bPriority;

	bPriority = GETPRIORITY(pstTask->qwFlags);
	if (bPriority == TASK_FLAGS_WAIT)
	{
		kAddListToTail(&(gs_vstScheduler[bAPICID].stWaitList), pstTask);
		return TRUE;
	}
	else if (bPriority >= TASK_MAXREADYLISTCOUNT)
		return FALSE;

	kAddListToTail(&(gs_vstScheduler[bAPICID].vstReadyList[bPriority]), pstTask);
	return TRUE;
}

static TCB* kRemoveTaskFromReadyList(BYTE bAPICID, QWORD qwTaskID)
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
	if (bPriority >= TASK_MAXREADYLISTCOUNT)
		return NULL;

	pstTarget = kRemoveList(&(gs_vstScheduler[bAPICID].vstReadyList[bPriority]), qwTaskID);
	return pstTarget;
}

/*
 * Return Scheduler ID that includes the Task.
 * And lock the Spin Lock. So, Caller must unlock when return value is TRUE
 */
static BOOL kFindSchedulerOfTaskAndLock(QWORD qwTaskID, BYTE* pbAPICID)
{
	TCB *pstTarget;
	BYTE bAPICID;

	while (1)
	{
		pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
		if ((pstTarget == NULL) || (pstTarget->stLink.qwID != qwTaskID))
			return FALSE;

		bAPICID = pstTarget->bAPICID;
		kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

		// check after get spin lock
		pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
		if (pstTarget->bAPICID == bAPICID)
			break;

		kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	}

	*pbAPICID = bAPICID;
	return TRUE;
}


BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority)
{
	TCB* pstTarget;
	BYTE bAPICID;

	if (bPriority > TASK_MAXREADYLISTCOUNT)
		return FALSE;

	if (kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE)
		return FALSE;

	// Just change priority if current running task
	pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
	if (pstTarget->stLink.qwID == qwTaskID)
	{
		SETPRIORITY(pstTarget->qwFlags, bPriority);
	}
	// Seek from ready list and change if not current running task
	else
	{
		pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);
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
			kAddTaskToReadyList(bAPICID, pstTarget);
		}
	}

	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

	return TRUE;
}

BOOL kSchedule(void)
{
	TCB* pstRunningTask, * pstNextTask;
	BOOL bPreviousInterrupt;
	BYTE bCurrentAPICID;

	bPreviousInterrupt = kSetInterruptFlag(FALSE);

	bCurrentAPICID = kGetAPICID();

	if (kGetReadyTaskCount(bCurrentAPICID) < 1)
	{
		kSetInterruptFlag(bPreviousInterrupt);
		return FALSE;
	}

	kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	// Get Next Task
	pstNextTask = kGetNextTaskToRun(bCurrentAPICID);
	if (pstNextTask == NULL)
	{
		kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		kSetInterruptFlag(bPreviousInterrupt);
		return FALSE;
	}

	// Add Current Running Task to Ready List
	pstRunningTask = gs_vstScheduler[bCurrentAPICID].pstRunningTask;
	gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstNextTask;

	if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
	{
		gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask +=
			TASK_PROCESSORTIME - gs_vstScheduler[bCurrentAPICID].iProcessorTime;
	}

	// Set TS if next task is not last fpu used task
	if (gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
	{
		kSetTS();
	}
	else
	{
		kClearTS();
	}

	gs_vstScheduler[bCurrentAPICID].iProcessorTime = TASK_PROCESSORTIME;

	if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)
	{
		kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstRunningTask);
		kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		kSwitchContext(NULL, &(pstNextTask->stContext));
	}
	else
	{
		kAddTaskToReadyList(bCurrentAPICID, pstRunningTask);
		kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
	}

	kSetInterruptFlag(bPreviousInterrupt);
	return TRUE;
}

// call if only interrupt/exception is occurred
BOOL kScheduleInInterrupt(void)
{
	TCB* pstRunningTask, * pstNextTask;
	char* pcContextAddress;
	BYTE bCurrentAPICID;
	QWORD qwISTStartAddress;

	bCurrentAPICID = kGetAPICID();

	kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	pstNextTask = kGetNextTaskToRun(bCurrentAPICID);
	if (pstNextTask == NULL)
	{
		kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		return FALSE;
	}

	// task switch
	// Overwrites the context savedby the interrupt handler with another context

	qwISTStartAddress = IST_STARTADDRESS + IST_SIZE -
		(IST_SIZE / MAXPROCESSORCOUNT * bCurrentAPICID);

	pcContextAddress = (char*)qwISTStartAddress - sizeof(CONTEXT);

	pstRunningTask = gs_vstScheduler[bCurrentAPICID].pstRunningTask;
	gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstNextTask;

	if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE)
	{
		gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
	}

	if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)
	{
		kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstRunningTask);
	}
	else
	{
		kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
	}

	// Set TS if next task is not last fpu used task
	if (gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
	{
		kSetTS();
	}
	else
	{
		kClearTS();
	}

	kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

	if ((pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) != TASK_FLAGS_ENDTASK)
	{
		kAddTaskToSchedulerWithLoadBalancing(pstRunningTask);
	}

	gs_vstScheduler[bCurrentAPICID].iProcessorTime = TASK_PROCESSORTIME;
	return TRUE;
}

void kDecreaseProcessorTime(BYTE bAPICID)
{
	if (gs_vstScheduler[bAPICID].iProcessorTime > 0)
	{
		gs_vstScheduler[bAPICID].iProcessorTime--;
	}
}

BOOL kIsProcessorTimeExpired(BYTE bAPICID)
{
	if (gs_vstScheduler[bAPICID].iProcessorTime <= 0)
		return TRUE;
	return FALSE;
}

BOOL kEndTask(QWORD qwTaskID)
{
	TCB* pstTarget;
	BYTE bPriority;
	BYTE bAPICID;

	if (kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE)
		return FALSE;

	pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
	if (pstTarget->stLink.qwID == qwTaskID)
	{
		// set End Task bit and switch task
		// if current running task
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

		kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

		if (kGetAPICID() == bAPICID)
		{
			kSchedule();

			// Never reached..
			while (1);
		}

		return TRUE;
	}

	pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);
	if (pstTarget == NULL)
	{
		pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
		if (pstTarget != NULL)
		{
			pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
			SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
		}
		kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
		return TRUE;
	}

	pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
	SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
	kAddListToTail(&(gs_vstScheduler[bAPICID].stWaitList), pstTarget);
	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	return TRUE;
}

void kExitTask(void)
{
	kEndTask(gs_vstScheduler[kGetAPICID()].pstRunningTask->stLink.qwID);
}

int kGetReadyTaskCount(BYTE bAPICID)
{
	int iTotalCount = 0;
	int i;

	kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));

	for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++)
	{
		iTotalCount += kGetListCount(&(gs_vstScheduler[bAPICID].vstReadyList[i]));
	}

	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	return iTotalCount;
}

int kGetTaskCount(BYTE bAPICID)
{
	int iTotalCount;

	iTotalCount = kGetReadyTaskCount(bAPICID);

	kLockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	iTotalCount += kGetListCount(&(gs_vstScheduler[bAPICID].stWaitList)) + 1;
	kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
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

QWORD kGetProcessorLoad(BYTE bAPICID)
{
	return gs_vstScheduler[bAPICID].qwProcessorLoad;
}


static TCB* kGetProcessByThread(TCB* pstThread)
{
	TCB* pstProcess;

	if (pstThread->qwFlags & TASK_FLAGS_PROCESS)
		return pstThread;

	pstProcess = kGetTCBInTCBPool(GETTCBOFFSET(pstThread->qwParentProcessID));

	if ((pstProcess == NULL) || (pstProcess->stLink.qwID != pstThread->qwParentProcessID))
		return NULL;

	return pstProcess;
}

// Scheduler Load Balancing with number of task of each Scheduler
void kAddTaskToSchedulerWithLoadBalancing(TCB* pstTask)
{
	BYTE bCurrentAPICID;
	BYTE bTargetAPICID;

	bCurrentAPICID = pstTask->bAPICID;

	if ((gs_vstScheduler[bCurrentAPICID].bUseLoadBalancing == TRUE) &&
			(pstTask->bAffinity == TASK_LOADBALANCINGID))
	{
		bTargetAPICID = kFindSchedulerOfMinimumTaskCount(pstTask);
	}
	else if ((pstTask->bAffinity != bCurrentAPICID) &&
			(pstTask->bAffinity != TASK_LOADBALANCINGID))
	{
		bTargetAPICID = pstTask->bAffinity;
	}
	else
	{
		bTargetAPICID = bCurrentAPICID;
	}

	kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
	if ((bCurrentAPICID != bTargetAPICID) && (pstTask->stLink.qwID ==
				gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID))
	{
		kClearTS();
		kSaveFPUContext(pstTask->vqwFPUContext);
		gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID = TASK_INVALIDID;
	}
	kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	kLockForSpinLock(&(gs_vstScheduler[bTargetAPICID].stSpinLock));
	pstTask->bAPICID = bTargetAPICID;
	kAddTaskToReadyList(bTargetAPICID, pstTask);
	kUnlockForSpinLock(&(gs_vstScheduler[bTargetAPICID].stSpinLock));
}

static BYTE kFindSchedulerOfMinimumTaskCount(const TCB* pstTask)
{
	BYTE bPriority, bMinCoreIndex;
	int iCurrentTaskCount, iMinTaskCount;
	int iTempTaskCount, iProcessorCount;
	int i;

	if ((iProcessorCount = kGetProcessorCount()) == 1)
		return pstTask->bAPICID;

	bPriority = GETPRIORITY(pstTask->qwFlags);

	// get number of task which same priority with the task from scheduler
	iCurrentTaskCount = kGetListCount(&(gs_vstScheduler[pstTask->bAPICID].
				vstReadyList[bPriority]));

	// check other cores
	iMinTaskCount = TASK_MAXCOUNT;
	bMinCoreIndex = pstTask->bAPICID;
	for (i = 0; i < iProcessorCount; i++)
	{
		if (i == pstTask->bAPICID)
			continue;

		iTempTaskCount = kGetListCount(&(gs_vstScheduler[i].vstReadyList[
					bPriority]));
		
		if ((iTempTaskCount + 2 <= iCurrentTaskCount) &&
				(iTempTaskCount < iMinTaskCount))
		{
			bMinCoreIndex = i;
			iMinTaskCount = iTempTaskCount;
		}
	}

	return bMinCoreIndex;
}

BYTE kSetTaskLoadBalancing(BYTE bAPICID, BOOL bUseLoadBalancing)
{
	gs_vstScheduler[bAPICID].bUseLoadBalancing = bUseLoadBalancing;
}

BOOL kChangeProcessorAffinity(QWORD qwTaskID, BYTE bAffinity)
{
	TCB* pstTarget;
	BYTE bAPICID;

	if (kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE)
		return FALSE;

	pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
	if (pstTarget->stLink.qwID == qwTaskID)
	{
		pstTarget->bAffinity = bAffinity;
		kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
	}
	else
	{
		pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);
		if (pstTarget == NULL)
		{
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if (pstTarget != NULL)
				pstTarget->bAffinity = bAffinity;
		}
		else
		{
			pstTarget->bAffinity = bAffinity;
		}
		kUnlockForSpinLock(&(gs_vstScheduler[bAPICID].stSpinLock));
		kAddTaskToSchedulerWithLoadBalancing(pstTarget);
	}

	return TRUE;
}

/*
 * Idle Task Functions
 */
void kIdleTask(void)
{
	TCB* pstTask, * pstChildThread, * pstProcess;
	QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
	QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
	QWORD qwTaskID, qwChildThreadID;
	int i, iCount;
	void* pstThreadLink;
	BYTE bCurrentAPICID, bProcessAPICID;

	bCurrentAPICID = kGetAPICID();

	qwLastSpendTickInIdleTask = gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask;
	qwLastMeasureTickCount = kGetTickCount();

	while (1)
	{
		qwCurrentMeasureTickCount = kGetTickCount();
		qwCurrentSpendTickInIdleTask = gs_vstScheduler[bCurrentAPICID].
			qwSpendProcessorTimeInIdleTask;

		if (qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0)
		{
			gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 0;
		}
		else
		{
			gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 100 -
				(qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) *
				100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount);
		}

		qwLastMeasureTickCount = qwCurrentMeasureTickCount;
		qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

		// Processor Sleep
		kHaltProcessorByLoad(bCurrentAPICID);

		if (kGetListCount(&(gs_vstScheduler[bCurrentAPICID].stWaitList)) > 0)
		{
			while (1)
			{
				kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
				pstTask = kRemoveListFromHeader(&(
							gs_vstScheduler[bCurrentAPICID].stWaitList));
				kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
				if (pstTask == NULL)
				{
					break;
				}

				if (pstTask->qwFlags & TASK_FLAGS_PROCESS)
				{
					// Exit Thread
					iCount = kGetListCount(&(pstTask->stChildThreadList));
					for (i = 0; i < iCount; i++)
					{
						kLockForSpinLock(&(
									gs_vstScheduler[bCurrentAPICID].stSpinLock));
						pstThreadLink = (TCB*)kRemoveListFromHeader(
								&(pstTask->stChildThreadList));
						if (pstThreadLink == NULL)
						{
							kUnlockForSpinLock(&(
										gs_vstScheduler[bCurrentAPICID].stSpinLock));
							break;
						}

						// Add to child thread list again to remove childself from list
						pstChildThread = GETTCBFROMTHREADLINK(pstThreadLink);
						kAddListToTail(&(pstTask->stChildThreadList),
								&(pstChildThread->stThreadLink));

						qwChildThreadID = pstChildThread->stLink.qwID;

						kUnlockForSpinLock(&(
									gs_vstScheduler[bCurrentAPICID].stSpinLock));

						kEndTask(qwChildThreadID);
					}

					if (kGetListCount(&(pstTask->stChildThreadList)) > 0)
					{
						kLockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
						kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstTask);
						kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
						continue;
					}
					else
					{
						// free user level memory
						if (pstTask->qwFlags & TASK_FLAGS_USERLEVEL)
							kFreeMemory(pstTask->pvMemoryAddress);
					}
				}
				else if (pstTask->qwFlags & TASK_FLAGS_THREAD)
				{
					pstProcess = kGetProcessByThread(pstTask);
					if (pstProcess != NULL)
					{
						if (kFindSchedulerOfTaskAndLock(pstProcess->stLink.qwID,
									&bProcessAPICID) == TRUE)
						{
							kRemoveList(&(pstProcess->stChildThreadList),
									pstTask->stLink.qwID);
							kUnlockForSpinLock(&(gs_vstScheduler[bProcessAPICID].
										stSpinLock));
						}
					}
				}

				qwTaskID = pstTask->stLink.qwID;
				kFreeMemory(pstTask->pvStackAddress);
				kFreeTCB(qwTaskID);
				kPrintf("IDLE: Task ID[0x%q] is completely ended.\n", qwTaskID);
			}
		}

		kSchedule();
	}
}

void kHaltProcessorByLoad(BYTE bAPICID)
{
	if (gs_vstScheduler[bAPICID].qwProcessorLoad < 40)
	{
		kHlt();
		kHlt();
		kHlt();
	}
	else if (gs_vstScheduler[bAPICID].qwProcessorLoad < 80)
	{
		kHlt();
		kHlt();
	}
	else if (gs_vstScheduler[bAPICID].qwProcessorLoad < 95)
	{
		kHlt();
	}
}

QWORD kGetLastFPUUsedTaskID(BYTE bAPICID)
{
	return gs_vstScheduler[bAPICID].qwLastFPUUsedTaskID;
}

void kSetLastFPUUsedTaskID(BYTE bAPICID, QWORD qwTaskID)
{
	gs_vstScheduler[bAPICID].qwLastFPUUsedTaskID = qwTaskID;
}
