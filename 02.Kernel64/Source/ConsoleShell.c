#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "AssemblyUtility.h"
#include "Task.h"
#include "Synchronization.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"
#include "MPConfigurationTable.h"
#include "LocalAPIC.h"
#include "MultiProcessor.h"
#include "IOAPIC.h"
#include "InterruptHandler.h"
#include "VBE.h"
#include "SystemCall.h"
#include "Loader.h"



// Define Command Table
SHELLCOMMANDENTRY gs_vstCommandTable[] = 
{
	{"help"		, "Show Help"					, kHelp						},
	{"cls" 		, "Clear Screen"				, kCls						},
	{"totalram"	, "Show Total RAM Size"			, kShowTotalRAMSize			},
	//{"strtod"	, "String To Dec/Hex Convert"	, kStringToDecimalHexTest	},
	{"shutdown"	, "Shutdown And Reboot OS"		, kShutdown					},	

	{"settimer" , "Set PIT Controller Counter 0, ex)settimer 10(ms) 1(periodic)", kSetTimer},
	//{"wait"		, "Wait ms Using PIT, ex) wait 100(ms)", kWaitUsingPIT 		},
	//{"rdtsc"	, "Read Time Stamp Counter"		, kReadTimeStampCounter		},
	{"cpuspeed" , "Measure Processor Speed"		, kMeasureProcessorSpeed	},
	{"date"		, "Show Date And Time"			, kShowDateAndTime			},

	//{"createtask", "Create Task, ex) createtask 1(type) 10(count)", kCreateTestTask},

	{"changepriority", "Change Task Priority, ex)changepriority 1(ID) 2(Priority)", kChangeTaskPriority},
	{"tasklist", "Show Task List", kShowTaskList},
	{"killtask", "End Task, ex)killtask 1(ID)", kKillTask},
	{"cpuload", "Show Processor Load", kCPULoad},
	
	//{"testmutex", "Test Mutex Function", kTestMutex},
	
	//{"testthread", "Test Thread And Process Function", kTestThread},
	{"showmatrix", "Show Matrix Screen", kShowMatrix},

	//{"testpie", "Test PIE Calculation", kTestPIE},

	{"dynamicmeminfo", "Show Dynamic Memory Information", kShowDynamicMemoryInformation},
	//{"testseqalloc", "Test Sequential Allocation & Free", kTestSequentialAllocation},
	{"testranalloc", "Test Random Allocation & Free"	, kTestRandomAllocation},

	{"hddinfo", "Show HDD Information", kShowHDDInformation},
	{"readsector", "Read HDD Sector, ex)readsector 0(LBA) 10(Count)", kReadSector},
	{"writesector", "Write HDD Sector, ex)writesector 0(LBA) 10(Count)", kWriteSector},

	{"mounthdd", "Mount HDD", kMountHDD},
	{"formathdd", "Format HDD", kFormatHDD},
	{"filesysteminfo", "Show File System Information", kShowFileSystemInformation},
	{"createfile", "Create File, ex)createfile a.txt", kCreateFileInRootDirectory},
	{"deletefile", "Delete File, ex)deletefile a.txt", kDeleteFileInRootDirectory},
	{"dir", "Show Directory", kShowRootDirectory},
	
	{"writefile", "Write Data To File, ex)writefile a.txt", kWriteDataToFile},
	{"readfile", "Read Data From File, ex)readfile a.txt", kReadDataFromFile},
	//{"testfileio", "Test File I/O Function", kTestFileIO},

	//{"testperformance", "Test File Read/Write Performance", kTestPerformance},
	{"flush", "Flush File System Cache", kFlushCache},

	{"download", "Download Data From Serial, ex)download a.txt", kDownloadFile},

	{"showmpinfo", "Show MP Configuration Table Information", kShowMPConfigurationTable},

	//{"startap", "Start Application Processor", kStartApplicationProcessor},

	//{"startsymmetricio", "Start Symmetric I/O Mode", kStartSymmetricIOMode},
	{"showirqintinmap", "Show IRQ->INTIN Mapping Table", kShowIRQINTINMappingTable},

	{"showintproccount", "Show Interrupt Processing Count", kShowInterruptProcessingCount},
	//{"startintloadbal", "Start Interrupt Load Balancing", kStartInterruptLoadBalancing},

	//{"starttaskloadbal", "Start Task Load Balancing", kStartTaskLoadBalancing},
	{"changeaffinity", "Change Task Affinity, ex)changeaffinity 1(ID) 0xFF(Affinity)", kChangeTaskAffinity},

	{"vbemodeinfo", "Show VBE Mode Information", kShowVBEModeInfo},

	{"testsystemcall", "Test System Call Operation", kTestSystemCall},

	{"exec", "Execute Application Program, ex)exec a.elf argument", kExecuteApplicationProgram},

	{"installpackage", "Install Package To HDD", kInstallPackage},
};



// =========================================================
// Configuring Shell
// =========================================================


// Shell Main Loop
void kStartConsoleShell(void)
{
	char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
	int	 iCommandBufferIndex = 0;
	BYTE bKey;
	int  iCursorX, iCursorY;
	CONSOLEMANAGER* pstConsoleManager;


	// get Console Manager
	pstConsoleManager = kGetConsoleManager();


	// print Prompt
	kPrintf(CONSOLESHELL_PROMPTMESSAGE);

	
	// loop until bExit set
	while(pstConsoleManager->bExit == FALSE)
	{
		bKey = kGetCh();


		// if bExit flag set, break loop
		if(pstConsoleManager->bExit == TRUE)
		{
			break;
		}


		// Back Space handling
		if(bKey == KEY_BACKSPACE)
		{
			if(iCommandBufferIndex > 0)
			{
				kGetCursor(&iCursorX, &iCursorY);
				
				kPrintStringXY(iCursorX - 1, iCursorY, " ");	// print space in front of currnet pos
				
				kSetCursor(iCursorX - 1, iCursorY);
				iCommandBufferIndex--;		// erase one char in command buffer
			}
		}

		// Enter handling
		else if(bKey == KEY_ENTER)
		{
			kPrintf("\n");

			if(iCommandBufferIndex > 0)		// if there's command string..
			{
				vcCommandBuffer[iCommandBufferIndex] = '\0';
				kExecuteCommand(vcCommandBuffer);
			}

			kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);

			// flush and initialize Command Buffer
			kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
			iCommandBufferIndex = 0;
		}

		// ignore Shift, Caps Lock, Num Lock, Scroll Lock
		else if( (bKey == KEY_LSHIFT) 	|| (bKey == KEY_RSHIFT)  ||
				 (bKey == KEY_CAPSLOCK) || (bKey == KEY_NUMLOCK) || (bKey == KEY_SCROLLLOCK) )
		{
			;
		}
		else if(bKey < 128)
		{
			if(bKey == KEY_TAB)		// handle TAB as "Space"
			{
				bKey = ' ';
			}


			// print, store command if command buffer is empty
			if(iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT)	
			{
				vcCommandBuffer[iCommandBufferIndex++] = bKey;
				kPrintf("%c", bKey);
			}
		}
	}
}


// Compare Command with Command Buffer, Execute Command Handler (Function)
void kExecuteCommand(const char* pcCommandBuffer)
{
	int i, iSpaceIndex;
	int iCommandBufferLength, iCommandLength;
	int iCount;

	
	// extract Command seperated by ' '
	iCommandBufferLength = kStrLen(pcCommandBuffer);

	for(iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++)
	{
		if(pcCommandBuffer[iSpaceIndex] == ' ')
			break;
	}

	
	// check Command Table if there's matching command
	iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);	// Total number of commands

	for(i=0; i<iCount; i++)
	{
		iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);
		
		if( (iCommandLength == iSpaceIndex) && 
				(kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0) )
		{
			gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1);	// (*pfFunction)(const char* pcParameterBuffer)
			break;
		}
	}

	if(i >= iCount)
		kPrintf("'%s' is not found.\n", pcCommandBuffer);
}


// Initialize Parameter Data Structure
void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameter)
{
	pstList->pcBuffer 		  = pcParameter;
	pstList->iLength  		  = kStrLen(pcParameter);
	pstList->iCurrentPosition = 0;
}


// return contents, length of parameter seperated by space
int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter)
{
	int i;
	int iLength;

	// if there's no longer parameter, return 0
	if(pstList->iLength <= pstList->iCurrentPosition)
		return 0;

	// move as much as Buffer Length, search Space
	for(i = pstList->iCurrentPosition; i < pstList->iLength; i++)
	{
		if(pstList->pcBuffer[i] == ' ')
			break;
	}

	// copy Parameter, return Length
	kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
	iLength = i - pstList->iCurrentPosition;
	pcParameter[iLength] = '\0';

	// update parameter pos
	pstList->iCurrentPosition += iLength + 1;

	return iLength;
}




// =========================================================
// Handling Command
// =========================================================


// print Shell Help
static void kHelp(const char* pcParameterBuffer)
{
	int i;
	int iCount;
	int iCursorX, iCursorY;
	int iLength, iMaxCommandLength = 0;

	kPrintf("=====================================================\n");
	kPrintf("                 MINT 64 Shell Help                  \n");
	kPrintf("=====================================================\n");

	iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);		// Total number of Command


	// calculate length of the longest command
	for(i=0; i<iCount; i++)
	{
		iLength = kStrLen(gs_vstCommandTable[i].pcCommand);

		if(iLength > iMaxCommandLength)
			iMaxCommandLength = iLength;
	}


	// print Help
	for(i=0; i<iCount; i++)
	{
		kPrintf("%s", gs_vstCommandTable[i].pcCommand);
		kGetCursor(&iCursorX, &iCursorY);
		kSetCursor(iMaxCommandLength, iCursorY);
		kPrintf(" - %s\n", gs_vstCommandTable[i].pcHelp);

		if( (i != 0) && ((i % 20) == 0) )
		{
			kPrintf("Press any key to continue... ('q' is exit) : ");

			if(kGetCh() == 'q')
			{
				kPrintf("\n");
				break;
			}

			kPrintf("\n");
		}
	}
}


