#ifndef __CONSOLESHELL_H__
#define __CONSOLESHELL_H__

#include "Types.h"

#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT	300
#define CONSOLESHELL_PROMPTMESSAGE			"MINT64 > "

typedef void (* CommandFunction)(const char* pcParameter);

#pragma pack(push, 1)

typedef struct kShellCommandEntryStruct
{
	char* pcCommand;	// Command String
	char* pcHelp;		// Help String
	CommandFunction pfFunction;		// Function Pointer to Execute
} SHELLCOMMANDENTRY;

typedef struct kParameterListStruct
{
	const char* pcBuffer;	// parameter buffer
	int iLength;			// parameter length
	int iCurrentPosition;	// start position of current parameter
} PARAMETERLIST;

#pragma pack(pop)

void kStartConsoleShell(void);
void kExecuteCommand(const char* pcCommandBuffer);
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter);
int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter);

static void kHelp(const char* pcParameterBuffer);
static void kCls(const char* pcParameterBuffer);
static void kShowTotalRAMSize(const char* pcParameterBuffer);
static void kShutdown(const char* pcParameterBuffer);
static void kMeasureProcessorSpeed(const char* pcParameterBuffer);
static void kShowDateAndTime(const char* pcParameterBuffer);

// Test Task functions
static void kChangeTaskPriority(const char* pcParameterBuffer);
static void kShowTaskList(const char* pcParameterBuffer);
static void kKillTask(const char* pcParameterBuffer);
static void kCPULoad(const char* pcParameterBuffer);

// Test Mutex functions
static void kShowMatrix(const char* pcParameterBuffer);

// Test Memory Allocation Functions
static void kShowDynamicMemoryInformation(const char* pcParameterBuffer);

// PATA Driver Functions
static void kShowHDDInformation(const char* pcParameterBuffer);
static void kReadSector(const char* pcParameterBuffer);
static void kWriteSector(const char* pcParameterBuffer);

// File System Functions
static void kMountHDD(const char* pcParameterBuffer);
static void kFormatHDD(const char* pcParameterBuffer);
static void kShowFileSystemInformation(const char* pcParameterBuffer);
static void kCreateFileInRootDirectory(const char* pcParameterBuffer);
static void kDeleteFileInRootDirectory(const char* pcParameterBuffer);
static void kShowRootDirectory(const char* pcParameterBuffer);
static void kWriteDataToFile(const char* pcParameterBuffer);
static void kReadDataFromFile(const char* pcParameterBuffer);
static void kFlushCache(const char* pcParameterBuffer);

// Serial Port
static void kDownloadFile(const char* pcParameterBuffer);

// Multi Processor
static void kShowMPConfigurationTable(const char* pcParameterBuffer);
static void kShowIRQINTINMappingTable(const char* pcParameterBuffer);
static void kShowInterruptProcessingCount(const char* pcParameterBuffer);
static void kChangeTaskAffinity(const char* pcParameterBuffer);

// Graphic Mode
static void kShowVBEModeInfo(const char* pcParameterBuffer);

// System call
static void kTestSystemCall(const char* pcParameterBuffer);
static void kExecuteApplicationProgram(const char* pcParameterBuffer);

#endif
