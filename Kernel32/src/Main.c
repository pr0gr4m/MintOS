#include "Types.h"

void kPrintString(int iX, int iY, const char* pcString);
BOOL kInitializeKernel64Area(void);
BOOL kIsMemoryEnough(void);

void Main(void)
{
	DWORD i;

	kPrintString(0, 3, "C Language Kernel Started..................................[PASS]");

	kInitializeKernel64Area();
	kPrintString(0, 4, "Minimum Memory Size Check..................................[    ]");
	if (kIsMemoryEnough() == FALSE)
	{
		kPrintString(60, 4, "FAIL");
		kPrintString(0, 5, "Not Enough Memory!! MINT64 OS Requires Over 64M Memory!!");
		while (1);
	}
	else
	{
		kPrintString(60, 4, "PASS");
	}

	kPrintString(0, 5, "IA-32e Kernel Area Initialize..............................[    ]");
	if (kInitializeKernel64Area() == FALSE)
	{
		kPrintString(60, 5, "FAIL");
		kPrintString(0, 6, "Kernel Area Initialization Fail!!");
		while (1);
	}

	kPrintString(60, 5, "PASS");

	while (1);
}

void kPrintString(int iX, int iY, const char* pcString)
{
	CHARACTER* pstScreen = (CHARACTER*)0xB8000;
	int i;

	pstScreen += (iY * 80) + iX;
	for (i = 0; pcString[i] != 0; i++)
	{
		pstScreen[i].bCharacter = pcString[i];
	}
}

BOOL kInitializeKernel64Area(void)
{
	DWORD* pdwCurrentAddress;

	// Start address to init
	pdwCurrentAddress = (DWORD*)0x100000;

	while ((DWORD)pdwCurrentAddress < 0x600000)
	{
		*pdwCurrentAddress = 0x00;
		// error when reading value is not 0
		if (*pdwCurrentAddress != 0)
		{
			return FALSE;
		}
		pdwCurrentAddress++;
	}
	return TRUE;
}

BOOL kIsMemoryEnough(void)
{
	DWORD* pdwCurrentAddress;

	// Start address to check
	pdwCurrentAddress = (DWORD*)0x100000;

	// check until 64MB
	while ((DWORD)pdwCurrentAddress < 0x4000000)
	{
		*pdwCurrentAddress = 0x12345678;
		if (*pdwCurrentAddress != 0x12345678)
		{
			return FALSE;
		}
		pdwCurrentAddress += (0x100000 / 4);
	}
	return TRUE;
}
