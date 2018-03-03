#ifndef __VBE_H__
#define __VBE_H__

#include "Types.h"

#define VBE_MODEINFOBLOCKADDRESS		0x7E00
#define VBE_STARTGRAPHICMODEFLAGADDRESS	0x7C0A

#pragma pack(push, 1)

typedef struct kVBEInfoBlockStruct
{
	// common field
	WORD wModeAttribute;
	BYTE bWinAAttribute;
	BYTE bWinBAttribute;
	WORD wWinGranulity;
	WORD wWinSize;
	WORD wWinASegment;
	WORD wWinBSegment;
	DWORD dwWinFuncPtr;
	WORD wBytesPerScanLine;

	// VBE Ver >= 1.2
	WORD wXResolution;
	WORD wYResolution;
	BYTE bXCharSize;
	BYTE bYCharSize;
	BYTE bNumberOfPlane;
	BYTE bBitsPerPixel;
	BYTE bNumberOfBanks;
	BYTE bMemoryModel;
	BYTE bBankSize;
	BYTE bNumberOfImagePages;
	BYTE bReserved;

	BYTE bRedMaskSize;
	BYTE bRedFieldPosition;
	BYTE bGreenMaskSize;
	BYTE bGreenFieldPosition;
	BYTE bBlueMaskSize;
	BYTE bBlueFieldPosition;
	BYTE bReservedMaskSize;
	BYTE bReservedFieldPosition;
	BYTE bDirectColorModeInfo;

	// VBE Ver >= 2.0
	DWORD dwPhysicalBasePointer;
	DWORD dwReserved1;
	DWORD dwReserved2;

	// VBE Ver >= 3.0
	WORD wLinearBytesPerScanLine;
	BYTE bBankNumberOfImagePages;
	BYTE bLinearNumberOfImagePages;
	BYTE bLinearRedMaskSize;
	BYTE bLinearRedFieldPosition;
	BYTE bLinearGreenMaskSize;
	BYTE bLinearGreenFieldPosition;
	BYTE bLinearBlueMaskSize;
	BYTE bLinearBlueFieldPosition;
	BYTE bLinearReservedMaskSize;
	BYTE bLinearReservedFieldPosition;
	DWORD dwMaxPixelClock;

	BYTE vbReserved[189];
} VBEMODEINFOBLOCK;

#pragma pack(pop)


VBEMODEINFOBLOCK* kGetVBEModeInfoBlock(void);

#endif
