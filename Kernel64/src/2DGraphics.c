#include "2DGraphics.h"
#include "VBE.h"
#include "Font.h"
#include "Utility.h"

inline BOOL kIsInRectangle(const RECT* pstArea, int iX, int iY)
{
	if ((iX < pstArea->iX1) || (pstArea->iX2 < iX) ||
			(iY < pstArea->iY1) || (pstArea->iY2 < iY))
		return FALSE;
	return TRUE;
}

inline int kGetRectangleWidth(const RECT* pstArea)
{
	if (pstArea->iX2 >= pstArea->iX1)
		return pstArea->iX2 - pstArea->iX1 + 1;
	else
		return pstArea->iX1 - pstArea->iX2 + 1;
}

inline int kGetRectangleHeight(const RECT* pstArea)
{
	if (pstArea->iY2 >= pstArea->iY1)
		return pstArea->iY2 - pstArea->iY1 + 1;
	else
		return pstArea->iY1 - pstArea->iY2 + 1;
}

inline BOOL kIsRectangleOverlapped(const RECT* pstArea1, const RECT* pstArea2)
{
	if ((pstArea1->iX1 > pstArea2->iX2) || (pstArea1->iX2 < pstArea2->iX1) ||
			(pstArea1->iY1 > pstArea2->iY2) || (pstArea1->iY2 < pstArea2->iY1))
		return FALSE;
	return TRUE;
}

BOOL kGetOverlappedRectangle(const RECT* pstArea1, const RECT* pstArea2,
		RECT* pstIntersection)
{
	int iMaxX1, iMinX2;
	int iMaxY1, iMinY2;

	iMaxX1 = MAX(pstArea1->iX1, pstArea2->iX1);
	iMinX2 = MIN(pstArea1->iX2, pstArea2->iX2);
	if (iMinX2 < iMaxX1)
		return FALSE;

	iMaxY1 = MAX(pstArea1->iY1, pstArea2->iY1);
	iMinY2 = MIN(pstArea1->iY2, pstArea2->iY2);
	if (iMinY2 < iMaxY1)
		return FALSE;

	pstIntersection->iX1 = iMaxX1;
	pstIntersection->iY1 = iMaxY1;
	pstIntersection->iX2 = iMinX2;
	pstIntersection->iY2 = iMinY2;
	return TRUE;
}

void kSetRectangleData(int iX1, int iY1, int iX2, int iY2, RECT* pstRect)
{
	// to be x1 < x2
	if (iX1 < iX2)
	{
		pstRect->iX1 = iX1;
		pstRect->iX2 = iX2;
	}
	else
	{
		pstRect->iX1 = iX2;
		pstRect->iX2 = iX1;
	}
	// to be y1 < y2
	if (iY1 < iY2)
	{
		pstRect->iY1 = iY1;
		pstRect->iY2 = iY2;
	}
	else
	{
		pstRect->iY1 = iY2;
		pstRect->iY2 = iY1;
	}
}

inline void kInternalDrawPixel(const RECT* pstMemoryArea, COLOR* pstMemoryAddress,
		int iX, int iY, COLOR stColor)
{
	int iWidth;
	if (kIsInRectangle(pstMemoryArea, iX, iY) == FALSE)
		return;
	iWidth = kGetRectangleWidth(pstMemoryArea);
	*(pstMemoryAddress + (iWidth * iY) + iX) = stColor;
}

void kInternalDrawLine(const RECT* pstMemoryArea, COLOR* pstMemoryAddress,
		int iX1, int iY1, int iX2, int iY2, COLOR stColor)
{
	int iDeltaX, iDeltaY;
	int iError = 0;
	int iDeltaError;
	int iX, iY;
	int iStepX, iStepY;
	RECT stLineArea;

	kSetRectangleData(iX1, iY1, iX2, iY2, &stLineArea);
	if (kIsRectangleOverlapped(pstMemoryArea, &stLineArea) == FALSE)
		return;

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
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iX, iY, stColor);
			iError += iDeltaError;
			if (iError >= iDeltaX)
			{
				iY += iStepY;
				iError -= iDeltaX << 1;
			}
		}
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iX, iY, stColor);
	}
	else
	{
		iDeltaError = iDeltaX << 1;
		iX = iX1;
		for (iY = iY1; iY != iY2; iY += iStepY)
		{
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iX, iY, stColor);
			iError += iDeltaError;
			if (iError >= iDeltaY)
			{
				iX += iStepX;
				iError -= iDeltaY << 1;
			}
		}
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iX, iY, stColor);
	}
}

