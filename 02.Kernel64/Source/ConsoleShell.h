#ifndef __CONSOLESHELL_H__
#define __CONSOLESHELL_H__

#include "Types.h"


// ----------------------------------------
// Macro
// ----------------------------------------

#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT	300
#define CONSOLESHELL_PROMPTMESSAGE			"MINT64 > "

// define function pointer - receive string pointer type parameter
typedef void (*CommandFunction)(const char* pcParameter);

// package signature
#define PACKAGESIGNATURE					"MINT64OSPACKAGE "

// max file name length : same with FILESYSTEM_MAXFILENAMELENGTH
#define MAXFILENAMELENGTH					24



// ----------------------------------------
// Structure
// ----------------------------------------

#pragma pack(push, 1)


// store shell command

typedef struct kShellCommandEntryStruct
{
	char* pcCommand;				// Command String
	char* pcHelp;					// Command Help
	CommandFunction pfFunction;		// Function Pointer (execute command)
} SHELLCOMMANDENTRY;


// parameter list structure

typedef struct kParameterListStruct
{
	const char* pcBuffer;		// parameter buffer address
	int iLength;				// parameter length
	int iCurrentPosition;		// parameter position to handle
} PARAMETERLIST;


// entry of package file

typedef struct PackageItemStruct
{
	char vcFileName[MAXFILENAMELENGTH];		// file name
	DWORD dwFileLength;						// file size
} PACKAGEITEM;


// package header

typedef struct PackageHeaderStruct
{
	char vcSignature[16];			// signature of MINT64 OS package file
	DWORD dwHeaderSize;				// total size of package header
	PACKAGEITEM vstItem[0];			// start of package item (entry)
} PACKAGEHEADER;


#pragma pack(pop)



// ----------------------------------------
// Function
// ----------------------------------------

// Shell Code

void kStartConsoleShell(void);
void kExecuteCommand(const char* pcCommandBuffer);
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter);
int	 kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter);


// Handling Command

static void kHelp(const char* pcParameterBuffer);
static void kCls(const char* pcParameterBuffer);
static void kShowTotalRAMSize(const char* pcParameterBuffer);
static void kStringToDecimalHexTest(const char* pcParameterBuffer);
static void kShutdown(const char* pcParameterBuffer);

static void kSetTimer(const char* pcParameterBuffer);
static void kWaitUsingPIT(const char* pcParameterBuffer);
static void kReadTimeStampCounter(const char* pcParameterBuffer);
static void kMeasureProcessorSpeed(const char* pcParameterBuffer);
static void kShowDateAndTime(const char* pcParameterBuffer);

static void kCreateTestTask(const char* pcParameterBuffer);

static void kChangeTaskPriority(const char* pcParameterBuffer);
static void kShowTaskList(const char* pcParameterBuffer);
static void kKillTask(const char* pcParameterBuffer);
static void kCPULoad(const char* pcParameterBuffer);

static void kTestMutex(const char* pcParameterBuffer);

static void kCreateThreadTask(void);
static void kTestThread(const char* pcParameterBuffer);
static void kShowMatrix(const char* pcParameterBuffer);

static void kTestPIE(const char* pcParameterBuffer);

static void kShowDynamicMemoryInformation(const char* pcParameterBuffer);
static void kTestSequentialAllocation(const char* pcParameterBuffer);
static void kTestRandomAllocation(const char* pcParameterBuffer);
static void kRandomAllocationTask(void);

static void kShowHDDInformation(const char* pcParameterBuffer);
static void kReadSector(const char* pcParameterBuffer);
static void kWriteSector(const char* pcParameterBuffer);

static void kMountHDD(const char* pcParameterBuffer);
static void kFormatHDD(const char* pcParameterBuffer);
static void kShowFileSystemInformation(const char* pcParameterBuffer);
static void kCreateFileInRootDirectory(const char* pcParameterBuffer);
static void kDeleteFileInRootDirectory(const char* pcParameterBuffer);
static void kShowRootDirectory(const char* pcParameterBuffer);

static void kWriteDataToFile(const char* pcParameterBuffer);
static void kReadDataFromFile(const char* pcParameterBuffer);
static void kTestFileIO(const char* pcParameterBuffer);

static void kFlushCache(const char* pcParameterBuffer);
static void kTestPerformance(const char* pcParameterBuffer);

static void kDownloadFile(const char* pcParameterBuffer);

static void kShowMPConfigurationTable(const char* pcParameterBuffer);

static void kStartApplicationProcessor(const char* pcParameterBuffer);

static void kStartSymmetricIOMode(const char* pcParameterBuffer);
static void kShowIRQINTINMappingTable(const char* pcParameterBuffer);

static void kShowInterruptProcessingCount(const char* pcParameterBuffer);
static void kStartInterruptLoadBalancing(const char* pcParameterBuffer);

static void kStartTaskLoadBalancing(const char* pcParameterBuffer);
static void kChangeTaskAffinity(const char* pcParameterBuffer);

static void kShowVBEModeInfo(const char* pcParameterBuffer);

static void kTestSystemCall(const char* pcParameterBuffer);

static void kExecuteApplicationProgram(const char* pcParameterBuffer);

static void kInstallPackage(const char* pcParameterBuffer);



#endif
