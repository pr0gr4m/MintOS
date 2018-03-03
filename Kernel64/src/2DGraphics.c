#include "2DGraphics.h"
#include "VBE.h"
#include "Font.h"
#include "Utility.h"

inline void kDrawPixel(int iX, int iY, COLOR stColor)
{
	VBEMODEINFOBLOCK* pstModeInfo;
	pstModeInfo = kGetVBEModeInfoBlock();
	*(((COLOR*)(QWORD)pstModeInfo->dwPhysicalBasePointer) +
			pstModeInfo->wXResolution * iY + iX) = stColor;
}

void kDrawLine(int iX1, int iY1, int iX2, int iY2, COLOR stColor)
{
	int iDeltaX, iDeltaY;
	int iError = 0;
	int iDeltaError;
	int iX, iY;
	int iStepX, iStepY;

	iDeltaX = iX2 - iX1;
	iDeltaY = iY2 - iY1;

	if (iDeltaX < 0)
	{
		iDeltaX = -iDeltaX;
		iStepX = -1;
	}
	else
	{
		iStepX = 1;
	}

	if (iDeltaY < 0)
	{
		iDeltaY = -iDeltaY;
		iStepY = -1;
	}
	else
	{
		iStepY = 1;
	}

	if (iDeltaX > iDeltaY)
	{
		iDeltaError = iDeltaY << 1;
		iY = iY1;
		for (iX = iX1; iX != iX2; iX += iStepX)
		{
			kDrawPixel(iX, iY, stColor);
			iError += iDeltaError;
			if (iError >= iDeltaX)
			{
				iY += iStepY;
				iError -= iDeltaX << 1;
			}
		}
		kDrawPixel(iX, iY, stColor);
	}
	else
	{
		iDeltaError = iDeltaX << 1;
		iX = iX1;
		for (iY = iY1; iY != iY2; iY += iStepY)
		{
			kDrawPixel(iX, iY, stColor);
			iError += iDeltaError;
			if (iError >= iDeltaY)
			{
				iX += iStepX;
				iError -= iDeltaY << 1;
			}
		}
		kDrawPixel(iX, iY, stColor);
	}
}

void kDrawRect(int iX1, int iY1, int iX2, int iY2, COLOR stColor, BOOL bFill)
{
	int iWidth;
	int iTemp;
	int iY;
	int iStepY;
	COLOR* pstVideoMemoryAddress;
	VBEMODEINFOBLOCK* pstModeInfo;

	if (bFill == FALSE)
	{
		kDrawLine(iX1, iY1, iX2, iY1, stColor);
		kDrawLine(iX1, iY1, iX1, iY2, stColor);
		kDrawLine(iX2, iY1, iX2, iY2, stColor);
		kDrawLine(iX1, iY2, iX2, iY2, stColor);
	}
	else
	{
		pstModeInfo = kGetVBEModeInfoBlock();
		pstVideoMemoryAddress =
			(COLOR*)((QWORD)pstModeInfo->dwPhysicalBasePointer);

		if (iX2 < iX1)
		{
			iTemp = iX1;
			iX1 = iX2;
			iX2 = iTemp;

			iTemp = iY1;
			iY1 = iY2;
			iY2 = iTemp;
		}

		iWidth = iX2 - iX1 + 1;

		if (iY1 <= iY2)
			iStepY = 1;
		else
			iStepY = -1;

		pstVideoMemoryAddress += iY1 * pstModeInfo->wXResolution + iX1;
		for (iY = iY1; iY != iY2; iY += iStepY)
		{
			kMemSetWord(pstVideoMemoryAddress, stColor, iWidth);
			if (iStepY >= 0)
				pstVideoMemoryAddress += pstModeInfo->wXResolution;
			else
				pstVideoMemoryAddress -= pstModeInfo->wXResolution;
		}

		kMemSetWord(pstVideoMemoryAddress, stColor, iWidth);
	}
}