// clear screen
static void kCls(const char* pcParameterBuffer)
{
	kClearScreen();		// first line is for debugging, erase and set cursor X,Y to 0, 1
	kSetCursor(0, 1);
}


// print Total Memory Size
static void kShowTotalRAMSize(const char* pcParameterBuffer)
{
	kPrintf("Total RAM Size : %d MB\n", kGetTotalRAMSize());
}


// convert char type number to int type number, print converted one
static void kStringToDecimalHexTest(const char* pcParameterBuffer)
{
	char vcParameter[100];
	int  iLength;
	PARAMETERLIST stList;
	int  iCount = 0;

	long lValue;


	// initialize Parameter
	kInitializeParameter(&stList, pcParameterBuffer);


	while(1)
	{
		// get Next Parameter, if parameter length is 0, it means no parameter exist
		iLength = kGetNextParameter(&stList, vcParameter);

		if(iLength == 0)
			break;

		// print parameter information, convert to DEC/HEX
		kPrintf("Param %d = '%s', Length = %d, ", iCount + 1, vcParameter, iLength);

		// start with 0x : HEX, else : DEC
		if(kMemCmp(vcParameter, "0x", 2) == 0)
		{
			lValue = kAToI(vcParameter + 2, 16);
			kPrintf("Hex Value = %q\n", lValue);
		}
		else
		{
			lValue = kAToI(vcParameter, 10);
			kPrintf("Decimal Value = %d\n", lValue);
		}

		iCount++;
	}
}


// restart PC
static void kShutdown(const char* pcParameterBuffer)
{
	kPrintf("System Shutdown Start..\n");


	// move File System Cache contents to HDD
	kPrintf("Cache Flush... ");
	if(kFlushFileSystemCache() == TRUE)
	{
		kPrintf("Pass\n");
	}
	else
	{
		kPrintf("Fail\n");
	}


	// restart using Keyboard Controller
	kPrintf("Press Any Key To Reboot PC.");
	kGetCh();
	kReboot();
}



// set value of Counter 0 in PIC Controller
static void kSetTimer(const char* pcParameterBuffer)
{
	char vcParameter[100];
	PARAMETERLIST stList;
	long lValue;
	BOOL bPeriodic;


	// Initialize Parameter
	kInitializeParameter(&stList, pcParameterBuffer);


	// extract millisecond (param 1)
	if(kGetNextParameter(&stList, vcParameter) == 0)
	{
		kPrintf("ex) settimer 10(ms) 1(periodic)\n");
		return;
	}
	lValue = kAToI(vcParameter, 10);


	// extract periodic (param 2)
	if(kGetNextParameter(&stList, vcParameter) == 0)
	{
		kPrintf("ex) settimer 10(ms) 1(periodic)\n");
		return;
	}
	bPeriodic = kAToI(vcParameter, 10);

	
	kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
	kPrintf("Time = %d ms, Periodic = %d, Change Complete\n", lValue, bPeriodic);
}


// wait ms using PIT Controller
static void kWaitUsingPIT(const char* pcParameterBuffer)
{
	char vcParameter[100];
	int  iLength;
	PARAMETERLIST stList;
	long lMillisecond;
	int  i;


	// Initialize Parameter
	kInitializeParameter(&stList, pcParameterBuffer);

	
	// parameter 1 must exist
	if(kGetNextParameter(&stList, vcParameter) == 0)
	{
		kPrintf("ex) wait 100(ms)\n");
		return;
	}

	lMillisecond = kAToI(pcParameterBuffer, 10);
	kPrintf("%d ms Sleep Start...\n", lMillisecond);


	// Deactivate Interrupt, Measure time directly using PIT Controller
	kDisableInterrupt();
	
	for(i=0; i<lMillisecond/30; i++)
		kWaitUsingDirectPIT(MSTOCOUNT(30));			// divide lMilisecond by 30 ms
	kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));

	kEnableInterrupt();

	kPrintf("%d ms Sleep Complete\n", lMillisecond);


	// restore Timer
	kInitializePIT(MSTOCOUNT(1), TRUE);
}


// read Time Stamp Counter
static void kReadTimeStampCounter(const char* pcParameterBuffer)
{
	QWORD qwTSC;

	qwTSC = kReadTSC();
	kPrintf("Time Stamp Counter = %q\n", qwTSC);
}


// measure Processor Speed
static void kMeasureProcessorSpeed(const char* pcParameterBuffer)
{
	int i;
	QWORD qwLastTSC, qwTotalTSC = 0;

	kPrintf("Now Measuring");


	// Measure Processor Speed using TimeStamp Differece for 10 second
	kDisableInterrupt();
	
	for(i=0; i<200; i++)	// 50 ms * 200 = 10 s
	{
		qwLastTSC = kReadTSC();
		kWaitUsingDirectPIT(MSTOCOUNT(50));
		qwTotalTSC += kReadTSC() - qwLastTSC;

		kPrintf(".");
	}

	// restore Timer
	kInitializePIT(MSTOCOUNT(1), TRUE);

	kEnableInterrupt();
	
	kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC/10/1000/1000);		// qwTotalTSC / 10s / khz / mhz
}


// print Date / Time stored in RTC Controller
static void kShowDateAndTime(const char* pcParameterBuffer)
{
	BYTE bSecond, bMinute, bHour;
	BYTE bDayOfWeek, bDayOfMonth, bMonth;
	WORD wYear;
	

	// read Date / Time from RTC Controller
	kReadRTCTime(&bHour, &bMinute, &bSecond);
	kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

	
	kPrintf("Date : %d/%d/%d %s, ", wYear, bMonth, bDayOfMonth, kConvertDayOfWeekToString(bDayOfWeek));
	kPrintf("Time : %d:%d:%d\n", bHour, bMinute, bSecond);
}



// Task 1 : print char going around edge of screen
static void kTestTask1(void)
{
	BYTE bData;
	int i=0, iX=0, iY=0, iMargin, j;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;

	pstRunningTask = kGetRunningTask(kGetAPICID());
	iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

	for(j=0; j<20000; j++)
	{
		switch(i)
		{
			case 0:
				iX++;
				if(iX >= (CONSOLE_WIDTH - iMargin))
				{
					i = 1;
				}
				break;

			case 1:
				iY++;
				if(iY >= (CONSOLE_HEIGHT - iMargin))
				{
					i = 2;
				}
				break;

			case 2:
				iX--;
				if(iX < iMargin)
				{
					i = 3;
				}
				break;

			case 3:
				iY--;
				if(iY < iMargin)
				{
					i = 0;
				}
				break;

		}

		pstScreen[iY * CONSOLE_WIDTH + iX].bCharactor = bData;
		pstScreen[iY * CONSOLE_WIDTH + iX].bAttribute = bData & 0x0F;
		bData++;

		//kSchedule();
	}

	kExitTask();
}


// Task 2 : print pinwheel referring to itself Process ID
static void kTestTask2(void)
{
	int i = 0, iOffset;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;
	char vcData[4] = {'-', '\\', '|', '/'};

	pstRunningTask = kGetRunningTask(kGetAPICID());
	iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
	iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	while(1)
	{
		pstScreen[iOffset].bCharactor = vcData[i % 4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
		i++;

		//kSchedule();
	}
}


// Task 3 : print its Task ID, Core ID whenever Core ID executing Task changed
static void kTestTask3(void)
{
	QWORD qwTaskID;
	TCB*  pstRunningTask;
	BYTE  bLastLocalAPICID;
	QWORD qwLastTick;


	pstRunningTask = kGetRunningTask(kGetAPICID());
	qwTaskID = pstRunningTask->stLink.qwID;
	
	kPrintf("Test Task 3 Started. Task ID = 0x%Q, Local APIC ID = 0x%X\n", qwTaskID, kGetAPICID());


	// save Current Local APIC ID.
	// print message when Task moved to other Core by Task Load Balancing
	bLastLocalAPICID = kGetAPICID();


	while(1)
	{
		// if Previous Local APIC ID != Current Local APIC ID, print message
		if(bLastLocalAPICID != kGetAPICID())
		{
			kPrintf("Core Changed, Task ID = 0x%Q, Previous Local APIC ID = 0x%X "
					"Current = 0x%X", qwTaskID, bLastLocalAPICID, kGetAPICID());

			// change Current Core ID
			bLastLocalAPICID = kGetAPICID();
		}

		kSchedule();
	}
}


// Create Task for Multitasking
static void kCreateTestTask(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcType[30];
	char vcCount[30];
	int i;

	// extract Parameter
	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcType);
	kGetNextParameter(&stList, vcCount);


	switch(kAToI(vcType, 10))
	{
		// Create kTestTask1
		case 1:
			for(i=0; i<kAToI(vcCount, 10); i++)
			{
				if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask1, TASK_LOADBALANCINGID) == NULL)
				{
					break;
				}
			}
			
			kPrintf("Task 1 [%d] Created\n", i);
			break;

		// Create kTestTask2
		case 2:
		default:
			for(i=0; i<kAToI(vcCount, 10); i++)
			{
				if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2, TASK_LOADBALANCINGID) == NULL)
				{
					break;
				}
			}

			kPrintf("Task 2 [%d] Created\n", i);
			break;

		// Create kTestTask3
		case 3:
			for(i=0; i<kAToI(vcCount, 10); i++)
			{
				if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask3, TASK_LOADBALANCINGID) == NULL)
				{
					break;
				}

				kSchedule();
			}

			kPrintf("Task 3 [%d] Created\n", i);
			break;
	}
}



