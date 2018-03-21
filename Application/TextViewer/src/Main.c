#include "MintLib.h"
#include "Main.h"

int Main(char* pcArgument)
{
	QWORD qwWindowID;
	int iX, iY;
	int iWidth, iHeight;
	TEXTINFO stInfo;
	int iMoveLine;
	EVENT stReceivedEvent;
	KEYEVENT* pstKeyEvent;
	WINDOWEVENT* pstWindowEvent;
	DWORD dwFileOffset;
	RECT stScreenArea;

	if (IsGraphicMode() == FALSE)
	{
		printf("This task can run only GUI Mode.\n");
		return -1;
	}

	if (strlen(pcArgument) == 0)
	{
		printf("usage: exec textviewer.elf abc.txt\n");
		return 0;
	}

	if (ReadFileToBuffer(pcArgument, &stInfo) == FALSE)
	{
		printf("%s file is not found\n", pcArgument);
		return 0;
	}

	GetScreenArea(&stScreenArea);
	iWidth = 500;
	iHeight = 500;
	iX = (GetRectangleWidth(&stScreenArea) - iWidth) / 2;
	iY = (GetRectangleHeight(&stScreenArea) - iHeight) / 2;
	qwWindowID = CreateWindow(iX, iY, iWidth, iHeight, WINDOW_FLAGS_DEFAULT |
			WINDOW_FLAGS_RESIZABLE, "Text Viewer");

	// calc line offset
	CalculateFileOffsetOfLine(iWidth, iHeight, &stInfo);
	stInfo.iCurrentLineIndex = 0;
	DrawTextBuffer(qwWindowID, &stInfo);

	// GUI event handle
	while (1)
	{
		if (ReceiveEventFromWindowQueue(qwWindowID, &stReceivedEvent) == FALSE)
		{
			Sleep(10);
			continue;
		}

		switch (stReceivedEvent.qwType)
		{
			case EVENT_KEY_DOWN:
				pstKeyEvent = &(stReceivedEvent.stKeyEvent);
				if (pstKeyEvent->bFlags & KEY_FLAGS_DOWN)
				{
					switch (pstKeyEvent->bASCIICode)
					{
						case KEY_PAGEUP:
							iMoveLine = -stInfo.iRowCount;
							break;

						case KEY_PAGEDOWN:
							iMoveLine = stInfo.iRowCount;
							break;
							
						case KEY_UP:
							iMoveLine = -1;
							break;

						case KEY_DOWN:
							iMoveLine = 1;
							break;

						default:
							iMoveLine = 0;
							break;
					}

					if (stInfo.iCurrentLineIndex + iMoveLine < 0)
						iMoveLine = -stInfo.iCurrentLineIndex;
					else if (stInfo.iCurrentLineIndex + iMoveLine >= stInfo.iMaxLineCount)
						iMoveLine = stInfo.iMaxLineCount - stInfo.iCurrentLineIndex - 1;

					if (iMoveLine == 0)
						break;

					stInfo.iCurrentLineIndex += iMoveLine;
					DrawTextBuffer(qwWindowID, &stInfo);
				}
				break;

			case EVENT_WINDOW_RESIZE:
				pstWindowEvent = &(stReceivedEvent.stWindowEvent);
				iWidth = GetRectangleWidth(&(pstWindowEvent->stArea));
				iHeight = GetRectangleHeight(&(pstWindowEvent->stArea));

				// save current offset
				dwFileOffset = stInfo.pdwFileOffsetOfLine[stInfo.iCurrentLineIndex];

				// recalc line offset
				CalculateFileOffsetOfLine(iWidth, iHeight, &stInfo);
				stInfo.iCurrentLineIndex = dwFileOffset / stInfo.iColumnCount;

				DrawTextBuffer(qwWindowID, &stInfo);
				break;

			case EVENT_WINDOW_CLOSE:
				DeleteWindow(qwWindowID);
				free(stInfo.pbFileBuffer);
				free(stInfo.pdwFileOffsetOfLine);
				return 0;

			default:
				break;
		}
	}

	return 0;
}

BOOL ReadFileToBuffer(const char* pcFileName, TEXTINFO* pstInfo)
{
	DIR* pstDirectory;
	struct dirent* pstEntry;
	DWORD dwFileSize;
	FILE* pstFile;
	DWORD dwReadSize;

	pstDirectory = opendir("/");
	dwFileSize = 0;

	while (1)
	{
		if ((pstEntry = readdir(pstDirectory)) == NULL)
			break;
		
		if ((strlen(pstEntry->d_name) == strlen(pcFileName)) &&
				(memcmp(pstEntry->d_name, pcFileName, strlen(pcFileName)) == 0))
		{
			dwFileSize = pstEntry->dwFileSize;
			break;
		}
	}

	closedir(pstDirectory);

	if (dwFileSize == 0)
	{
		printf("%s file doesn't exist or size is zero \n", pcFileName);
		return FALSE;
	}

	memcpy(&(pstInfo->vcFileName), pcFileName, sizeof(pstInfo->vcFileName));
	pstInfo->vcFileName[sizeof(pstInfo->vcFileName) - 1] = '\0';

	// save file context
	if ((pstInfo->pdwFileOffsetOfLine = malloc(MAXLINECOUNT * sizeof(DWORD))) == NULL)
	{
		printf("Memory allocation fail\n");
		return FALSE;
	}

	if ((pstInfo->pbFileBuffer = (BYTE*)malloc(dwFileSize)) == NULL)
	{
		printf("Memory %d bytes allocate fail\n", dwFileSize);
		free(pstInfo->pdwFileOffsetOfLine);
		return FALSE;
	}

	pstFile = fopen(pcFileName, "r");
	if ((pstFile != NULL) &&
			(fread(pstInfo->pbFileBuffer, 1, dwFileSize, pstFile) == dwFileSize))
	{
		fclose(pstFile);
		printf("%s file read success\n", pcFileName);
	}
	else
	{
		printf("%s file read fail\n", pcFileName);
		free(pstInfo->pdwFileOffsetOfLine);
		free(pstInfo->pbFileBuffer);
		fclose(pstFile);
		return FALSE;
	}

	pstInfo->dwFileSize = dwFileSize;
	return TRUE;
}

