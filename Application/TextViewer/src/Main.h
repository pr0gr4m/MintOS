#ifndef __MAIN_H__
#define __MAIN_H__

#include "Types.h"

#define MAXLINECOUNT		(256 * 1024)
#define MARGIN			5
#define TABSPACE		4

typedef struct TextInformationStruct
{
	BYTE* pbFileBuffer;
	DWORD dwFileSize;

	int iColumnCount;
	int iRowCount;

	DWORD* pdwFileOffsetOfLine;

	int iMaxLineCount;
	int iCurrentLineIndex;

	char vcFileName[100];
} TEXTINFO;

BOOL ReadFileToBuffer(const char* pcFileName, TEXTINFO* pstInfo);
void CalculateFileOffsetOfLine(int iWidth, int iHeight, TEXTINFO* pstInfo);
BOOL DrawTextBuffer(QWORD qwWindowID, TEXTINFO* pstInfo);

#endif