// Change Task Priority
static void kChangeTaskPriority(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcID[30];
	char vcPriority[30];
	QWORD qwID;
	BYTE bPriority;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcID);
	kGetNextParameter(&stList, vcPriority);


	// Change Task Priority
	if(kMemCmp(vcID, "0x", 2) == 0)
		qwID = kAToI(vcID+2, 16);
	else
		qwID = kAToI(vcID, 10);

	bPriority = kAToI(vcPriority, 10);


	kPrintf("Change Task Priority ID [0x%q] Priority[%d] ", qwID, bPriority);

	if(kChangePriority(qwID, bPriority) == TRUE)
		kPrintf("Success\n");
	else
		kPrintf("Fail\n");
}


// Print list of created Task
static void kShowTaskList(const char* pcParameterBuffer)		// FIXME
{
	int i;
	TCB* pstTCB;
	int iCount = 0;
	int iTotalTaskCount = 0;
	char vcBuffer[20];
	int iRemainLength;
	int iProcessorCount;


	/*
	// add Task count in each Scheduler
	iProcessorCount = kGetProcessorCount();

	for(i=0; i<iProcessorCount; i++)
	{
		iTotalTaskCount += kGetTaskCount(i);
	}

	kPrintf("========== Task Total Count [%d] ==========\n", iTotalTaskCount);
	*/

	kPrintf("========== Task Count per Core==========\n");


	// if Core count is more than 2, print Task count of each Scheduler
	if(iProcessorCount > 1)
	{
		//for(i=0; i<iProcessorCount; i++)
		for(i=0; i<MAXPROCESSORCOUNT; i++)
		{
			if( (i != 0) && ((i % 4) == 0) )
			{
				kPrintf("\n");
			}

			kSPrintf(vcBuffer, "Core %d : %d", i, kGetTaskCount(i));
			kPrintf(vcBuffer);


			// fill remaining space with space
			iRemainLength = 19 - kStrLen(vcBuffer);
			kMemSet(vcBuffer, ' ', iRemainLength);
			vcBuffer[iRemainLength] = '\0';
			kPrintf(vcBuffer);
		}

		kPrintf("\nPress any key to continue... ('q' is exit) : ");
		
		if(kGetCh() == 'q')
		{
			kPrintf("\n");
			return;
		}

		kPrintf("\n\n");
	}


	for(i=0; i<TASK_MAXCOUNT; i++)
	{
		// get TCB, print Task ID in use
		pstTCB = kGetTCBInTCBPool(i);

		if((pstTCB->stLink.qwID >> 32) != 0)
		{
			if( (iCount != 0) && ((iCount % 6) == 0) )
			{
				kPrintf("Press any key to continue... ('q' is quit) : ");
				
				if(kGetCh() == 'q')
				{
					kPrintf("\n");
					break;
				}
				kPrintf("\n");
			}
			
			kPrintf("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n", 
					1 + iCount++, pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags), pstTCB->qwFlags, 
					kGetListCount(&(pstTCB->stChildThreadList)));
			kPrintf("	Core ID[0x%X], CPU Affinity[0x%X]\n", pstTCB->bAPICID, pstTCB->bAffinity);
			kPrintf("	Parent PID[0x%Q], Memory Address[0x%Q], Size[[0x%Q]\n",
						pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize);
		}
	}
}


// End Task
static void kKillTask(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcID[30];
	QWORD qwID;
	TCB* pstTCB;
	int i;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcID);


	// end Task
	if(kMemCmp(vcID, "0x", 2) == 0)
		qwID = kAToI(vcID + 2, 16);
	else
		qwID = kAToI(vcID, 10);

	// end Specific Task
	if(qwID != 0xFFFFFFFF)
	{
		pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
		qwID = pstTCB->stLink.qwID;

		if( ((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00) )
		{
			kPrintf("Kill Task ID [0x%q] ", qwID);

			if(kEndTask(qwID) == TRUE)
				kPrintf("Success\n");
			else
				kPrintf("Fail\n");
		}
		else
		{
			kPrintf("Task does not exist or Task is System task\n");
		}
	}
	else	// Kill all Task except System task
	{
		for(i=0; i<TASK_MAXCOUNT; i++)
		{
			pstTCB = kGetTCBInTCBPool(i);
			qwID = pstTCB->stLink.qwID;

			if( ((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00) )	// if allocated and not System task
			{
				kPrintf("Kill Task ID [0x%q] ", qwID);

				if(kEndTask(qwID) == TRUE)
					kPrintf("Success\n");
				else
					kPrintf("Fail\n");
			}
		}
	}
}


// Print percentage of CPU Usage
static void kCPULoad(const char* pcParameterBuffer)			// FIXME
{
	int i;
	char vcBuffer[50];
	int iRemainLength;


	kPrintf("=============== Process Load ===============\n");

	// print Load per Core
	//for(i=0; i<kGetProcessorCount(); i++)
	for(i=0; i<MAXPROCESSORCOUNT; i++)
	{
		if( (i != 0) && ((i % 4) == 0) )
		{
			kPrintf("\n");
		}

		kSPrintf(vcBuffer, "Core %d : %d%%", i, kGetProcessorLoad(i));
		kPrintf("%s", vcBuffer);


		// fill remaining space with space
		iRemainLength = 19 - kStrLen(vcBuffer);
		kMemSet(vcBuffer, ' ', iRemainLength);
		vcBuffer[iRemainLength] = '\0';
		kPrintf(vcBuffer);
	}

	kPrintf("\n");
}



// Mutex and Variable for testing
static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;


static void kPrintNumberTask(void)
{
	int i;
	int j;
	QWORD qwTickCount;

	// wait 50ms not to overlapped with Console Shell Message
	qwTickCount = kGetTickCount();
	while((kGetTickCount() - qwTickCount) < 50)
	{
		kSchedule();
	}

	for(i=0; i<5; i++)
	{
		kLock(&(gs_stMutex));		// CRITICAL SECTION START
		
		kPrintf("Task ID [0x%q] Value : %d\n", kGetRunningTask(kGetAPICID())->stLink.qwID, gs_qwAdder);
		gs_qwAdder += 1;

		kUnlock(&(gs_stMutex));		// CRITICAL SECTION END
	}

	// wait N sec for all task ended
	qwTickCount = kGetTickCount();
	while((kGetTickCount() - qwTickCount) < 3000)
	{
		kSchedule();
	}

	kExitTask();
}


// Create Task to test Mutex
static void kTestMutex(const char* pcParameterBuffer)
{
	int i;
	gs_qwAdder = 1;

	kInitializeMutex(&(gs_stMutex));

	for(i=0; i<3; i++)
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kPrintNumberTask, kGetAPICID());

	kPrintf("Wait Until %d Task End...\n", i);

	kGetCh();
}



// create Task 2 as its Thread
static void kCreateThreadTask(void)
{
	int i;

	for(i=0; i<3; i++)
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2, TASK_LOADBALANCINGID);

	while(1)
	{
		kSleep(1);
	}
}


// create Task to test Thread
static void kTestThread(const char* pcParameterBuffer)
{
	TCB* pstProcess;

	pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xEEEEEEEE, 0x1000, 
							 (QWORD)kCreateThreadTask, TASK_LOADBALANCINGID);
	
	if(pstProcess != NULL)
		kPrintf("Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);
	else
		kPrintf("Process Create Fail\n");
}


static volatile QWORD gs_qwRandomValue = 0;


QWORD kRandom(void)		// return Random value
{
	gs_qwRandomValue = (gs_qwRandomValue * 412153 + 5571031) >> 16;
	
	return gs_qwRandomValue;
}


static void kDropCharactorThread(void)
{
	int iX, iY;
	int i;
	char vcText[2] = {0, };

	iX = kRandom() % CONSOLE_WIDTH;

	while(1)
	{
		// wait for a while
		kSleep(kRandom() % 20);

		if( (kRandom() % 20) < 15 )		// print Space
		{
			vcText[0] = ' ';

			for(i=0; i<CONSOLE_HEIGHT-1; i++)
			{
				kPrintStringXY(iX, i, vcText);
				kSleep(50);
			}
		}
		else							// print Random Char
		{
			for(i=0; i<CONSOLE_HEIGHT-1; i++)
			{
				vcText[0] = (i + kRandom()) % 128;
				kPrintStringXY(iX, i, vcText);
				kSleep(50);
			}
		}
	}
}


static void kMatrixProcess(void)		// Contains 300 Threads - kDropCharactorThread()
{
	int i;

	for(i=0; i<300; i++)
	{
		if(kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, 
					   (QWORD)kDropCharactorThread, TASK_LOADBALANCINGID) == NULL)
		{
			break;
		}


		kSleep(kRandom() % 5 + 5);
	}

	kPrintf("%d Thread is created\n", i);

	// Press a key to end Process
	kGetCh();
}


static void kShowMatrix(const char* pcParameterBuffer)
{
	TCB* pstProcess;

	pstProcess = kCreateTask(TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, (void*)0xE00000, 0xE00000, 
							 (QWORD)kMatrixProcess, TASK_LOADBALANCINGID);

	if(pstProcess != NULL)
	{
		kPrintf("Matrix Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);

		// wait until Task end
		while( (pstProcess->stLink.qwID >> 32) != 0 )
		{
			kSleep(100);
		}
	}
	else
	{
		kPrintf("Matrix Process Create Fail\n");
	}
}



static void kFPUTestTask(void)
{
	double dValue1;
	double dValue2;
	TCB* pstRunningTask;
	QWORD qwCount = 0;
	QWORD qwRandomValue;
	int i;
	int iOffset;
	char vcData[4] = {'-', '\\', '|', '/'};
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	
	pstRunningTask = kGetRunningTask(kGetAPICID());

	// Screen Offset
	iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;		// * 2 : Char | Attribute
	iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));


	while(1)
	{
		dValue1 = 1;
		dValue2 = 1;
		
		for(i=0; i<10; i++)
		{
			qwRandomValue = kRandom();	
			dValue1 *= (double)qwRandomValue;
			dValue2 *= (double)qwRandomValue;
			
			kSleep(1);

			qwRandomValue = kRandom();
			dValue1 /= (double)qwRandomValue;
			dValue2 /= (double)qwRandomValue;
		}

		if(dValue1 != dValue2)
		{
			kPrintf("Value Is Not Same [%f] != [%f]\n", dValue1, dValue2);
			break;
		}

		qwCount++;

		pstScreen[iOffset].bCharactor = vcData[qwCount % 4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
	}
}