void kDrawCircle(int iX, int iY, int iRadius, COLOR stColor, BOOL bFill)
{
	int iCircleX, iCircleY;
	int iDistance;
	
	if (iRadius <= 0)
		return;

	// start at (0, R)
	iCircleY = iRadius;

	if (bFill == FALSE)
	{
		kDrawPixel(iX, iRadius + iY, stColor);
		kDrawPixel(iX, -iRadius + iY, stColor);
		kDrawPixel(iRadius + iX, iY, stColor);
		kDrawPixel(-iRadius + iX, iY, stColor);
	}
	else
	{
		kDrawLine(iX, iRadius + iY, iX, -iRadius + iY, stColor);
		kDrawLine(iRadius + iX, iY, -iRadius + iX, iY, stColor);
	}

	iDistance = -iRadius;
	for (iCircleX = 1; iCircleX <= iCircleY; iCircleX++)
	{
		iDistance += (iCircleX << 1) - 1;
		
		if (iDistance >= 0)
		{
			iCircleY--;
			iDistance += (-iCircleY << 1) + 2;
		}

		if (bFill == FALSE)
		{
			kDrawPixel(iCircleX + iX, iCircleY + iY, stColor);
			kDrawPixel(iCircleX + iX, -iCircleY + iY, stColor);
			kDrawPixel(-iCircleX + iX, iCircleY + iY, stColor);
			kDrawPixel(-iCircleX + iX, -iCircleY + iY, stColor);
			kDrawPixel(iCircleY + iX, iCircleX + iY, stColor);
			kDrawPixel(iCircleY + iX, -iCircleX + iY, stColor);
			kDrawPixel(-iCircleY + iX, iCircleX + iY, stColor);
			kDrawPixel(-iCircleY + iX, -iCircleX + iY, stColor);
		}
		else
		{
			kDrawRect(-iCircleX + iX, iCircleY + iY,
					iCircleX + iX, iCircleY + iY, stColor, TRUE);
			kDrawRect(-iCircleX + iX, -iCircleY + iY,
					iCircleX + iX, -iCircleY + iY, stColor, TRUE);
			kDrawRect(-iCircleY + iX, iCircleX + iY,
					iCircleY + iX, iCircleX + iY, stColor, TRUE);
			kDrawRect(-iCircleY + iX, -iCircleX + iY,
					iCircleY + iX, -iCircleX + iY, stColor, TRUE);
		}
	}
}

void kDrawText(int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor,
		const char* pcString, int iLength)
{
	int iCurrentX, iCurrentY;
	int i, j, k;
	BYTE bBitmask;
	int iBitmaskStartIndex;
	VBEMODEINFOBLOCK* pstModeInfo;
	COLOR* pstVideoMemoryAddress;

	pstModeInfo = kGetVBEModeInfoBlock();

	pstVideoMemoryAddress =
		(COLOR*)((QWORD)pstModeInfo->dwPhysicalBasePointer);

	iCurrentX = iX;

	for (k = 0; k < iLength; k++)
	{
		iCurrentY = iY * pstModeInfo->wXResolution;
		iBitmaskStartIndex = pcString[k] * FONT_ENGLISHHEIGHT;

		for (j = 0; j < FONT_ENGLISHHEIGHT; j++)
		{
			bBitmask = g_vucEnglishFont[iBitmaskStartIndex++];
			for (i = 0; i < FONT_ENGLISHWIDTH; i++)
			{
				if (bBitmask & (0x01 << (FONT_ENGLISHWIDTH - i - 1)))
					pstVideoMemoryAddress[iCurrentY + iCurrentX + i] = stTextColor;
				else
					pstVideoMemoryAddress[iCurrentY + iCurrentX + i] = stBackgroundColor;
			}
			iCurrentY += pstModeInfo->wXResolution;
		}

		iCurrentX += FONT_ENGLISHWIDTH;
	}
}

