#include "DynamicMemory.h"
#include "Utility.h"

static DYNAMICMEMORY gs_stDynamicMemory;

void kInitializeDynamicMemory(void)
{
	QWORD qwDynamicMemorySize;
	int i, j;
	BYTE* pbCurrentBitmapPosition;
	int iBlockCountOfLevel, iMetaBlockCount;

	qwDynamicMemorySize = kCalculateDynamicMemorySize();
	iMetaBlockCount = kCalculateMetaBlockCount(qwDynamicMemorySize);

	gs_stDynamicMemory.iBlockCountOfSmallestBlock =
		(qwDynamicMemorySize / DYNAMICMEMORY_MIN_SIZE) - iMetaBlockCount;

	for (i = 0; (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) > 0; i++)
	{
		;
	}
	gs_stDynamicMemory.iMaxLevelCount = i;

	gs_stDynamicMemory.pbAllocatedBlockListIndex = (BYTE*)DYNAMICMEMORY_START_ADDRESS;
	for (i = 0; i < gs_stDynamicMemory.iBlockCountOfSmallestBlock; i++)
	{
		gs_stDynamicMemory.pbAllocatedBlockListIndex[i] = 0xFF;
	}

	gs_stDynamicMemory.pstBitmapOfLevel = (BITMAP*)(DYNAMICMEMORY_START_ADDRESS +
			(sizeof(BYTE) * gs_stDynamicMemory.iBlockCountOfSmallestBlock));
	pbCurrentBitmapPosition = ((BYTE*)gs_stDynamicMemory.pstBitmapOfLevel) +
		(sizeof(BITMAP) * gs_stDynamicMemory.iMaxLevelCount);
	
	for (j = 0; j < gs_stDynamicMemory.iMaxLevelCount; j++)
	{
		gs_stDynamicMemory.pstBitmapOfLevel[j].pbBitmap = pbCurrentBitmapPosition;
		gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 0;
		iBlockCountOfLevel = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> j;

		for (i = 0; i < iBlockCountOfLevel / 8; i++)
		{
			*pbCurrentBitmapPosition = 0x00;
			pbCurrentBitmapPosition++;
		}

		if ((iBlockCountOfLevel % 8) != 0)
		{
			*pbCurrentBitmapPosition = 0x00;
			i = iBlockCountOfLevel % 8;
			if ((i % 2) == 1)
			{
				*pbCurrentBitmapPosition |= (DYNAMICMEMORY_EXIST << (i - 1));
				gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 1;
			}
			pbCurrentBitmapPosition++;
		}
	}

	gs_stDynamicMemory.qwStartAddress = DYNAMICMEMORY_START_ADDRESS +
		iMetaBlockCount * DYNAMICMEMORY_MIN_SIZE;
	gs_stDynamicMemory.qwEndAddress = kCalculateDynamicMemorySize() +
		DYNAMICMEMORY_START_ADDRESS;
	gs_stDynamicMemory.qwUsedSize = 0;
	kInitializeSpinLock(&(gs_stDynamicMemory.stSpinLock));
}

static QWORD kCalculateDynamicMemorySize(void)
{
	QWORD qwRAMSize;

	qwRAMSize = (kGetTotalRAMSize() * 1024 * 1024);
	if (qwRAMSize > (QWORD)3 * 1024 * 1024 * 1024)
		qwRAMSize = (QWORD)3 * 1024 * 1024 * 1024;
	return qwRAMSize - DYNAMICMEMORY_START_ADDRESS;
}

static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize)
{
	long lBlockCountOfSmallestBlock;
	DWORD dwSizeOfAllocatedBlockListIndex;
	DWORD dwSizeOfBitmap;
	long i;

	lBlockCountOfSmallestBlock = qwDynamicRAMSize / DYNAMICMEMORY_MIN_SIZE;
	dwSizeOfAllocatedBlockListIndex = lBlockCountOfSmallestBlock * sizeof(BYTE);

	dwSizeOfBitmap = 0;
	for (i = 0; (lBlockCountOfSmallestBlock >> i) > 0; i++)
	{
		dwSizeOfBitmap += sizeof(BITMAP);
		dwSizeOfBitmap += ((lBlockCountOfSmallestBlock >> i) + 7) / 8;
	}

	return (dwSizeOfAllocatedBlockListIndex + dwSizeOfBitmap +
			DYNAMICMEMORY_MIN_SIZE - 1) / DYNAMICMEMORY_MIN_SIZE;
}

