#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"
#include "AssemblyUtility.h"

BOOL kLockForSystemData(void)
{
	return kSetInterruptFlag(FALSE);
}

void kUnlockForSystemData(BOOL bInterruptFlag)
{
	kSetInterruptFlag(bInterruptFlag);
}

void kInitializeMutex(MUTEX* pstMutex)
{
	pstMutex->bLockFlag = FALSE;
	pstMutex->dwLockCount = 0;
	pstMutex->qwTaskID = TASK_INVALIDID;
}

void kLock(MUTEX* pstMutex)
{
	if (kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE)
	{
		// if hte mutex is locked

		if (pstMutex->qwTaskID == kGetRunningTask()->stLink.qwID)
		{
			pstMutex->dwLockCount++;
			return;
		}

		while (kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE)
		{
			kSchedule();
		}
	}

	pstMutex->dwLockCount = 1;
	pstMutex->qwTaskID = kGetRunningTask()->stLink.qwID;
}

void kUnlock(MUTEX* pstMutex)
{
	if ((pstMutex->bLockFlag == FALSE) ||
			(pstMutex->qwTaskID != kGetRunningTask()->stLink.qwID))
	{
		return;
	}

	if (pstMutex->dwLockCount > 1)
	{
		pstMutex->dwLockCount--;
		return;
	}

	pstMutex->qwTaskID = TASK_INVALIDID;
	pstMutex->dwLockCount = 0;
	pstMutex->bLockFlag = FALSE;
}
