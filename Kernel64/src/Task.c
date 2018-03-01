#include "Task.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Descriptor.h"
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

TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize,
		QWORD qwEntryPointAddress)
{
	TCB* pstTask, * pstProcess;
	void* pvStackAddress;

	kLockForSpinLock(&(gs_stScheduler.stSpinLock));
	pstTask = kAllocateTCB();
	if (pstTask == NULL)
	{
		kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
		return NULL;
	}

	pstProcess = kGetProcessByThread(kGetRunningTask());
	if (pstProcess == NULL)
	{
		kFreeTCB(pstTask->stLink.qwID);
		kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
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

	kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));

	pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE *
				GETTCBOFFSET(pstTask->stLink.qwID)));

	kSetupTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress,
			TASK_STACKSIZE);

	kInitializeList(&(pstTask->stChildThreadList));

	pstTask->bFPUUsed = FALSE;

	kLockForSpinLock(&(gs_stScheduler.stSpinLock));
	kAddTaskToReadyList(pstTask);
	kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));

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
	TCB* pstTask;

	kInitializeTCBPool();

	for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++)
	{
		kInitializeList(&(gs_stScheduler.vstReadyList[i]));
		gs_stScheduler.viExecuteCount[i] = 0;
	}
	kInitializeList(&(gs_stScheduler.stWaitList));

	pstTask = kAllocateTCB();
	gs_stScheduler.pstRunningTask = pstTask;
	pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
	pstTask->qwParentProcessID = pstTask->stLink.qwID;
	pstTask->pvMemoryAddress = (void *)0x100000;
	pstTask->qwMemorySize = 0x500000;
	pstTask->pvStackAddress = (void *)0x600000;
	pstTask->qwStackSize = 0x100000;

	// Init for calc processor rate
	gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
	gs_stScheduler.qwProcessorLoad = 0;

	// Init for FPU
	gs_stScheduler.qwLastFPUUsedTaskID = TASK_INVALIDID;

	// Init Spin Lock
	kInitializeSpinLock(&(gs_stScheduler.stSpinLock));
}

void kSetRunningTask(TCB* pstTask)
{
	kLockForSpinLock(&(gs_stScheduler.stSpinLock));
	gs_stScheduler.pstRunningTask = pstTask;
	kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
}

TCB* kGetRunningTask(void)
{
	TCB* pstRunningTask;

	kLockForSpinLock(&(gs_stScheduler.stSpinLock));
	pstRunningTask = gs_stScheduler.pstRunningTask;
	kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
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

	if (bPriority > TASK_MAXREADYLISTCOUNT)
		return FALSE;

	kLockForSpinLock(&(gs_stScheduler.stSpinLock));

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

	kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));

	return TRUE;
}

void kSchedule(void)
{
	TCB* pstRunningTask, * pstNextTask;
	BOOL bPreviousInterrupt;

	if (kGetReadyTaskCount() < 1)
		return;

	bPreviousInterrupt = kSetInterruptFlag(FALSE);
	kLockForSpinLock(&(gs_stScheduler.stSpinLock));

	// Get Next Task
	pstNextTask = kGetNextTaskToRun();
	if (pstNextTask == NULL)
	{
		kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
		kSetInterruptFlag(bPreviousInterrupt);
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

	// Set TS if next task is not last fpu used task
	if (gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
	{
		kSetTS();
	}
	else
	{
		kClearTS();
	}

	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

	if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)
	{
		kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
		kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
		kSwitchContext(NULL, &(pstNextTask->stContext));
	}
	else
	{
		kAddTaskToReadyList(pstRunningTask);
		kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
		kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
	}

	kSetInterruptFlag(bPreviousInterrupt);
}

// call if only interrupt/exception is occurred
BOOL kScheduleInInterrupt(void)
{
	TCB* pstRunningTask, * pstNextTask;
	char* pcContextAddress;

	kLockForSpinLock(&(gs_stScheduler.stSpinLock));

	pstNextTask = kGetNextTaskToRun();
	if (pstNextTask == NULL)
	{
		kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
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

	kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));

	// Set TS if next task is not last fpu used task
	if (gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
	{
		kSetTS();
	}
	else
	{
		kClearTS();
	}

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

	kLockForSpinLock(&(gs_stScheduler.stSpinLock));

	pstTarget = gs_stScheduler.pstRunningTask;
	if (pstTarget->stLink.qwID == qwTaskID)
	{
		// set End Task bit and switch task
		// if current running task
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

		kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
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
			kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
			return FALSE;
		}

		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
		kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
	}
	kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
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

	kLockForSpinLock(&(gs_stScheduler.stSpinLock));

	for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++)
	{
		iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
	}

	kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
	return iTotalCount;
}

int kGetTaskCount(void)
{
	int iTotalCount;

	iTotalCount = kGetReadyTaskCount();

	kLockForSpinLock(&(gs_stScheduler.stSpinLock));
	iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;
	kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
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


/*
 * Idle Task Functions
 */
void kIdleTask(void)
{
	TCB* pstTask, * pstChildThread, * pstProcess;
	QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
	QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
	QWORD qwTaskID;
	int i, iCount;
	void* pstThreadLink;

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
				kLockForSpinLock(&(gs_stScheduler.stSpinLock));
				pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
				if (pstTask == NULL)
				{
					kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
					break;
				}

				if (pstTask->qwFlags & TASK_FLAGS_PROCESS)
				{
					// Exit Thread
					iCount = kGetListCount(&(pstTask->stChildThreadList));
					for (i = 0; i < iCount; i++)
					{
						pstThreadLink = (TCB*)kRemoveListFromHeader(
								&(pstTask->stChildThreadList));
						if (pstThreadLink == NULL)
							break;

						// Add to child thread list again to remove childself from list
						pstChildThread = GETTCBFROMTHREADLINK(pstThreadLink);
						kAddListToTail(&(pstTask->stChildThreadList),
								&(pstChildThread->stThreadLink));

						kEndTask(pstChildThread->stLink.qwID);
					}

					if (kGetListCount(&(pstTask->stChildThreadList)) > 0)
					{
						kAddListToTail(&(gs_stScheduler.stWaitList), pstTask);
						kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
						continue;
					}
					else
					{
						// TODO
					}
				}
				else if (pstTask->qwFlags & TASK_FLAGS_THREAD)
				{
					pstProcess = kGetProcessByThread(pstTask);
					if (pstProcess != NULL)
					{
						kRemoveList(&(pstProcess->stChildThreadList),
								pstTask->stLink.qwID);
					}
				}

				qwTaskID = pstTask->stLink.qwID;
				kFreeTCB(qwTaskID);
				kUnlockForSpinLock(&(gs_stScheduler.stSpinLock));
				
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

QWORD kGetLastFPUUsedTaskID(void)
{
	return gs_stScheduler.qwLastFPUUsedTaskID;
}

void kSetLastFPUUsedTaskID(QWORD qwTaskID)
{
	gs_stScheduler.qwLastFPUUsedTaskID = qwTaskID;
}
