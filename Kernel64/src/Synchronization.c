#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"
#include "AssemblyUtility.h"
#include "MultiProcessor.h"

#if 0

BOOL kLockForSystemData(void)
{
	return kSetInterruptFlag(FALSE);
}

void kUnlockForSystemData(BOOL bInterruptFlag)
{
	kSetInterruptFlag(bInterruptFlag);
}

#endif

// spin lock

void kInitializeSpinLock(SPINLOCK* pstSpinLock)
{
	pstSpinLock->bLockFlag = FALSE;
	pstSpinLock->dwLockCount = 0;
	pstSpinLock->bAPICID = 0xFF;
	pstSpinLock->bInterruptFlag = FALSE;
}

void kLockForSpinLock(SPINLOCK* pstSpinLock)
{
	BOOL bInterruptFlag;

	bInterruptFlag = kSetInterruptFlag(FALSE);

	if (kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE)
	{	// already lock
		if (pstSpinLock->bAPICID == kGetAPICID())
		{	// duplicate lock for owner
			pstSpinLock->dwLockCount++;
			return;
		}

		while (kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE)
		{
			while (pstSpinLock->bLockFlag == TRUE)
				kPause();
		}
	}

	pstSpinLock->dwLockCount = 1;
	pstSpinLock->bAPICID = kGetAPICID();
	pstSpinLock->bInterruptFlag = bInterruptFlag;
}

void kUnlockForSpinLock(SPINLOCK* pstSpinLock)
{
	BOOL bInterruptFlag;

	bInterruptFlag = kSetInterruptFlag(FALSE);
	
	if ((pstSpinLock->bLockFlag == FALSE) ||
			(pstSpinLock->bAPICID != kGetAPICID()))
	{
		kSetInterruptFlag(bInterruptFlag);
		return;
	}

	if (pstSpinLock->dwLockCount > 1)
	{
		pstSpinLock->dwLockCount--;
		return;
	}

	// save and use interrupt flag to avoid race condition
	bInterruptFlag = pstSpinLock->bInterruptFlag;
	pstSpinLock->bAPICID = 0xFF;
	pstSpinLock->dwLockCount = 0;
	pstSpinLock->bInterruptFlag = FALSE;
	pstSpinLock->bLockFlag = FALSE;
	kSetInterruptFlag(bInterruptFlag);
}


// mutex

void kInitializeMutex(MUTEX* pstMutex)
{
	pstMutex->bLockFlag = FALSE;
	pstMutex->dwLockCount = 0;
	pstMutex->qwTaskID = TASK_INVALIDID;
}

void kLock(MUTEX* pstMutex)
{
	BYTE bCurrentAPICID;
	BOOL bInterruptFlag;

	bInterruptFlag = kSetInterruptFlag(FALSE);
	bCurrentAPICID = kGetAPICID();

	if (kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE)
	{
		// mutex is already locked

		if (pstMutex->qwTaskID == kGetRunningTask(bCurrentAPICID)->stLink.qwID)
		{
			pstMutex->dwLockCount++;
			kSetInterruptFlag(bInterruptFlag);
			return;
		}

		while (kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE)
		{
			kSchedule();
		}
	}

	pstMutex->dwLockCount = 1;
	pstMutex->qwTaskID = kGetRunningTask(bCurrentAPICID)->stLink.qwID;
	kSetInterruptFlag(bInterruptFlag);
}

void kUnlock(MUTEX* pstMutex)
{
	BOOL bInterruptFlag;

	bInterruptFlag = kSetInterruptFlag(FALSE);

	if ((pstMutex->bLockFlag == FALSE) ||
			(pstMutex->qwTaskID != kGetRunningTask(kGetAPICID())->stLink.qwID))
	{
		kSetInterruptFlag(bInterruptFlag);
		return;
	}

	if (pstMutex->dwLockCount > 1)
	{
		pstMutex->dwLockCount--;
	}
	else
	{
		pstMutex->qwTaskID = TASK_INVALIDID;
		pstMutex->dwLockCount = 0;
		pstMutex->bLockFlag = FALSE;
	}
	kSetInterruptFlag(bInterruptFlag);
}