void CalculateFileOffsetOfLine(int iWidth, int iHeight, TEXTINFO* pstInfo)
{
	DWORD i;
	int iLineIndex;
	int iColumnIndex;

	pstInfo->iColumnCount = (iWidth - MARGIN * 2) / FONT_ENGLISHWIDTH;
	pstInfo->iRowCount = (iHeight - (WINDOW_TITLEBAR_HEIGHT * 2) -
			(MARGIN * 2)) / FONT_ENGLISHHEIGHT;
	
	iLineIndex = 0;
	iColumnIndex = 0;
	pstInfo->pdwFileOffsetOfLine[0] = 0;
	for (i = 0; i < pstInfo->dwFileSize; i++)
	{
		if (pstInfo->pbFileBuffer[i] == '\r')
			continue;
		else if (pstInfo->pbFileBuffer[i] == '\t')
			iColumnIndex += TABSPACE - (iColumnIndex % TABSPACE);
		else
			iColumnIndex++;

		if ((iColumnIndex >= pstInfo->iColumnCount) ||
				(pstInfo->pbFileBuffer[i] == '\n'))
		{
			iLineIndex++;
			iColumnIndex = 0;

			if (i + 1 < pstInfo->dwFileSize)
				pstInfo->pdwFileOffsetOfLine[iLineIndex] = i + 1;

			if (iLineIndex >= MAXLINECOUNT)
				break;
		}
	}

	pstInfo->iMaxLineCount = iLineIndex;
}

BOOL DrawTextBuffer(QWORD qwWindowID, TEXTINFO* pstInfo)
{
	DWORD i, j;
	DWORD dwBaseOffset;
	BYTE bTemp;
	int iXOffset, iYOffset;
	int iLineCountToPrint;
	int iColumnCountToPrint;
	char vcBuffer[100];
	RECT stWindowArea;
	int iLength, iWidth;
	int iColumnIndex;

	iXOffset = MARGIN;
	iYOffset = WINDOW_TITLEBAR_HEIGHT;
	GetWindowArea(qwWindowID, &stWindowArea);

	iWidth = GetRectangleWidth(&stWindowArea);
	DrawRect(qwWindowID, 2, iYOffset, iWidth - 3, WINDOW_TITLEBAR_HEIGHT * 2,
			RGB(55, 215, 47), TRUE);

	sprintf(vcBuffer, "File: %s, Line: %d/%d\n", pstInfo->vcFileName,
			pstInfo->iCurrentLineIndex + 1, pstInfo->iMaxLineCount);
	iLength = strlen(vcBuffer);

	DrawText(qwWindowID, (iWidth - iLength * FONT_ENGLISHWIDTH) / 2,
			WINDOW_TITLEBAR_HEIGHT + 2, RGB(255, 255, 255), RGB(55, 215, 47),
			vcBuffer, strlen(vcBuffer));

	// print file context
	iYOffset = (WINDOW_TITLEBAR_HEIGHT * 2) + MARGIN;
	DrawRect(qwWindowID, iXOffset, iYOffset, iXOffset + FONT_ENGLISHWIDTH *
			pstInfo->iColumnCount, iYOffset + FONT_ENGLISHHEIGHT * pstInfo->iRowCount,
			RGB(255, 255, 255), TRUE);

	iLineCountToPrint = MIN(pstInfo->iRowCount, (pstInfo->iMaxLineCount -
				pstInfo->iCurrentLineIndex));
	for (j = 0; j < iLineCountToPrint; j++)
	{
		// line offset to print
		dwBaseOffset = pstInfo->pdwFileOffsetOfLine[pstInfo->iCurrentLineIndex + j];

		iColumnCountToPrint = MIN(pstInfo->iColumnCount,
				(pstInfo->dwFileSize - dwBaseOffset));
		iColumnIndex = 0;
		for (i = 0; (i < iColumnCountToPrint) &&
				(iColumnIndex < pstInfo->iColumnCount); i++)
		{
			bTemp = pstInfo->pbFileBuffer[i + dwBaseOffset];
			if (bTemp == '\n')
				break;
			else if (bTemp == '\t')
				iColumnIndex += TABSPACE - (iColumnIndex % TABSPACE);
			else if (bTemp == '\r')
			{ // nothing
			}
			else
			{
				DrawText(qwWindowID, iColumnIndex * FONT_ENGLISHWIDTH + iXOffset,
						iYOffset + (j * FONT_ENGLISHHEIGHT), RGB(0, 0, 0),
						RGB(255, 255, 255), &bTemp, 1);
				iColumnIndex++;
			}
		}
	}

	ShowWindow(qwWindowID, TRUE);
	return TRUE;
}