static void kTestPIE(const char* pcParameterBuffer)
{
	double dResult;
	int i;

	kPrintf("PIE Calculation Test\n");
	kPrintf("Result : 355 / 113 = ");
	
	dResult = (double)355/113;

	kPrintf("%d.%d%d\n", (QWORD)dResult, ((QWORD)(dResult*10) % 10), ((QWORD)(dResult*100) % 10));

	
	// Create Task calculating Floating Point
	for(i=0; i<100; i++)
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kFPUTestTask, TASK_LOADBALANCINGID);
}



static void kShowDynamicMemoryInformation(const char* pcParameterBuffer)
{
	QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;

	kGetDynamicMemoryInformation(&qwStartAddress, &qwTotalSize, &qwMetaSize, &qwUsedSize);

	// size unit : byte

	kPrintf("========== Dynamic Memory Information ==========\n");
	kPrintf("Start Address : [0x%Q]\n", qwStartAddress);
	kPrintf("Total Size : [0x%Q] Byte, [%d] MB\n", qwTotalSize, qwTotalSize/1024/1024);
	kPrintf("Meta Size : [0x%Q] Byte, [%d] KB\n", qwMetaSize, qwMetaSize/1024);
	kPrintf("Used Size : [0x%Q] Byte, [%d] KB\n", qwUsedSize, qwUsedSize/1024);
}


// Sequential Memory Allocation in Block List and Free
static void kTestSequentialAllocation(const char* pcParameterBuffer)
{
	DYNAMICMEMORY* pstMemory;
	long i, j, k;
	QWORD* pqwBuffer;

	
	kPrintf("========== Dynamic Memory Test ==========\n");
	pstMemory = kGetDynamicMemoryManager();


	// Level loop
	for(i=0; i<pstMemory->iMaxLevelCount; i++)
	{
		kPrintf("Block List [%d] Test Start\n", i);
		kPrintf("Allocation And Compare : ");

		// ****************************************************
		// Allocate all blocks and Check by filling value
		// ****************************************************
		
		// Block loop 
		for(j=0; j<(pstMemory->iBlockCountOfSmallestBlock >> i); j++)
		{
			pqwBuffer = kAllocateMemory(DYNAMICMEMORY_MIN_SIZE << i);

			if(pqwBuffer == NULL)
			{
				kPrintf("\nAllocation Fail\n");
				return;
			}

			// **************************************
			// fill value and then check
			// **************************************

			for(k=0; k<(DYNAMICMEMORY_MIN_SIZE << i)/8; k++)
			{
				pqwBuffer[k] = k;
			}

			for(k=0; k<(DYNAMICMEMORY_MIN_SIZE << i)/8; k++)
			{
				if(pqwBuffer[k] != k)
				{
					kPrintf("\nCompare Fail\n");
					return;
				}
			}

			// Procedure print
			kPrintf(".");

		}	// Block loop end
		

		kPrintf("\nFree : ");

		// Free allocated block
		for(j=0; j<(pstMemory->iBlockCountOfSmallestBlock >> i); j++)
		{
			if(kFreeMemory( (void*)(pstMemory->qwStartAddress + (DYNAMICMEMORY_MIN_SIZE << i) * j) ) == FALSE)
			{
				kPrintf("\nFree Fail\n");
				return;
			}

			kPrintf(",");
		}
		
		kPrintf("\n");
	}	// Level loop end

	kPrintf("Test Complete\n");
}


// Random Memory Allocation in Block List and Free
static void kTestRandomAllocation(const char* pcParameterBuffer)
{
	int i;

	for(i=0; i<1000; i++)
	{
		kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, 
					(QWORD)kRandomAllocationTask, TASK_LOADBALANCINGID);
	}
}


// The Task which repeat random allocation and free
static void kRandomAllocationTask(void)
{
	TCB* pstTask;
	QWORD qwMemorySize;
	char vcBuffer[200];
	BYTE* pbAllocationBuffer;
	int i, j;
	int iY;

	pstTask = kGetRunningTask(kGetAPICID());
	iY = (pstTask->stLink.qwID) % 15 + 9;		// iY = 9 ~ 23


	// 10 loops
	for(j=0; j<10; j++)
	{
		// allocate from 1KB to 32MB
		do
		{
			qwMemorySize = ((kRandom() % (32 * 1024)) + 1) * 1024;	// unit : byte
			pbAllocationBuffer = kAllocateMemory(qwMemorySize);

			// if not allocated, other Task may use memory
			// 		so, wait for a while and then retry
			if(pbAllocationBuffer == 0)
				kSleep(1);

		} while(pbAllocationBuffer == 0);

		kSPrintf(vcBuffer, "| Address : [0x%Q], Size : [0x%Q]. Allocation Success", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(15, iY, vcBuffer);
		kSleep(200);


		// Divide buffer half, then fill random data
		kSPrintf(vcBuffer, "| Address : [0x%Q], Size : [0x%Q]. Data Write...     ", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(15, iY, vcBuffer);

		for(i=0; i<qwMemorySize/2; i++)
		{
			pbAllocationBuffer[i] = kRandom() & 0xFF;
			pbAllocationBuffer[i + (qwMemorySize/2)]= pbAllocationBuffer[i];
		}

		kSleep(200);

		// check if filled data is correct
		kSPrintf(vcBuffer, "| Address : [0x%Q], Size : [0x%Q]. Data Verify...    ", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(15, iY, vcBuffer);
		
		for(i=0; i<qwMemorySize/2; i++)
		{
			if(pbAllocationBuffer[i] != pbAllocationBuffer[i + (qwMemorySize/2)])
			{
				kPrintf("Task ID [0x%Q] Verify Fail\n", pstTask->stLink.qwID);
				kExitTask();
			}
		}

		kFreeMemory(pbAllocationBuffer);
		kSleep(200);
	} 	// 10 loops end
	
	kExitTask();
}



static void kShowHDDInformation(const char* pcParameterBuffer)
{
	HDDINFORMATION stHDD;
	char vcBuffer[100];


	if(kGetHDDInformation(&stHDD) == FALSE)
	{
		kPrintf("HDD Information Read Fail\n");
		return;
	}


	kPrintf("========== Primary Master HDD Information ==========\n");

	// Print Model Number
	kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
	vcBuffer[sizeof(stHDD.vwModelNumber) - 1] = '\0';
	kPrintf("Model Number:\t %s\n", vcBuffer);


	// Print Serial Number
	kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
	vcBuffer[sizeof(stHDD.vwSerialNumber) - 1] = '\0';
	kPrintf("Serial Number:\t %s\n", vcBuffer);


	// Print Head Count, Cylinder Count, Sector Count per Cylinder
	kPrintf("Head Count:\t %d\n", stHDD.wNumberOfHead);
	kPrintf("Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder);
	kPrintf("Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder);


	// Print Total Sector Count
	kPrintf("Total Sector:\t %d Sector, %d MB\n", stHDD.dwTotalSectors, stHDD.dwTotalSectors/2/1024);

}


static void kReadSector(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcLBA[50], vcSectorCount[50];
	DWORD dwLBA;
	int iSectorCount;
	char* pcBuffer;
	int i, j;
	BYTE bData;
	BOOL bExit = FALSE;


	// Get LBA Address, Sector Count
	kInitializeParameter(&stList, pcParameterBuffer);

	if( (kGetNextParameter(&stList, vcLBA) == 0) || (kGetNextParameter(&stList, vcSectorCount) == 0) )
	{
		kPrintf("ex) readsector 0(LBA) 10(Count)\n");
		return;
	}

	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);

	
	// allocate Dynamic Memory by iSectorCount, then read
	pcBuffer = kAllocateMemory(iSectorCount * 512);

	if(kReadHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) == iSectorCount)
	{
		kPrintf("LBA [%d], [%d] Sector Read Success", dwLBA, iSectorCount);
			

		// Print contents in pcBuffer
		for(j=0; j<iSectorCount; j++)		// j = Sector Offset
		{
			// read one sector
			for(i=0; i<512; i++)	// i = Byte Offset of Sector
			{
				if( !((j==0) && (i==0)) && ((i % 256) == 0) )
				{
					kPrintf("\nPress any key to continue... ('q' is exit) : ");

					if(kGetCh() == 'q')
					{
						bExit = TRUE;
						break;
					}
				}

				if((i % 16) == 0)
				{
					kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA+j, i);
				}


				// if less than 16 Byte, add '0'
				bData = pcBuffer[j * 512 + i] & 0xFF;

				if(bData < 16)
				{
					kPrintf("0");
				}
				kPrintf("%X ", bData);

			}	// print one sector loop end

			if(bExit == TRUE)
			{
				break;
			}
		}	// print all sector loop end 
		
		kPrintf("\n");
	}
	else
	{
		kPrintf("Read Fail\n");
	}

	kFreeMemory(pcBuffer);
}


static void kWriteSector(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcLBA[50], vcSectorCount[50];
	DWORD dwLBA;
	int iSectorCount;
	char* pcBuffer;
	int i, j;
	BOOL bExit = FALSE;
	BYTE bData;
	static DWORD s_dwWriteCount = 0;


	// Get LBA, Sector Count
	kInitializeParameter(&stList, pcParameterBuffer);

	if( (kGetNextParameter(&stList, vcLBA) == 0) || (kGetNextParameter(&stList, vcSectorCount) == 0) )
	{
		kPrintf("ex) writesector 0(LBA) 10(Count)\n");
		return;
	}

	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);
	
	s_dwWriteCount++;


	// allocate buffer and fill data
	// 		Pattern : LBA Address (4 byte), Write Count (4 byte)
	pcBuffer = kAllocateMemory(iSectorCount * 512);

	for(j=0; j<iSectorCount; j++)	// j = Sector Offset
	{
		for(i=0; i<512; i += 8)	// i = Byte Offset
		{
			*(DWORD*)&(pcBuffer[j * 512 + i])     = dwLBA + j;
			*(DWORD*)&(pcBuffer[j * 512 + i + 4]) = s_dwWriteCount;
		}
	}

	
	// Write
	if(kWriteHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) != iSectorCount)
	{
		kPrintf("Write Fail\n");
		return;
	}
	kPrintf("LBA [%d], [%d] Sector Write Success", dwLBA, iSectorCount);


	// Print pcBuffer
	for(j=0; j<iSectorCount; j++)
	{
		for(i=0; i<512; i++)
		{
			if( !((j == 0) && (i == 0)) && ((i % 256) == 0) )
			{
				kPrintf("\nPress any key to continue... ('q' is exit) : ");

				if(kGetCh() == 'q')
				{
					bExit = TRUE;
					break;
				}
			}

			if( (i % 16) == 0 )
			{
				kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA+j, i);
			}


			bData = pcBuffer[j * 512 + i] & 0xFF;

			if(bData < 16)
			{
				kPrintf("0");
			}
			kPrintf("%X ", bData);
		}	// print one sector loop end

		if(bExit == TRUE)
		{
			break;
		}
	}	// print all sector loop end

	kPrintf("\n");

	kFreeMemory(pcBuffer);
}



