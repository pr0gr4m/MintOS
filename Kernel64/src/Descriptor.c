#include "Descriptor.h"
#include "Utility.h"
#include "ISR.h"

extern void kPrintString(int iX, int iY, const char* pcString);

/*
 * GDT & TSS
 */

// Init GDT Table
void kInitializeGDTTableAndTSS(void)
{
	GDTR* pstGDTR;
	GDTENTRY8* pstEntry;
	TSSSEGMENT* pstTSS;
	int i;

	// set GDTR
	pstGDTR = (GDTR*)GDTR_STARTADDRESS;
	pstEntry = (GDTENTRY8*)(GDTR_STARTADDRESS + sizeof(GDTR));
	pstGDTR->wLimit = GDT_TABLESIZE - 1;
	pstGDTR->qwBaseAddress = (QWORD)pstEntry;
	
	// set TSS
	pstTSS = (TSSSEGMENT*)((QWORD)pstEntry + GDT_TABLESIZE);

	// create 4 segment for NULL, 64bit Code/Data, TSS
	kSetGDTEntry8(&(pstEntry[0]), 0, 0, 0, 0, 0);
	kSetGDTEntry8(&(pstEntry[1]), 0, 0xFFFFF, GDT_FLAGS_UPPER_CODE,
			GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE);
	kSetGDTEntry8(&(pstEntry[2]), 0, 0xFFFFF, GDT_FLAGS_UPPER_DATA,
			GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA);
	kSetGDTEntry16((GDTENTRY16*)&(pstEntry[3]), (QWORD)pstTSS,
			sizeof(TSSSEGMENT) - 1, GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS,
			GDT_TYPE_TSS);

	// Init TSS
	kInitializeTSSSegment(pstTSS);
}

void kSetGDTEntry8(GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit,
		BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType)
{
	pstEntry->wLowerLimit = dwLimit & 0xFFFF;
	pstEntry->wLowerBaseAddress = dwBaseAddress & 0xFFFF;
	pstEntry->bUpperBaseAddress1 = (dwBaseAddress >> 16) & 0xFF;
	pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
	pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0x0F) | bUpperFlags;
	pstEntry->bUpperBaseAddress2 = (dwBaseAddress >> 24) & 0xFF;
}

void kSetGDTEntry16(GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit,
		BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType)
{
	pstEntry->wLowerLimit = dwLimit & 0xFFFF;
	pstEntry->wLowerBaseAddress = qwBaseAddress & 0xFFFF;
	pstEntry->bMiddleBaseAddress1 = (qwBaseAddress >> 16) & 0xFF;
	pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
	pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0xFF) | bUpperFlags;
	pstEntry->bMiddleBaseAddress2 = (qwBaseAddress >> 24) & 0xFF;
	pstEntry->dwUpperBaseAddress = qwBaseAddress >> 32;
	pstEntry->dwReserved = 0;
}

void kInitializeTSSSegment(TSSSEGMENT* pstTSS)
{
	kMemSet(pstTSS, 0, sizeof(TSSSEGMENT));
	pstTSS->qwIST[0] = IST_STARTADDRESS + IST_SIZE;
	pstTSS->wIOMapBaseAddress = 0xFFFF;
}


/*
 * IDT
 */

// Init IDT Table
void kInitializeIDTTables(void)
{
	IDTR* pstIDTR;
	IDTENTRY* pstEntry;
	int i;

	// start address of IDTR
	pstIDTR = (IDTR*)IDTR_STARTADDRESS;
	// create IDT Table
	pstEntry = (IDTENTRY*)(IDTR_STARTADDRESS + sizeof(IDTR));
	pstIDTR->qwBaseAddress = (QWORD)pstEntry;
	pstIDTR->wLimit = IDT_TABLESIZE - 1;

	for (i = 0; i < IDT_MAXENTRYCOUNT; i++)
	{
		kSetIDTEntry(&(pstEntry[i]), kDummyHandler, 0x08, IDT_FLAGS_IST1,
					IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	}
	// set Exception ISR
	kSetIDTEntry( &( pstEntry[ 0 ] ), kISRDivideError, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 1 ] ), kISRDebug, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 2 ] ), kISRNMI, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 3 ] ), kISRBreakPoint, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 4 ] ), kISROverflow, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 5 ] ), kISRBoundRangeExceeded, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 6 ] ), kISRInvalidOpcode, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 7 ] ), kISRDeviceNotAvailable, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 8 ] ), kISRDoubleFault, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 9 ] ), kISRCoprocessorSegmentOverrun, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 10 ] ), kISRInvalidTSS, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 11 ] ), kISRSegmentNotPresent, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 12 ] ), kISRStackSegmentFault, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 13 ] ), kISRGeneralProtection, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 14 ] ), kISRPageFault, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 15 ] ), kISR15, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 16 ] ), kISRFPUError, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 17 ] ), kISRAlignmentCheck, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 18 ] ), kISRMachineCheck, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 19 ] ), kISRSIMDError, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 20 ] ), kISRETCException, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );

	for (i = 21; i < 32; i++ )
	{
		kSetIDTEntry( &( pstEntry[ i ] ), kISRETCException, 0x08, IDT_FLAGS_IST1,
			IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	}

	// set Interrupt ISR
	kSetIDTEntry( &( pstEntry[ 32 ] ), kISRTimer, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 33 ] ), kISRKeyboard, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 34 ] ), kISRSlavePIC, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 35 ] ), kISRSerial2, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 36 ] ), kISRSerial1, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 37 ] ), kISRParallel2, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 38 ] ), kISRFloppy, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 39 ] ), kISRParallel1, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 40 ] ), kISRRTC, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 41 ] ), kISRReserved, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 42 ] ), kISRNotUsed1, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 43 ] ), kISRNotUsed2, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 44 ] ), kISRMouse, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 45 ] ), kISRCoprocessor, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 46 ] ), kISRHDD1, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	kSetIDTEntry( &( pstEntry[ 47 ] ), kISRHDD2, 0x08, IDT_FLAGS_IST1,
		IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );

	for (i = 48; i < IDT_MAXENTRYCOUNT; i++ )
	{
		kSetIDTEntry( &( pstEntry[ i ] ), kISRETCInterrupt, 0x08, IDT_FLAGS_IST1,
			IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT );
	}
}

void kSetIDTEntry(IDTENTRY* pstEntry, void* pvHandler, WORD wSelector,
		BYTE bIST, BYTE bFlags, BYTE bType)
{
	pstEntry->wLowerBaseAddress = (QWORD)pvHandler & 0xFFFF;
	pstEntry->wSegmentSelector = wSelector;
	pstEntry->bIST = bIST & 0x3;
	pstEntry->bTypeAndFlags = bType | bFlags;
	pstEntry->wMiddleBaseAddress = ((QWORD)pvHandler >> 16) & 0xFFFF;
	pstEntry->dwUpperBaseAddress = (QWORD)pvHandler >> 32;
	pstEntry->dwReserved = 0;
}

void kDummyHandler(void)
{
	kPrintString(0, 0, "====================================================");
	kPrintString(0, 1, "		Dummy Interrupt Handler Execute!!!		  ");
	kPrintString(0, 2, "		 Interrupt or Exception Occur!!!!		   ");
	kPrintString(0, 3, "====================================================");

	while (1);
}

