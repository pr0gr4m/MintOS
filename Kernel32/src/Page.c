#include "Page.h"
#include "../../Kernel64/src/Task.h"

#define DYNAMICMEMORY_START_ADDRESS		((TASK_STACKPOOLADDRESS + 0x1FFFFF) & \
										0xFFE00000)

void kInitializePageTables(void)
{
	PML4TENTRY* pstPML4TEntry;
	PDPTENTRY* pstPDPTEntry;
	PDENTRY* pstPDEntry;
	DWORD dwMappingAddress;
	DWORD dwPageEntryFlags;
	int i;

	// create a PML4 Table
	// init first entry with default value
	// init others with 0x00
	pstPML4TEntry = (PML4TENTRY*)0x100000;
	kSetPageEntryData(&(pstPML4TEntry[0]), 0x00, 0x101000,
			PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
	for (i = 1; i < PAGE_MAXENTRYCOUNT; i++)
	{
		kSetPageEntryData(&(pstPML4TEntry[i]), 0, 0, 0, 0);
	}

	// Create a Page Directory Pointer Table
	// set 64 number of entries to mapping 64GB
	pstPDPTEntry = (PDPTENTRY*)0x101000;
	for (i = 0; i < 64; i++)
	{
		kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0x102000 + (i * PAGE_TABLESIZE),
				PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
	}
	for (i = 64; i < PAGE_MAXENTRYCOUNT; i++)
	{
		kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0, 0, 0);
	}

	// create 64 number of  Page Directory Tables
	// one page directory can map to 1GB
	pstPDEntry = (PDENTRY*)0x102000;
	dwMappingAddress = 0;
	for (i = 0; i < PAGE_MAXENTRYCOUNT * 64; i++)
	{
		if (i < ((DWORD)DYNAMICMEMORY_START_ADDRESS / PAGE_DEFAULTSIZE))
			dwPageEntryFlags = PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS;		// kernel DPL
		else
			dwPageEntryFlags = PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS | PAGE_FLAGS_US;	// user DPL

		kSetPageEntryData(&(pstPDEntry[i]), (i * (PAGE_DEFAULTSIZE >> 20)) >> 12,
				dwMappingAddress, dwPageEntryFlags, 0);
		dwMappingAddress += PAGE_DEFAULTSIZE;
	}
}

void kSetPageEntryData(PTENTRY* pstEntry, DWORD dwUpperBaseAddress,
		DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags)
{
	pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
	pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}