// Connect HDD
static void kMountHDD(const char* pcParameterBuffer)	
{
	if(kMount() == FALSE)
	{
		kPrintf("HDD Mount Fail\n");
		return;
	}

	kPrintf("HDD Mount Success\n");
}


// Create File System in HDD
static void kFormatHDD(const char* pcParameterBuffer)
{
	if(kFormat() == FALSE)
	{
		kPrintf("HDD Format Fail\n");
		return;
	}

	kPrintf("HDD Format Success\n");
}


static void kShowFileSystemInformation(const char* pcParameterBuffer)
{
	FILESYSTEMMANAGER stManager;

	kGetFileSystemInformation(&stManager);

	kPrintf("========== File System Information ==========\n");
	kPrintf("Mounted:\t\t\t\t %d\n", stManager.bMounted);
	kPrintf("Reserved Sector Count:\t\t\t %d Sector\n", stManager.dwReservedSectorCount);
	kPrintf("Cluster Link Table Start Address:\t %d Sector\n", stManager.dwClusterLinkAreaStartAddress);
	kPrintf("Cluster Link Table Size:\t\t %d Sector\n", stManager.dwClusterLinkAreaSize);
	kPrintf("Data Area Start Address:\t\t %d Sector\n", stManager.dwDataAreaStartAddress);
	kPrintf("Total Cluster Count:\t\t\t %d Cluster\n", stManager.dwTotalClusterCount);
}


static void kCreateFileInRootDirectory(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	DWORD dwCluster;
	int i;
	FILE* pstFile;

	
	kInitializeParameter(&stList, pcParameterBuffer);
	iLength = kGetNextParameter(&stList, vcFileName);
	vcFileName[iLength] = '\0';

	if( (iLength > (FILESYSTEM_MAXFILENAMELENGTH- 1)) || (iLength == 0) )
	{
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}


	pstFile = fopen(vcFileName, "w");

	if(pstFile == NULL)
	{
		kPrintf("File Create Fail\n");
		return;
	}

	fclose(pstFile);

	kPrintf("File Create Success\n");
}


static void kDeleteFileInRootDirectory(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;


	kInitializeParameter(&stList, pcParameterBuffer);
	iLength = kGetNextParameter(&stList, vcFileName);
	vcFileName[iLength] = '\0';

	if( (iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0) )
	{
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}

	
	if(remove(vcFileName) != 0)
	{
		kPrintf("File Not Found or File Opened\n");
		return;
	}
	
	kPrintf("File Delete Complete\n");
}


static void kShowRootDirectory(const char* pcParameterBuffer)
{
	DIR* pstDirectory;
	int i, iCount, iTotalCount;
	struct dirent* pstEntry;
	char vcBuffer[400];
	char vcTempValue[50];
	DWORD dwTotalByte;
	DWORD dwUsedClusterCount;
	FILESYSTEMMANAGER stManager;

	
	// Get File System Info
	kGetFileSystemInformation(&stManager);


	// Open Root Directory
	pstDirectory = opendir("/");

	if(pstDirectory == NULL)
	{
		kPrintf("Root Directory Open Fail\n");
		return;
	}


	// ------------------------------------------------------
	// Calculate Total File Count, Total used Byte
	// ------------------------------------------------------
	iTotalCount = 0;
	dwTotalByte = 0;
	dwUsedClusterCount = 0;

	while(1)
	{
		// Read one Entry from Directory
		pstEntry = readdir(pstDirectory);

		// if there's no File anymore, exit
		if(pstEntry == NULL)
		{
			break;
		}

		iTotalCount++;
		dwTotalByte += pstEntry->dwFileSize;
		
		// calculate Cluster which actually used
		if(pstEntry->dwFileSize == 0)
		{
			dwUsedClusterCount++;		// 1 Cluster allocated at least, even though the size is 0
		}
		else
		{
			dwUsedClusterCount += 
				(pstEntry->dwFileSize + (FILESYSTEM_CLUSTERSIZE - 1)) / FILESYSTEM_CLUSTERSIZE;
		}
	}


	// -----------------------------------------------------
	// Print File Contents
	// -----------------------------------------------------
	rewinddir(pstDirectory);
	iCount = 0;

	while(1)
	{
		// Read one Entry
		pstEntry = readdir(pstDirectory);

		if(pstEntry == NULL)
		{
			break;
		}

		
		kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);
		vcBuffer[sizeof(vcBuffer) - 1] = '\0';

		// insert File Name
		kMemCpy(vcBuffer, pstEntry->d_name, kStrLen(pstEntry->d_name));

		// insert File Length
		kSPrintf(vcTempValue, "%d Byte", pstEntry->dwFileSize);
		kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));
		
		// insert File Start Cluster
		kSPrintf(vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex);
		kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue) + 1);

		kPrintf("	%s\n", vcBuffer);


		if( (iCount != 0) && ((iCount % 20) == 0) )
		{
			kPrintf("Press any key to continue...('q' is exit) : ");

			if(kGetCh() == 'q')
			{
				kPrintf("\n");
				break;
			}
		}

		iCount++;
	}	// Print File contents loop end


	// -----------------------------------------------------
	// Print Total File Count, Total File Size
	// -----------------------------------------------------
	
	kPrintf("\t\tTotal File Count: %d\n", iTotalCount);
	kPrintf("\t\tTotal File Size: %d KByte (%d Cluster)\n", dwTotalByte/1024, dwUsedClusterCount);

	kPrintf("\t\tFree Space: %d KByte (%d Cluster)\n", 
			(stManager.dwTotalClusterCount - dwUsedClusterCount) * FILESYSTEM_CLUSTERSIZE / 1024,
			stManager.dwTotalClusterCount - dwUsedClusterCount);


	// Close Directory
	closedir(pstDirectory);
}



// Create File, Write input data
static void kWriteDataToFile(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	FILE* fp;
	int iEnterCount;
	BYTE bKey;


	// Extract File Name
	kInitializeParameter(&stList, pcParameterBuffer);
	iLength = kGetNextParameter(&stList, vcFileName);
	vcFileName[iLength] = '\0';
	
	if( (iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0) )
	{
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}


	// Create File
	fp = fopen(vcFileName, "w");
	
	if(fp == NULL)
	{
		kPrintf("%s File Open Fail\n", vcFileName);
		return;
	}

	
	// Write contents to File until 3 Enter Key input
	iEnterCount = 0;

	// File Write loop
	while(1)
	{
		bKey = kGetCh();

		if(bKey == KEY_ENTER)
		{
			iEnterCount++;

			if(iEnterCount >= 3)
				break;
		}

		// if not Enter Key, Initialize Enter Key Count
		else
		{
			iEnterCount = 0;
		}

		
		kPrintf("%c", bKey);

		if(fwrite(&bKey, 1, 1, fp) != 1)	// write 1 byte
		{
			kPrintf("File Write Fail\n");
			break;
		}
	}	// File Write loop end


	kPrintf("File Write Success\n");
	fclose(fp);
}


static void kReadDataFromFile(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	FILE* fp;
	int iEnterCount;
	BYTE bKey;


	// extract File Name
	kInitializeParameter(&stList, pcParameterBuffer);
	iLength = kGetNextParameter(&stList, vcFileName);
	vcFileName[iLength] = '\0';

	if( (iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iLength == 0) )
	{
		kPrintf("Too Long or Too Short File Name\n");
		return;
	}


	// Create File
	fp = fopen(vcFileName, "r");
	
	if(fp == NULL)
	{
		kPrintf("%s File Open Fail\n");
		return;
	}


	// Print File until EOF
	iEnterCount = 0;

	// Print File loop
	while(1)
	{
		if(fread(&bKey, 1, 1, fp) != 1)
		{
			break;
		}

		kPrintf("%c", bKey);


		// if Enter Key input, increase Enter Key Count
		if(bKey == KEY_ENTER)
		{
			iEnterCount++;

			if( (iEnterCount != 0) && ((iEnterCount % 20) == 0) )
			{
				kPrintf("Press any key to continue...('q' is exit) : ");

				if(kGetCh() == 'q')
				{
					kPrintf("\n");
					break;
				}

				kPrintf("\n");
				iEnterCount = 0;
			}
		}
	}	// Print File loop end

	fclose(fp);
}


