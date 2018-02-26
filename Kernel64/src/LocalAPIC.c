#include "LocalAPIC.h"
#include "MPConfigurationTable.h"

QWORD kGetLocalAPICBaseAddress(void)
{
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;

	pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;
	return pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
}

void kEnableSoftwareLocalAPIC(void)
{
	QWORD qwLocalAPICBaseAddress;

	qwLocalAPICBaseAddress = kGetLocalAPICBaseAddress();
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_SVR) |= 0x100;
}
