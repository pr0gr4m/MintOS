#include "Utility.h"
#include "AssemblyUtility.h"
#include "VBE.h"
#include <stdarg.h>

volatile QWORD g_qwTickCount = 0;

void kMemSet(void* pvDestination, BYTE bData, int iSize)
{
	int i;
	QWORD qwData;
	int iRemainByteStartOffset;

	qwData = 0;
	for (i = 0; i < 8; i++)
		qwData = (qwData << 8) | bData;

	for (i = 0; i < (iSize / 8); i++)
		((QWORD*)pvDestination)[i] = qwData;

	iRemainByteStartOffset = i * 8;
	for (i = 0; i < (iSize % 8); i++)
		((char*)pvDestination)[iRemainByteStartOffset++] = bData;
}

int kMemCpy(void* pvDestination, const void* pvSource, int iSize)
{
	int i, iRemainByteStartOffset;

	for (i = 0; i < (iSize / 8); i++)
		((QWORD*)pvDestination)[i] = ((QWORD*)pvSource)[i];

	iRemainByteStartOffset = i * 8;
	for (i = 0; i < (iSize % 8); i++)
	{
		((char*)pvDestination)[iRemainByteStartOffset] =
			((char*)pvSource)[iRemainByteStartOffset];
		iRemainByteStartOffset++;
	}

	return iSize;
}

int kMemCmp(const void* pvDestination, const void* pvSource, int iSize)
{
	int i, j;
	int iRemainByteStartOffset;
	QWORD qwValue;
	char cValue;

	for (i = 0; i < (iSize / 8); i++)
	{
		qwValue = ((QWORD*)pvDestination)[i] - ((QWORD*)pvSource)[i];
		if (qwValue != 0)
		{
			for (j = 0; j < 8; j++)
			{
				if (((qwValue >> (j * 8)) & 0xFF) != 0)
					return (qwValue >> (j * 8)) & 0xFF;
			}
		}
	}

	iRemainByteStartOffset = i * 8;
	for (i = 0; i < (iSize % 8); i++)
	{
		cValue = ((char*)pvDestination)[iRemainByteStartOffset] -
			((char*)pvSource)[iRemainByteStartOffset];
		if (cValue != 0)
			return cValue;
		iRemainByteStartOffset++;
	}

	return 0;
}

inline void kMemSetWord(void* pvDestination, WORD wData, int iWordSize)
{
	int i;
	QWORD qwData;
	int iRemainWordStartOffset;

	qwData = 0;
	for (i = 0; i < 4; i++)
		qwData = (qwData << 16) | wData;

	for (i = 0; i < (iWordSize / 4); i++)
		((QWORD*)pvDestination)[i] = qwData;

	iRemainWordStartOffset = i * 4;
	for (i = 0; i < (iWordSize % 4); i++)
		((WORD*)pvDestination)[iRemainWordStartOffset++] = wData;
}

BOOL kSetInterruptFlag(BOOL bEnableInterrupt)
{
	QWORD qwRFLAGS;

	qwRFLAGS = kReadRFLAGS();
	if (bEnableInterrupt == TRUE)
		kEnableInterrupt();
	else
		kDisableInterrupt();

	if (qwRFLAGS & 0x0200)
		return TRUE;
	return FALSE;
}

int kStrLen(const char* pcBuffer)
{
	const char* s;
	for (s = pcBuffer; *s; s++);
	return s - pcBuffer;
}

static QWORD gs_qwTotalRAMMBSize = 0;

void kCheckTotalRAMSize(void)
{
	DWORD* pdwCurrentAddress;
	DWORD dwPreviousValue;

	pdwCurrentAddress = (DWORD*)0x4000000;
	while (1)
	{
		dwPreviousValue = *pdwCurrentAddress;
		*pdwCurrentAddress = 0x12345678;
		if (*pdwCurrentAddress != 0x12345678)
			break;
		*pdwCurrentAddress = dwPreviousValue;
		pdwCurrentAddress += (0x400000 / 4);
	}

	gs_qwTotalRAMMBSize = (QWORD)pdwCurrentAddress / 0x100000;
}

QWORD kGetTotalRAMSize(void)
{
	return gs_qwTotalRAMMBSize;
}

long kAToI(const char* pcBuffer, int iRadix)
{
	switch (iRadix)
	{
		case 16:
			return kHexStringToQword(pcBuffer);

		case 10:
		default:
			return kDecimalStringToLong(pcBuffer);
	}
	return 0;	// never reached
}

QWORD kHexStringToQword(const char* pcBuffer)
{
	QWORD qwValue = 0;
	int i;

	for (i = 0; pcBuffer[i]; i++)
	{
		qwValue *= 16;
		// issue : over F
		if (('A' <= pcBuffer[i]) && (pcBuffer[i] <= 'Z'))
		{
			qwValue += (pcBuffer[i] - 'A') + 10;
		}
		else if (('a' <= pcBuffer[i]) && (pcBuffer[i] <= 'z'))
		{
			qwValue += (pcBuffer[i] - 'a') + 10;
		}
		else
		{
			qwValue += pcBuffer[i] - '0';
		}
	}
	return qwValue;
}

long kDecimalStringToLong(const char* pcBuffer)
{
	long lValue = 0;
	int i;

	if (pcBuffer[0] == '-')
		i = 1;
	else
		i = 0;

	for ( ; pcBuffer[i]; i++)
	{
		lValue *= 10;
		lValue += pcBuffer[i] - '0';
	}

	if (pcBuffer[0] == '-')
		lValue = -lValue;
	return lValue;
}