static void kTestFileIO(const char* pcParameterBuffer)
{
	FILE* pstFile;
	BYTE* pbBuffer;
	int i;
	int j;
	DWORD dwRandomOffset;
	DWORD dwByteCount;
	BYTE vbTempBuffer[1024];
	DWORD dwMaxFileSize;


	kPrintf("=============== File I/O Function Test ===============\n");

	// Allocate 4MB Buffer
	dwMaxFileSize = 4 * 1024 * 1024;
	pbBuffer = kAllocateMemory(dwMaxFileSize);

	if(pbBuffer == NULL)
	{
		kPrintf("Memory Allocation Fail\n");
		return;
	}

	// Remove Test File
	remove("testfileio.bin");


	// ================================================================
	// File Opening Test
	// ================================================================
	
	kPrintf("1. File Open Fail Test...");

	// "r" option makes No File, So should return NULL when Test File not exist
	pstFile = fopen("testfileio.bin", "r");

	if(pstFile == NULL)
	{
		kPrintf("[Pass]\n");
	}
	else
	{
		kPrintf("[Fail]\n");
		fclose(pstFile);
	}


	// ================================================================
	// File Creation Test
	// ================================================================
	
	kPrintf("2. File Create Test...");

	// "w" option can make File, so the Handle should be returned successfully
	pstFile = fopen("testfileio.bin", "w");
	
	if(pstFile != NULL)
	{
		kPrintf("[Pass]\n");
		kPrintf("	File Handle [0x%Q]\n", pstFile);
	}
	else
	{
		kPrintf("[Fail]\n");
	}


	// ================================================================
	// Sequential Data Writing Test
	// ================================================================
	
	kPrintf("3. Sequential Write Test (Cluster Size)...");

	// Write with opened Handle
	for(i=0; i<100; i++)
	{
		kMemSet(pbBuffer, i, FILESYSTEM_CLUSTERSIZE);

		if(fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE)
		{
			kPrintf("[Fail]\n");
			kPrintf("	%d Cluster Error\n", i);
			break;
		}
	}

	if(i >= 100)
	{
		kPrintf("[Pass]\n");
	}

	
	// ================================================================
	// Sequential Data Reading Test
	// ================================================================
	
	kPrintf("4. Sequential Read And Verify Test (Cluster Size)...");

	// Move to BOF (Begin Of File)
	fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_END);

	// Read File with opened Handle, then Verify Data
	for(i=0; i<100; i++)
	{
		// Read File
		if(fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE)
		{
			kPrintf("[Fail]\n");
			return;
		}

		// Verify Data
		for(j=0; j<FILESYSTEM_CLUSTERSIZE; j++)
		{
			if(pbBuffer[j] != (BYTE)i)
			{
				kPrintf("[Fail]\n");
				kPrintf("	%d Cluster Error, [%X] != [%X]\n", i, pbBuffer[j], (BYTE)i);
				break;
			}
		}

	}

	if(i >= 100)
	{
		kPrintf("[Pass]\n");
	}
	

	// ================================================================
	// Random Data Writing Test
	// ================================================================
	
	kPrintf("5. Random Write Test...\n");

	// Fill Buffer with 0
	kMemSet(pbBuffer, 0, dwMaxFileSize);

	// Read File contents, and copy to Buffer
	fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_CUR);
	fread(pbBuffer, 1, dwMaxFileSize, pstFile);

	// Write data to File, Buffer at the same time by moving random position
	for(i=0; i<100; i++)
	{
		dwByteCount = ( kRandom() % (sizeof(vbTempBuffer) - 1) ) + 1;
		dwRandomOffset = kRandom() % (dwMaxFileSize - dwByteCount);

		kPrintf("	%d. Offset [%d] Byte [%d]...", i, dwRandomOffset, dwByteCount);


		// move File Pointer
		fseek(pstFile, dwRandomOffset, SEEK_SET);
		kMemSet(vbTempBuffer, i, dwByteCount);


		// write Data
		if(fwrite(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount)
		{
			kPrintf("[Fail]\n");
			break;
		}
		else
		{
			kPrintf("[Pass]\n");
		}

		kMemSet(pbBuffer + dwRandomOffset, i, dwByteCount);
	}

	
	// Move to End Of File, and make File Size 4 MB
	fseek(pstFile, dwMaxFileSize - 1, SEEK_SET);
	fwrite(&i, 1, 1, pstFile);
	pbBuffer[dwMaxFileSize - 1] = (BYTE)i;


	// ================================================================
	// Random Data Reading Test
	// ================================================================
	
	kPrintf("6. Random Read And Verify Test...\n");
	
	// Read file by moving random position, then compare with Buffer
	for(i=0; i<100; i++)
	{
		dwByteCount = ( kRandom() % (sizeof(vbTempBuffer) - 1) ) + 1;
		dwRandomOffset = kRandom() % ( (dwMaxFileSize) - dwByteCount );

		kPrintf("	%d. Offset [%d] Byte [%d]...", i, dwRandomOffset, dwByteCount);

		
		// move File Pointer
		fseek(pstFile, dwRandomOffset, SEEK_SET);

		// read data
		if(fread(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount)
		{
			kPrintf("[Fail]\n");
			kPrintf("[%d] Read Fail\n", dwRandomOffset);
			break;
		}

		// compare with Buffer
		if(kMemCmp(pbBuffer + dwRandomOffset, vbTempBuffer, dwByteCount) != 0)
		{
			kPrintf("[Fail]\n");
			kPrintf("[%d] Compare Fail\n", dwRandomOffset);
			break;
		}

		kPrintf("[Pass]\n");
	}
	

	// ================================================================
	// Sequential Data Writing Test again
	// ================================================================
	
	kPrintf("7. Sequential Write, Read And Verify Test(1024 Byte)...\n");

	// move File Pointer to BOF
	fseek(pstFile, -dwMaxFileSize, SEEK_CUR);

	// write with opened Handle, from BOF to 2MB
	for(i=0; i<(2 * 1024 * 1024 / 1024); i++)
	{
		kPrintf("	%d. Offset [%d] Byte [%d] Write...", i, i*1024, 1024);
		
		// write 1024 byte
		if(fwrite(pbBuffer + (i*1024), 1, 1024, pstFile) != 1024)
		{
			kPrintf("[Fail]\n");
			return;
		}
		else
		{
			kPrintf("[Pass]\n");
		}
	}


	// move File Pointer to BOF
	fseek(pstFile, -dwMaxFileSize, SEEK_SET);

	// read & verify Data with opened Handle
	// 		possible to wrong Data saving by Random Write, so verify all 4MB area
	for(i=0; i<(dwMaxFileSize / 1024); i++)
	{
		// Data verity
		kPrintf("	%d. Offset [%d] Byte [%d] Read And Verity...", i, i*1024, 1024);

		// read 1024 bytes
		if(fread(vbTempBuffer, 1, 1024, pstFile) != 1024)
		{
			kPrintf("[Fail]\n");
			return;
		}
		
		if(kMemCmp(pbBuffer + (i*1024), vbTempBuffer, 1024) != 0)
		{
			kPrintf("[Fail]\n");
			break;
		}
		else
		{
			kPrintf("[Pass]\n");
		}
	}


	// ================================================================
	// File Deletion Failure Test
	// ================================================================
	
	kPrintf("8. File Delete Fail Test...");

	// File is in Open State, so remove() should be failed
	if(remove("testfileio.bin") != 0)
	{
		kPrintf("[Pass]\n");
	}
	else
	{
		kPrintf("[Fail]\n");
	}


	// ================================================================
	// Closing File Test
	// ================================================================
	
	kPrintf("9. File Close Test...");

	if(fclose(pstFile) == 0)
	{
		kPrintf("[Pass]\n");
	}
	else
	{
		kPrintf("[Fail]\n");
	}


	// ================================================================
	// File Deletion Test
	// ================================================================
	
	kPrintf("10. File Delete Test...");

	// File is closed, so remove() should remove File successfully
	if(remove("testfileio.bin") == 0)
	{
		kPrintf("[Pass]\n");
	}
	else
	{
		kPrintf("[Fail]\n");
	}


	// Free memory
	kFreeMemory(pbBuffer);
}



static void kFlushCache(const char* pcParameterBuffer)
{
	QWORD qwTickCount;


	qwTickCount = kGetTickCount();

	kPrintf("Cache Flush... ");
	
	if(kFlushFileSystemCache() == TRUE)
	{
		kPrintf("Pass\n");
	}
	else
	{
		kPrintf("Fail\n");
	}

	kPrintf("Total Time = %d ms\n", kGetTickCount() - qwTickCount);
}


static void kTestPerformance(const char* pcParameterBuffer)
{
	FILE* pstFile;
	DWORD dwClusterTestFileSize;
	DWORD dwOneByteTestFileSize;
	QWORD qwLastTickCount;
	DWORD i;
	BYTE* pbBuffer;

	
	// Cluster Read/Write Testing with 1 MB File
	dwClusterTestFileSize = 1024 * 1024;

	// Byte Read/Write Testing with 16 KB File
	dwOneByteTestFileSize = 16 * 1024;

	
	// Allocate Buffer for testing
	pbBuffer = kAllocateMemory(dwClusterTestFileSize);

	if(pbBuffer == NULL)
	{
		kPrintf("Memory Allocation Fail\n");
		
		return;
	}
	
	// init Buffer
	kMemSet(pbBuffer, 'I', FILESYSTEM_CLUSTERSIZE);


	kPrintf("=============== File I/O Performance Test ===============\n");

	// =============================================================
	// Sequential Write Testing (Cluster Size)
	// =============================================================
	
	kPrintf("1. Sequential Read/Write Test (Cluster Size)\n");

	
	// remove old Test File, and create again
	remove("performance.txt");

	pstFile = fopen("performance.txt", "w");

	if(pstFile == NULL)
	{
		kPrintf("File Open Fail\n");
		kFreeMemory(pbBuffer);

		return;
	}


	qwLastTickCount = kGetTickCount();


	// write Cluster in File
	for(i=0; i<(dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE); i++)
	{
		if(fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE)
		{
			kPrintf("Write Fail\n");

			// close File and free memory
			fclose(pstFile);
			kFreeMemory(pbBuffer);

			return;
		}
	}


	// print time
	kPrintf("	Sequential Write(Cluster Size): %d ms\n", kGetTickCount() - qwLastTickCount);
	


	// =============================================================
	// Sequential Read Testing (Cluster Size)
	// =============================================================
	
	// go to BOF (Begin Of File)
	fseek(pstFile, 0, SEEK_SET);

	qwLastTickCount = kGetTickCount();


	// read Cluster in File
	for(i=0; i<(dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE); i++)
	{
		if(fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE)
		{
			kPrintf("Read Fail\n");

			// close File and free memory
			fclose(pstFile);
			kFreeMemory(pbBuffer);

			return;
		}
	}


	// print time
	kPrintf("	Sequential Read(Cluster Size): %d ms\n", kGetTickCount() - qwLastTickCount);


	// =============================================================
	// Sequential Write Testing (1 Byte)
	// =============================================================
	
	kPrintf("2. Sequential Read/Write Test (1 Byte)\n");

	
	// remove old Test File, create again
	remove("performance.txt");

	pstFile = fopen("performance.txt", "w");

	if(pstFile == NULL)
	{
		kPrintf("File Open Fail\n");
		kFreeMemory(pbBuffer);

		return;
	}


	qwLastTickCount = kGetTickCount();

	
	// write 1 byte to File
	for(i=0; i<dwOneByteTestFileSize; i++)
	{
		if(fwrite(pbBuffer, 1, 1, pstFile) != 1)
		{
			kPrintf("Write Fail\n");
			
			fclose(pstFile);
			kFreeMemory(pbBuffer);

			return;
		}
	}

	
	// print time
	kPrintf("	Sequential Write(1 Byte): %d ms\n", kGetTickCount() - qwLastTickCount);



	// =============================================================
	// Sequential Read Testing (1 Byte)
	// =============================================================
	
	// move to BOF
	fseek(pstFile, 0, SEEK_SET);

	
	qwLastTickCount = kGetTickCount();


	// read 1 byte
	for(i=0; i<dwOneByteTestFileSize; i++)
	{
		if(fread(pbBuffer, 1, 1, pstFile) != 1)
		{
			kPrintf("Read Fail\n");

			fclose(pstFile);
			kFreeMemory(pbBuffer);

			return;
		}
	}

	
	// print time
	kPrintf("	Sequential Read(1 Byte): %d ms\n", kGetTickCount() - qwLastTickCount);


	// close File and free memory
	fclose(pstFile);
	kFreeMemory(pbBuffer);
}



static void kDownloadFile(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char  vcFileName[50];
	int   iFileNameLength;
	DWORD dwDataLength;
	FILE* fp;
	DWORD dwReceivedSize;
	DWORD dwTempSize;
	BYTE  vbDataBuffer[SERIAL_FIFOMAXSIZE];
	QWORD qwLastReceivedTickCount;


	// extract File Name
	kInitializeParameter(&stList, pcParameterBuffer);
	iFileNameLength = kGetNextParameter(&stList, vcFileName);
	vcFileName[iFileNameLength] = '\0';

	if( (iFileNameLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) || (iFileNameLength == 0) )
	{
		kPrintf("Too Long or Too Short File Name\n");
		kPrintf("ex) download a.txt\n");

		return;
	}


	// clear all Serial FIFO
	kClearSerialFIFO();


	// ===========================================================================
	// Wait for receiving data length, receive 4 bytes and then send ACK
	// ===========================================================================
	
	kPrintf("Waiting for Data Length.....");

	dwReceivedSize = 0;
	qwLastReceivedTickCount = kGetTickCount();

	while(dwReceivedSize < 4)
	{
		// receive remaining data
		dwTempSize = kReceiveSerialData( ((BYTE*)&dwDataLength) + dwReceivedSize, 4 - dwReceivedSize );
		dwReceivedSize += dwTempSize;

		// if there's no received data, wait for a while
		if(dwTempSize == 0)
		{
			kSleep(0);

			// if Waiting Time is over 30 sec, time out
			if( (kGetTickCount() - qwLastReceivedTickCount) > 30000)
			{
				kPrintf("Timeout Occured\n");
				return;
			}
		}
		else
		{
			// update Last data receive time
			qwLastReceivedTickCount = kGetTickCount();
		}
	}

	kPrintf("[%d] Byte\n", dwDataLength);


	// Receiving data length success, so send ACK
	kSendSerialData("A", 1);


	// ===========================================================================
	// Create File, and receive Data and save on File
	// ===========================================================================
	
	// create File
	fp = fopen(vcFileName, "w");

	if(fp == NULL)
	{
		kPrintf("%s File Open Fail\n", vcFileName);
		return;
	}


	// receive data
	kPrintf("Data Receive Start: ");
	
	dwReceivedSize = 0;
	qwLastReceivedTickCount = kGetTickCount();

	while(dwReceivedSize < dwDataLength)
	{
		// save data in buffer, and then write data
		dwTempSize = kReceiveSerialData(vbDataBuffer, SERIAL_FIFOMAXSIZE);
		dwReceivedSize += dwTempSize;

		// if data received, send ACK or write File
		if(dwTempSize != 0)
		{
			// Receiver send ACK when received Last Data or 16 bytes (FIFO Max Size)
			if( ((dwReceivedSize % SERIAL_FIFOMAXSIZE) == 0) || (dwReceivedSize == dwDataLength) )
			{
				kSendSerialData("A", 1);
				kPrintf("#");
			}

			// if error occured when writing, quit
			if(fwrite(vbDataBuffer, 1, dwTempSize, fp) != dwTempSize)
			{
				kPrintf("File Write Error Occur\n");
				break;
			}

			// update Last received tick count
			qwLastReceivedTickCount = kGetTickCount();
		}
		// if data not received, wait for a while
		else
		{
			kSleep(0);

			// if Waiting Time over 10 sec, Timeout
			if( (kGetTickCount() - qwLastReceivedTickCount) > 10000 )
			{
				kPrintf("Timeout Occured\n");
				break;
			}
		}
	}	// Data receive loop end
	

	// ===========================================================================
	// compare whole Data Size with received Data Size and print result
	// close File and flush File System Cache
	// ===========================================================================

	// compare between size to check error occured
	if(dwReceivedSize != dwDataLength)
	{
		kPrintf("\nError Occur. Total Size [%d] != Received Size [%d]\n", dwDataLength, dwReceivedSize);
	}
	else
	{
		kPrintf("\nReceive Complete, Total Size [%d] Byte\n", dwReceivedSize);
	}

	
	// close File and flush File System cache
	fclose(fp);
	kFlushFileSystemCache();
}



static void kShowMPConfigurationTable(const char* pcParameterBuffer)
{
	kPrintMPConfigurationTable();
}



static void kStartApplicationProcessor(const char* pcParameterBuffer)
{
	// Wake up AP
	if(kStartUpApplicationProcessor() == FALSE)
	{
		kPrintf("Application Processor Start Fail\n");
		return;
	}
	
	kPrintf("Application Processor Start Success\n");
	

	// Print Bootstrap Processor APIC ID
	kPrintf("Bootstrap Processor [APIC ID: %d], Start Application Processor\n", kGetAPICID());
}



// Start Symmetric I/O Mode
static void kStartSymmetricIOMode(const char* pcParameterBuffer)
{
	MPCONFIGURATIONMANAGER* pstMPManager;
	BOOL bInterruptFlag;


	// analyze MP Configuration Table
	if(kAnalysisMPConfigurationTable() == FALSE)
	{
		kPrintf("Analyze MP Configuration Table Fail\n");
		return;
	}


	// check if PIC Mode enabled by checking MP Configuration Manager
	pstMPManager = kGetMPConfigurationManager();

	if(pstMPManager->bUsePICMode == TRUE)
	{
		// if PIC Mode, disable PIC Mode
		// 		by sending 0x70 to I/O Port Address 0x22
		// 		by sending 0x01 to I/O Port Address 0x23
		kOutPortByte(0x22, 0x70);			// used to access IMCR Register
		kOutPortByte(0x23, 0x01);			// used to disable PIC Mode
	}

	
	// make all PIC Controller Interrupt for no occurence of Interrupt
	kPrintf("Mask All PIC Controller Interrupt\n");
	kMaskPICInterrupt(0xFFFF);

	
	// activate all Local APIC of AP
	kPrintf("Enable Global Local APIC\n");
	kEnableGlobalLocalAPIC();

	
	// activate Local APIC of BSP
	kPrintf("Enable Software Local APIC\n");
	kEnableSoftwareLocalAPIC();


	/////////////////////////////////////
	// disable Interrupt
	kPrintf("Disable CPU Interrupt Flag\n");
	bInterruptFlag = kSetInterruptFlag(FALSE);
	/////////////////////////////////////
	
	
	// set Task Priority Register to 0 for all Interrupt reception (Local APIC)
	kSetTaskPriority(0);


	// initialize Local Vector Table of Local APIC
	kInitializeLocalVectorTable();

	
	// set Flag as changed to Symmetric I/O Mode
	kSetSymmetricIOMode(TRUE);
	

	// initialize I/O APIC
	kPrintf("Initialize I/O Redirection Table\n");
	kInitializeIORedirectionTable();


	/////////////////////////////////////
	// restore previous Interrupt Flag
	kPrintf("Restore CPU Interrupt Flag\n");
	kSetInterruptFlag(bInterruptFlag);
	/////////////////////////////////////


	kPrintf("Change Symmetric I/O Mode Complete\n");
}


// Print Table which IRQ->INTIN Relationship stored
static void kShowIRQINTINMappingTable(const char* pcParameterBuffer)
{
	kPrintIRQToINTINMap();
}



// Print Interrupt Handling Count per Core
static void kShowInterruptProcessingCount(const char* pcParameterBuffer)
{
	INTERRUPTMANAGER* pstInterruptManager;
	int i;
	int j;
	int iProcessCount;
	char vcBuffer[20];
	int iRemainLength;
	int iLineCount;

	
	kPrintf("=============== Interrupt Count ===============\n");


	// read Core Count stored in MP Configuration Table
	iProcessCount = kGetProcessorCount();

	
	// ==============================================
	// Print Column
	// ==============================================
	
	// 4 Cores per 1 Column, 15 space allocated per 1 Column
	for(i=0; i<MAXPROCESSORCOUNT; i++)
	{
		if(i == 0)
		{
			kPrintf("IRQ Num\t\t");
		}
		else if( (i%4) == 0 )
		{
			kPrintf("\n     \t\t");
		}

		kSPrintf(vcBuffer, "Core %d", i);
		kPrintf(vcBuffer);


		// fill Remaining space with Space
		iRemainLength = 15 - kStrLen(vcBuffer);
		
		kMemSet(vcBuffer, ' ', iRemainLength);
		vcBuffer[iRemainLength] = '\0';

		kPrintf(vcBuffer);
	}

	kPrintf("\n");


	// ==============================================
	// Print Row and Interrupt Count
	// ==============================================
	
	// print Total Interrupt Count, Interrupt Handling Count per Core
	iLineCount = 0;
	pstInterruptManager = kGetInterruptManager();

	for(i=0; i<INTERRUPT_MAXVECTORCOUNT; i++)
	{
		for(j=0; j<MAXPROCESSORCOUNT; j++)
		{
			// print Row, print 4 Cores per 1 Line, allocate 15 space per 1 Column
			if(j == 0)
			{
				// stop per 20 Line
				if( (iLineCount != 0) && (iLineCount > 10) )
				{
					kPrintf("\nPress any key to continue... ('q' is exit) : ");

					if(kGetCh() == 'q')
					{
						kPrintf("\n");
						return;
					}

					iLineCount = 0;
					kPrintf("\n");
				}

				kPrintf("--------------------------------------------------------------------------\n");
				kPrintf("IRQ %d\t\t", i);

				iLineCount += 2;
			}
			else if( (j%4) == 0 )
			{
				kPrintf("\n     \t\t");
				iLineCount++;
			}

			kSPrintf(vcBuffer, "0x%Q", pstInterruptManager->vvqwCoreInterruptCount[j][i]);

			// fill Remaining area with Space
			kPrintf(vcBuffer);

			iRemainLength = 15 - kStrLen(vcBuffer);

			kMemSet(vcBuffer, ' ', iRemainLength);
			vcBuffer[iRemainLength] = '\0';

			kPrintf(vcBuffer);
		}

		kPrintf("\n");
	}
}


static void kStartInterruptLoadBalancing(const char* pcParameterBuffer)
{
	kPrintf("Start Interrupt Load Balancing\n");

	kSetInterruptLoadBalancing(TRUE);
}



static void kStartTaskLoadBalancing(const char* pcParameterBuffer)
{
	int i;

	kPrintf("Start Task Load Balancing\n");

	for(i=0; i<MAXPROCESSORCOUNT; i++)
	{
		kSetTaskLoadBalancing(i, TRUE);
	}
}


static void kChangeTaskAffinity(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcID[30];
	char vcAffinity[30];
	QWORD qwID;
	BYTE bAffinity;


	// get Parameter
	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcID);
	kGetNextParameter(&stList, vcAffinity);


	// get Task ID field
	if(kMemCmp(vcID, "0x", 2) == 0)
	{
		qwID = kAToI(vcID + 2, 16);
	}
	else
	{
		qwID = kAToI(vcID, 10);
	}


	// get Processor Affinity field
	if(kMemCmp(vcID, "0x", 2) == 0)
	{
		bAffinity = kAToI(vcAffinity + 2, 16);
	}
	else
	{
		bAffinity = kAToI(vcAffinity, 10);
	}


	kPrintf("Change Task Affinity ID [0x%Q] Affinity [0x%X] ", qwID, bAffinity);

	if(kChangeProcessorAffinity(qwID, bAffinity) == TRUE)
	{
		kPrintf("Success\n");
	}
	else
	{
		kPrintf("Fail\n");
	}
}



