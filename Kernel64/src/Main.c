#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "PIC.h"
#include "AssemblyUtility.h"

void kPrintString(int iX, int iY, const char* pcString);

void Main(void)
{
	char vcTemp[2] = { 0, };
	BYTE bFlags, bTemp;
	int i = 0;
	KEYDATA stData;

	kPrintString(0, 10, "Switch To IA-32e Mode Success!");
	kPrintString(0, 11, "IA-32e C Language Kernel Start.............................[PASS]");
	kPrintString(0, 12, "GDT Initialize And Switch For IA-32e Mode..................[    ]");
	kInitializeGDTTableAndTSS();
	kLoadGDTR(GDTR_STARTADDRESS);
	kPrintString(60, 12, "PASS");

	kPrintString(0, 13, "TSS Segment Load...........................................[    ]");
	kLoadTR(GDT_TSSSEGMENT);
	kPrintString(60, 13, "PASS");

	kPrintString(0, 14, "IDT Initialize.............................................[    ]");
	kInitializeIDTTables();
	kLoadIDTR(IDTR_STARTADDRESS);
	kPrintString(60, 14, "PASS");

	kPrintString(0, 15, "Keyboard Activate And Queue Initialize.....................[    ]");

	if (kInitializeKeyboard() == TRUE)
	{
		kPrintString(60, 15, "PASS");
		kChangeKeyboardLED(FALSE, FALSE, FALSE);
	}
	else
	{
		kPrintString(60, 15, "FAIL");
		while (1);
	}

	kPrintString(0, 16, "PIC Controller And Interrupt Initialize....................[    ]");
	kInitializePIC();
	kMaskPICInterrupt(0);
	kEnableInterrupt();
	kPrintString(60, 16, "PASS");

	while (1)
	{
		if (kGetKeyFromKeyQueue(&stData) == TRUE)
		{
			if (stData.bFlags & KEY_FLAGS_DOWN)
			{
				vcTemp[0] = stData.bASCIICode;
				kPrintString(i++, 17, vcTemp);

				if (vcTemp[0] == '0')
					bTemp = bTemp / 0;
			}
		}
	}
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