int kIToA(long lValue, char* pcBuffer, int iRadix)
{
	switch (iRadix)
	{
		case 16:
			return kHexToString(lValue, pcBuffer);
		case 10:
		default:
			return kDecimalToString(lValue, pcBuffer);
	}
	return 0;	// never reached
}

int kHexToString(QWORD qwValue, char* pcBuffer)
{
	QWORD i;
	QWORD qwCurrentValue;

	if (qwValue == 0)
	{
		pcBuffer[0] = '0';
		pcBuffer[1] = '\0';
		return 1;
	}

	for (i = 0; qwValue > 0; i++)
	{
		qwCurrentValue = qwValue % 16;
		if (qwCurrentValue >= 10)
		{
			pcBuffer[i] = 'A' + (qwCurrentValue - 10);
		}
		else
		{
			pcBuffer[i] = '0' + qwCurrentValue;
		}

		qwValue /= 16;
	}
	pcBuffer[i] = '\0';
	kReverseString(pcBuffer);
	return i;
}

int kDecimalToString(long lValue, char* pcBuffer)
{
	long i;
	
	if (lValue == 0)
	{
		pcBuffer[0] = '0';
		pcBuffer[1] = '\0';
		return 1;
	}

	if (lValue < 0)
	{
		i = 1;
		pcBuffer[0] = '-';
		lValue = -lValue;
	}
	else
	{
		i = 0;
	}

	for (; lValue > 0; i++)
	{
		pcBuffer[i] = '0' + lValue % 10;

		lValue /= 10;
	}
	pcBuffer[i] = '\0';
	if (pcBuffer[0] == '-')
		kReverseString(&(pcBuffer[1]));
	else
		kReverseString(pcBuffer);
	return i;
}

void kReverseString(char* pcBuffer)
{
	int iLength;
	int i;
	char cTemp;

	iLength = kStrLen(pcBuffer);
	for (i = 0; i < iLength / 2; i++)
	{
		cTemp = pcBuffer[i];
		pcBuffer[i] = pcBuffer[iLength - 1 - i];
		pcBuffer[iLength - 1 - i] = cTemp;
	}
}

int kSPrintf(char* pcBuffer, const char* pcFormatString, ...)
{
	va_list ap;
	int iReturn;

	va_start(ap, pcFormatString);
	iReturn = kVSPrintf(pcBuffer, pcFormatString, ap);
	va_end(ap);

	return iReturn;
}

int kVSPrintf(char* pcBuffer, const char* pcFormatString, va_list ap)
{
	QWORD i, j, k;
	int iBufferIndex = 0;
	int iFormatLength, iCopyLength;
	char* pcCopyString;
	QWORD qwValue;
	int iValue;
	double dValue;

	iFormatLength = kStrLen(pcFormatString);
	for (i = 0; i < iFormatLength; i++)
	{
		if (pcFormatString[i] == '%')
		{
			i++;
			switch (pcFormatString[i])
			{
				case 's':
					pcCopyString = (char*)(va_arg(ap, char*));
					iCopyLength = kStrLen(pcCopyString);
					kMemCpy(pcBuffer + iBufferIndex, pcCopyString, iCopyLength);
					iBufferIndex += iCopyLength;
					break;

				case 'c':
					pcBuffer[iBufferIndex] = (char)(va_arg(ap, int));
					iBufferIndex++;
					break;

				case 'd':
				case 'i':
					iValue = (int)(va_arg(ap, int));
					iBufferIndex += kIToA(iValue, pcBuffer + iBufferIndex, 10);
					break;

				case 'x':
				case 'X':
					qwValue = (DWORD)(va_arg(ap, DWORD)) & 0xFFFFFFFF;
					iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
					break;

				case 'q':
				case 'Q':
				case 'p':
					qwValue = (QWORD)(va_arg(ap, QWORD));
					iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
					break;

				case 'f':
					dValue = (double)(va_arg(ap, double));
					dValue += 0.005;
					pcBuffer[iBufferIndex] = '0' + (QWORD)(dValue * 100) % 10;
					pcBuffer[iBufferIndex + 1] = '0' + (QWORD)(dValue * 10) % 10;
					pcBuffer[iBufferIndex + 2] = '.';
					for (k = 0; ; k++)
					{
						if (((QWORD)dValue == 0) && (k != 0))
							break;

						pcBuffer[iBufferIndex + 3 + k] = '0' + ((QWORD)dValue % 10);
						dValue /= 10;
					}
					pcBuffer[iBufferIndex + 3 + k] = '\0';
					kReverseString(pcBuffer + iBufferIndex);
					iBufferIndex += 3 + k;
					break;

				default:
					pcBuffer[iBufferIndex] = pcFormatString[i];
					iBufferIndex++;
					break;
			}
		}
		else
		{
			pcBuffer[iBufferIndex++] = pcFormatString[i];
		}
	}

	pcBuffer[iBufferIndex] = '\0';
	return iBufferIndex;
}

QWORD kGetTickCount(void)
{
	return g_qwTickCount;
}

void kSleep(QWORD qwMillisecond)
{
	QWORD qwLastTickCount;

	qwLastTickCount = g_qwTickCount;
	while ((g_qwTickCount - qwLastTickCount) <= qwMillisecond)
	{
		kSchedule();
	}
}

static volatile QWORD gs_qwRandomValue = 0;

QWORD kRandom(void)
{
	gs_qwRandomValue = (gs_qwRandomValue * 412153 + 5571031) >> 16;
	return gs_qwRandomValue;
}

BOOL kIsGraphicMode(void)
{
	if (*(BYTE*)VBE_STARTGRAPHICMODEFLAGADDRESS == 0)
		return FALSE;
	return TRUE;
}