static void kShowVBEModeInfo(const char* pcParameterBuffer)
{
	VBEMODEINFOBLOCK* pstModeInfo;


	pstModeInfo = kGetVBEModeInfoBlock();

	kPrintf("VESA %x\n", pstModeInfo->wWinGranulity);
	kPrintf("X Resolution: %d\n", pstModeInfo->wXResolution);
	kPrintf("Y Resolution: %d\n", pstModeInfo->wYResolution);
	kPrintf("Bits per Pixel: %d\n", pstModeInfo->bBitsPerPixel);

	// print Resolution, Color info
	kPrintf("Red Mask Size: %d, Field Position: %d\n", 
			pstModeInfo->bRedMaskSize, pstModeInfo->bRedFieldPosition);
	kPrintf("Green Mask Size: %d, Field Position: %d\n", 
			pstModeInfo->bGreenMaskSize, pstModeInfo->bGreenFieldPosition);
	kPrintf("Blue Mask Size: %d, Field Position: %d\n", 
			pstModeInfo->bBlueMaskSize, pstModeInfo->bBlueFieldPosition);

	kPrintf("Physical Base Pointer: 0x%X\n", pstModeInfo->dwPhysicalBasePointer);

	kPrintf("Linear Red Mask Size: %d, Field Position: %d\n", 
			pstModeInfo->bLinearRedMaskSize, pstModeInfo->bLinearRedFieldPosition);
	kPrintf("Linear Green Mask Size: %d, Field Position: %d\n", 
			pstModeInfo->bLinearGreenMaskSize, pstModeInfo->bLinearGreenFieldPosition);
	kPrintf("Linear Blue Mask Size: %d, Field Position: %d\n", 
			pstModeInfo->bLinearBlueMaskSize, pstModeInfo->bLinearBlueFieldPosition);
}