void kInternalDrawRect(const RECT* pstMemoryArea, COLOR* pstMemoryAddress,
		int iX1, int iY1, int iX2, int iY2, COLOR stColor, BOOL bFill)
{
	int iWidth;
	int iTemp;
	int iY;
	int iMemoryAreaWidth;
	RECT stDrawRect;
	RECT stOverlappedArea;

	if (bFill == FALSE)
	{
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, iX1, iY1, iX2, iY1, stColor);
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, iX1, iY1, iX1, iY2, stColor);
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, iX2, iY1, iX2, iY2, stColor);
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, iX1, iY2, iX2, iY2, stColor);
	}
	else
	{
		kSetRectangleData(iX1, iY1, iX2, iY2, &stDrawRect);

		if (kGetOverlappedRectangle(pstMemoryArea, &stDrawRect,
					&stOverlappedArea) == FALSE)
			return;

		iWidth = kGetRectangleWidth(&stOverlappedArea);
		iMemoryAreaWidth = kGetRectangleWidth(pstMemoryArea);

		pstMemoryAddress += stOverlappedArea.iY1 * iMemoryAreaWidth +
			stOverlappedArea.iX1;

		for (iY = stOverlappedArea.iY1; iY < stOverlappedArea.iY2; iY++)
		{
			kMemSetWord(pstMemoryAddress, stColor, iWidth);
			pstMemoryAddress += iMemoryAreaWidth;
		}

		kMemSetWord(pstMemoryAddress, stColor, iWidth);
	}
}

void kInternalDrawCircle(const RECT* pstMemoryArea, COLOR* pstMemoryAddress,
		int iX, int iY, int iRadius, COLOR stColor, BOOL bFill)
{
	int iCircleX, iCircleY;
	int iDistance;
	
	if (iRadius <= 0)
		return;

	// start at (0, R)
	iCircleY = iRadius;

	if (bFill == FALSE)
	{
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iX, iRadius + iY, stColor);
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iX, -iRadius + iY, stColor);
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iRadius + iX, iY, stColor);
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, -iRadius + iX, iY, stColor);
	}
	else
	{
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, 
				iX, iRadius + iY, iX, -iRadius + iY, stColor);
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, 
				iRadius + iX, iY, -iRadius + iX, iY, stColor);
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
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, 
					iCircleX + iX, iCircleY + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, 
					iCircleX + iX, -iCircleY + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, 
					-iCircleX + iX, iCircleY + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, 
					-iCircleX + iX, -iCircleY + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, 
					iCircleY + iX, iCircleX + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, 
					iCircleY + iX, -iCircleX + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, 
					-iCircleY + iX, iCircleX + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, 
					-iCircleY + iX, -iCircleX + iY, stColor);
		}
		else
		{
			kInternalDrawRect(pstMemoryArea, pstMemoryAddress, 
					-iCircleX + iX, iCircleY + iY,
					iCircleX + iX, iCircleY + iY, stColor, TRUE);
			kInternalDrawRect(pstMemoryArea, pstMemoryAddress, 
					-iCircleX + iX, -iCircleY + iY,
					iCircleX + iX, -iCircleY + iY, stColor, TRUE);
			kInternalDrawRect(pstMemoryArea, pstMemoryAddress, 
					-iCircleY + iX, iCircleX + iY,
					iCircleY + iX, iCircleX + iY, stColor, TRUE);
			kInternalDrawRect(pstMemoryArea, pstMemoryAddress, 
					-iCircleY + iX, -iCircleX + iY,
					iCircleY + iX, -iCircleX + iY, stColor, TRUE);
		}
	}
}

void kInternalDrawText(const RECT* pstMemoryArea, COLOR* pstMemoryAddress,
		int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor,
		const char* pcString, int iLength)
{
	int iCurrentX, iCurrentY;
	int i, j, k;
	BYTE bBitmap;
	BYTE bCurrentBitmask;
	int iBitmapStartIndex;
	int iMemoryAreaWidth;
	RECT stFontArea;
	RECT stOverlappedArea;
	int iStartYOffset;
	int iStartXOffset;
	int iOverlappedWidth;
	int iOverlappedHeight;

	iCurrentX = iX;

	iMemoryAreaWidth = kGetRectangleWidth(pstMemoryArea);

	for (k = 0; k < iLength; k++)
	{
		iCurrentY = iY * iMemoryAreaWidth;

		kSetRectangleData(iCurrentX, iY, iCurrentX + FONT_ENGLISHWIDTH - 1,
				iY + FONT_ENGLISHHEIGHT - 1, &stFontArea);

		if (kGetOverlappedRectangle(pstMemoryArea, &stFontArea,
					&stOverlappedArea) == FALSE)
		{
			iCurrentX += FONT_ENGLISHWIDTH;
			continue;
		}

		iBitmapStartIndex = pcString[k] * FONT_ENGLISHHEIGHT;

		iStartXOffset = stOverlappedArea.iX1 - iCurrentX;
		iStartYOffset = stOverlappedArea.iY1 - iY;
		iOverlappedWidth = kGetRectangleWidth(&stOverlappedArea);
		iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);

		iBitmapStartIndex += iStartYOffset;

		for (j = iStartYOffset; j < iOverlappedHeight; j++)
		{
			bBitmap = g_vucEnglishFont[iBitmapStartIndex++];
			bCurrentBitmask = 0x01 << (FONT_ENGLISHWIDTH - 1 - iStartXOffset);

			for (i = iStartXOffset; i < iOverlappedWidth; i++)
			{
				if (bBitmap & bCurrentBitmask)
					pstMemoryAddress[iCurrentY + iCurrentX + i] = stTextColor;
				else
					pstMemoryAddress[iCurrentY + iCurrentX + i] = stBackgroundColor;

				bCurrentBitmask = bCurrentBitmask >> 1;
			}
			
			iCurrentY += iMemoryAreaWidth;
		}

		iCurrentX += FONT_ENGLISHWIDTH;
	}
}