void* kAllocateMemory(QWORD qwSize)
{
	QWORD qwAlignedSize;
	QWORD qwRelativeAddress;
	long lOffset;
	int iSizeArrayOffset;
	int iIndexOfBlockList;

	qwAlignedSize = kGetBuddyBlockSize(qwSize);
	if (qwAlignedSize == 0)
		return NULL;

	if (gs_stDynamicMemory.qwStartAddress + gs_stDynamicMemory.qwUsedSize +
			qwAlignedSize > gs_stDynamicMemory.qwEndAddress)
		return NULL;

	lOffset = kAllocationBuddyBlock(qwAlignedSize);
	if (lOffset == -1)
		return NULL;

	iIndexOfBlockList = kGetBlockListIndexOfMatchSize(qwAlignedSize);

	qwRelativeAddress = qwAlignedSize * lOffset;
	iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

	gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] =
		(BYTE)iIndexOfBlockList;
	gs_stDynamicMemory.qwUsedSize += qwAlignedSize;
	
	return (void*)(qwRelativeAddress + gs_stDynamicMemory.qwStartAddress);
}

static QWORD kGetBuddyBlockSize(QWORD qwSize)
{
	long i;
	
	for (i = 0; i < gs_stDynamicMemory.iMaxLevelCount; i++)
	{
		if (qwSize <= (DYNAMICMEMORY_MIN_SIZE << i))
		{
			return (DYNAMICMEMORY_MIN_SIZE << i);
		}
	}
	return 0;
}

static int kAllocationBuddyBlock(QWORD qwAlignedSize)
{
	int iBlockListIndex, iFreeOffset;
	int i;

	iBlockListIndex = kGetBlockListIndexOfMatchSize(qwAlignedSize);
	if (iBlockListIndex == -1)
		return -1;

	kLockForSpinLock(&(gs_stDynamicMemory.stSpinLock));

	for (i = iBlockListIndex; i < gs_stDynamicMemory.iMaxLevelCount; i++)
	{
		iFreeOffset = kFindFreeBlockInBitmap(i);
		if (iFreeOffset != -1)
		{
			break;
		}
	}

	if (iFreeOffset == -1)
	{
		kUnlockForSpinLock(&(gs_stDynamicMemory.stSpinLock));
		return -1;
	}

	kSetFlagInBitmap(i, iFreeOffset, DYNAMICMEMORY_EMPTY);

	if (i > iBlockListIndex)
	{
		for (i = i - 1; i >= iBlockListIndex; i--)
		{
			kSetFlagInBitmap(i, iFreeOffset * 2, DYNAMICMEMORY_EMPTY);
			kSetFlagInBitmap(i, iFreeOffset * 2 + 1, DYNAMICMEMORY_EXIST);
			iFreeOffset = iFreeOffset * 2;
		}
	}
	
	kUnlockForSpinLock(&(gs_stDynamicMemory.stSpinLock));
	return iFreeOffset;
}

static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize)
{
	int i;

	for (i = 0; i < gs_stDynamicMemory.iMaxLevelCount; i++)
	{
		if (qwAlignedSize <= (DYNAMICMEMORY_MIN_SIZE << i))
			return i;
	}
	return -1;
}