static void kTestSystemCall(const char* pcParameterBuffer)
{
	BYTE* pbUserMemory;


	// allocate 4 KB to create User Level Task
	pbUserMemory = kAllocateMemory(0x1000);

	if(pbUserMemory == NULL)
	{
		return;
	}


	// copy kSystemCallTestTask() code to allocated 4 KB Memory
	kMemCpy(pbUserMemory, kSystemCallTestTask, 0x1000);


	// create as User Level Task
	kCreateTask(TASK_FLAGS_USERLEVEL | TASK_FLAGS_PROCESS, pbUserMemory, 0x1000, (QWORD)pbUserMemory, TASK_LOADBALANCINGID);
}



static void kExecuteApplicationProgram(const char* pcParameterBuffer)
{
	PARAMETERLIST stList;
	char vcFileName[512];
	char vcArgumentString[1024];
	QWORD qwID;


	// get Parameters
	kInitializeParameter(&stList, pcParameterBuffer);


	// check instruction format : file name
	if(kGetNextParameter(&stList, vcFileName) == 0)
	{
		kPrintf("ex)exec a.elf argument\n");

		return;
	}


	// check instruction format : argument is optional
	if(kGetNextParameter(&stList, vcArgumentString) == 0)
	{
		vcArgumentString[0] = '\0';
	}

	kPrintf("Execute Program... File [%s], Argument [%s]\n", vcFileName, vcArgumentString);


	// create Task
	qwID = kExecuteProgram(vcFileName, vcArgumentString, TASK_LOADBALANCINGID);
	kPrintf("Task ID = 0x%Q\n", qwID);
}



static void kInstallPackage(const char* pcParameterBuffer)
{
	PACKAGEHEADER* pstHeader;
	PACKAGEITEM*   pstItem;
	WORD wKernelTotalSectorCount;
	int i;
	FILE* fp;
	QWORD qwDataAddress;


	kPrintf("Package Install Start...\n");


	// read total sector count (protected mode kernel + IA-32e mode kernel) from 0x7C05
	wKernelTotalSectorCount = *((WORD*)0x7C05);


	// package header is located behind of 0x10000 + kernel sector
	pstHeader = (PACKAGEHEADER*)((QWORD)0x10000 + wKernelTotalSectorCount * 512);

	
	// check signature
	if( kMemCmp(pstHeader->vcSignature, PACKAGESIGNATURE, sizeof(pstHeader->vcSignature)) != 0 )
	{
		kPrintf("Package Signature Fail\n");

		return;
	}


	// -------------------------------------------------------
	// copy files in package into HDD
	// -------------------------------------------------------
	
	// start address of data in package file
	qwDataAddress = (QWORD)pstHeader + pstHeader->dwHeaderSize;

	
	// file data of first entry
	pstItem = pstHeader->vstItem;


	// copy all file data into HDD
	for(i=0; i < pstHeader->dwHeaderSize / sizeof(PACKAGEITEM); i++)
	{
		kPrintf("[%d] file: %s, size: %d Byte\n", i+1, pstItem[i].vcFileName, pstItem[i].dwFileLength);

		
		// create file
		fp = fopen(pstItem[i].vcFileName, "w");

		if(fp == NULL)
		{
			kPrintf("%s File Create Fail\n");
			return;
		}

		
		// write file (copy file data)
		if( fwrite((BYTE*)qwDataAddress, 1, pstItem[i].dwFileLength, fp) != pstItem[i].dwFileLength )
		{
			kPrintf("Write Fail\n");

			// close file, flush file system cache
			fclose(fp);
			kFlushFileSystemCache();

			return;
		}


		// close file
		fclose(fp);


		// move to next file data
		qwDataAddress += pstItem[i].dwFileLength;
	}

	kPrintf("Package Install Complete\n");


	// flush file system cache
	kFlushFileSystemCache();
}



