#include "MultiProcessor.h"
#include "MPConfigurationTable.h"
#include "AssemblyUtility.h"
#include "Utility.h"
#include "LocalAPIC.h"
#include "PIT.h"

volatile int g_iWakeUpApplicationProcessorCount = 0;
volatile QWORD g_qwAPICIDAddress = 0;

BOOL kStartUpApplicationProcessor(void)
{
	if (kAnalysisMPConfigurationTable() == FALSE)
		return FALSE;

	kEnableGlobalLocalAPIC();
	kEnableSoftwareLocalAPIC();

	if (kWakeUpApplicationProcessor() == FALSE)
		return FALSE;

	return TRUE;
}

static BOOL kWakeUpApplicationProcessor(void)
{
	MPCONFIGURATIONMANAGER* pstMPManager;
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;
	QWORD qwLocalAPICBaseAddress;
	BOOL bInterruptFlag;
	int i;

	bInterruptFlag = kSetInterruptFlag(FALSE);

	pstMPManager = kGetMPConfigurationManager();
	pstMPHeader = pstMPManager->pstMPConfigurationTableHeader;
	qwLocalAPICBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;

	g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;

	// send INIT IPI
	// AP start with Protected Mode Kernel (0x10000)
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) =
		APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE |
		APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_INIT;

	kWaitUsingDirectPIT(MSTOCOUNT(10));

	// check success
	if (*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) &
			APIC_DELIVERYSTATUS_PENDING)
	{
		kInitializePIT(MSTOCOUNT(1), TRUE);

		kSetInterruptFlag(bInterruptFlag);
		return FALSE;
	}

	// send startup IPI
	for (i = 0; i < 2; i++)
	{
		// start address is 0x10(0x10000 / 4KB)
		*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) =
			APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE |
			APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL |
			APIC_DELIVERYMODE_STARTUP | 0x10;
		
		kWaitUsingDirectPIT(USTOCOUNT(200));

		if (*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) &
				APIC_DELIVERYSTATUS_PENDING)
		{
			kInitializePIT(MSTOCOUNT(1), TRUE);

			kSetInterruptFlag(bInterruptFlag);
			return FALSE;
		}
	}

	kInitializePIT(MSTOCOUNT(1), TRUE);
	kSetInterruptFlag(bInterruptFlag);

	while (g_iWakeUpApplicationProcessorCount <
			(pstMPManager->iProcessorCount - 1))
		kSleep(50);
	return TRUE;
}

BYTE kGetAPICID(void)
{
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;
	QWORD qwLocalAPICBaseAddress;

	if (g_qwAPICIDAddress == 0)
	{
		pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;
		if (pstMPHeader == NULL)
			return 0;
		qwLocalAPICBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
		g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;
	}

	return *((DWORD*)g_qwAPICIDAddress) >> 24;
}