static int kFindFreeBlockInBitmap(int iBlockListIndex)
{
	int i, iMaxCount;
	BYTE* pbBitmap;
	QWORD* pqwBitmap;

	if (gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount == 0)
		return -1;

	iMaxCount = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> iBlockListIndex;
	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;

	for (i = 0; i < iMaxCount; )
	{ 
		// check 64 bit at once
		if (((iMaxCount - i) / 64) > 0)
		{
			pqwBitmap = (QWORD*)&(pbBitmap[i / 8]);
			// jump if all bits are 0
			if (*pqwBitmap == 0)
			{
				i += 64;
				continue;
			}
		}

		if ((pbBitmap[i / 8] & (DYNAMICMEMORY_EXIST << (i % 8))) != 0)
			return i;

		i++;
	}
	return -1;
}

static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag)
{
	BYTE* pbBitmap;

	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;
	if (bFlag == DYNAMICMEMORY_EXIST)
	{
		if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) == 0)
		{
			gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount++;
		}
		pbBitmap[iOffset / 8] |= (0x01 << (iOffset % 8));
	}
	else
	{
		if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0)
		{
			gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount--;
		}
		pbBitmap[iOffset / 8] &= ~(0x01 << (iOffset % 8));
	}
}

BOOL kFreeMemory(void* pvAddress)
{
	QWORD qwRelativeAddress;
	int iSizeArrayOffset;
	QWORD qwBlockSize;
	int iBlockListIndex;
	int iBitmapOffset;

	if (pvAddress == NULL)
		return FALSE;

	qwRelativeAddress = ((QWORD)pvAddress) - gs_stDynamicMemory.qwStartAddress;
	iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

	if (gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] == 0xFF)
	{
		return FALSE;
	}

	iBlockListIndex = (int)gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset];
	gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] = 0xFF;
	qwBlockSize = DYNAMICMEMORY_MIN_SIZE << iBlockListIndex;

	iBitmapOffset = qwRelativeAddress / qwBlockSize;
	if (kFreeBuddyBlock(iBlockListIndex, iBitmapOffset) == TRUE)
	{
		gs_stDynamicMemory.qwUsedSize -= qwBlockSize;
		return TRUE;
	}
	return FALSE;
}

static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset)
{
	int iBuddyBlockOffset;
	int i;
	BOOL bFlag;

	kLockForSpinLock(&(gs_stDynamicMemory.stSpinLock));

	for (i = iBlockListIndex; i < gs_stDynamicMemory.iMaxLevelCount; i++)
	{
		kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EXIST);
		
		// check right offset if block offset is even
		// check left offset the other
		if ((iBlockOffset % 2) == 0)
		{
			iBuddyBlockOffset = iBlockOffset + 1;
		}
		else
		{
			iBuddyBlockOffset = iBlockOffset - 1;
		}
		bFlag = kGetFlagInBitmap(i, iBuddyBlockOffset);

		if (bFlag == DYNAMICMEMORY_EMPTY)
			break;

		kSetFlagInBitmap(i, iBuddyBlockOffset, DYNAMICMEMORY_EMPTY);
		kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EMPTY);

		iBlockOffset = iBlockOffset / 2;
	}

	kUnlockForSpinLock(&(gs_stDynamicMemory.stSpinLock));
	return TRUE;
}

static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset)
{
	BYTE* pbBitmap;
	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;
	if ((pbBitmap[iOffset / 8] & (0x01 << (iOffset % 8))) != 0x00)
		return DYNAMICMEMORY_EXIST;
	return DYNAMICMEMORY_EMPTY;
}

void kGetDynamicMemoryInformation(QWORD* pqwDynamicMemoryStartAddress,
		QWORD* pqwDynamicMemoryTotalSize, QWORD* pqwMetaDataSize,
		QWORD* pqwUsedMemorySize)
{
	*pqwDynamicMemoryStartAddress = DYNAMICMEMORY_START_ADDRESS;
	*pqwDynamicMemoryTotalSize = kCalculateDynamicMemorySize();
	*pqwMetaDataSize = kCalculateMetaBlockCount(*pqwDynamicMemoryTotalSize) *
		DYNAMICMEMORY_MIN_SIZE;
	*pqwUsedMemorySize = gs_stDynamicMemory.qwUsedSize;
}

DYNAMICMEMORY* kGetDynamicMemoryManager(void)
{
	return &gs_stDynamicMemory;
}

		
