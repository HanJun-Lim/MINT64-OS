#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__


#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"
#include "CacheManager.h"



// ===========================================
// Macro and Funcion pointer
// ===========================================


// MINT File System Signature
#define FILESYSTEM_SIGNATURE				0x7E38CF10


// Cluster Size (Sector Count) - 8 Sectors
#define FILESYSTEM_SECTORSPERCLUSTER		8


// End of File Cluster
#define FILESYSTEM_LASTCLUSTER				0xFFFFFFFF


// Empty Cluster
#define FILESYSTEM_FREECLUSTER				0x00


// Maximum Directory Entry Count in Root Directory
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT	( (FILESYSTEM_SECTORSPERCLUSTER * 512) / sizeof(DIRECTORYENTRY) )


// Cluster Size (Byte) - 4 KB, 4096 Byte
#define FILESYSTEM_CLUSTERSIZE				( (FILESYSTEM_SECTORSPERCLUSTER * 512) )


// Max Count of Handle
#define FILESYSTEM_HANDLE_MAXCOUNT			(TASK_MAXCOUNT * 3)


// Max length of File Name
#define FILESYSTEM_MAXFILENAMELENGTH		24


// Handle Type
#define FILESYSTEM_TYPE_FREE				0
#define FILESYSTEM_TYPE_FILE				1
#define FILESYSTEM_TYPE_DIRECTORY			2


// Seek Options
#define FILESYSTEM_SEEK_SET					0
#define FILESYSTEM_SEEK_CUR					1
#define FILESYSTEM_SEEK_END					2


// Define Function Pointer Type related to HDD Control
typedef BOOL (*fReadHDDInformation)(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
typedef int  (*fReadHDDSector)	   (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
typedef int  (*fWriteHDDSector)	   (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);


// Re-Define MINT File System Function to Standard I/O Function
#define fopen		kOpenFile
#define fread		kReadFile
#define fwrite		kWriteFile
#define fseek		kSeekFile
#define fclose		kCloseFile
#define remove		kRemoveFile
#define opendir		kOpenDirectory
#define readdir		kReadDirectory
#define rewinddir	kRewindDirectory
#define closedir	kCloseDirectory


// Re-Define File System Macro to Standard I/O Macro
#define SEEK_SET	FILESYSTEM_SEEK_SET
#define SEEK_CUR	FILESYSTEM_SEEK_CUR
#define SEEK_END	FILESYSTEM_SEEK_END


// Re-Define MINT File System Types and Fields to Standard I/O Type
#define size_t		DWORD
#define dirent		kDirectoryEntryStruct
#define d_name		vcFileName




// ===========================================
// Structure
// ===========================================

#pragma pack(push, 1)


typedef struct kPartitionStruct
{
	// Bootable Flag - 0x80 means bootable, 0x00 means non-bootable
	BYTE bBootableFlag;

	// Partition Start Address, barely used now, LBA address used instead
	BYTE vbStartingCHSAddress[3];

	// Partition Type
	BYTE bPartitionType;
	
	// Partition End Address, barely used now.
	BYTE vbEndingCHSAddress[3];

	// Partition Start Address, represented by LBA Address
	DWORD dwStartingLBAAddress;

	// Total Sector Count of Partition
	DWORD dwSizeInSector;
} PARTITION;


typedef struct kMBRStruct
{
	// Boot Loader Area
	BYTE vbBootCode[430];

	
	// File System Signature
	DWORD dwSignature;
	
	// Sector Count of Reserved Area
	DWORD dwReservedSectorCount;

	// Sector Count of Cluster Link Table Area
	DWORD dwClusterLinkSectorCount;

	// Total Cluster Count
	DWORD dwTotalClusterCount;


	// Partition Table
	PARTITION vstPartition[4];


	// Boot Loader Signature : 0x55, 0xAA
	BYTE vbBootLoaderSignature[2];
} MBR;


typedef struct kDirectoryEntryStruct
{
	// File Name : Max is 24
	char vcFileName[FILESYSTEM_MAXFILENAMELENGTH];

	// Real Size of File
	DWORD dwFileSize;

	// Cluster Index which File started
	DWORD dwStartClusterIndex;
} DIRECTORYENTRY;



typedef struct kFileHandleStruct
{
	// Directory Entry Offset that file exists
	int iDirectoryEntryOffset;

	// File Size
	DWORD dwFileSize;

	// Start Cluster Index of File
	DWORD dwStartClusterIndex;

	// Cluster Index which in I/O operation
	DWORD dwCurrentClusterIndex;

	// Privious Cluster of Current Cluster
	DWORD dwPreviousClusterIndex;

	// Current File Pointer Position (Byte offset)
	DWORD dwCurrentOffset;
} FILEHANDLE;


typedef struct kDirectoryHandleStruct
{
	// Buffer for Root Directory
	DIRECTORYENTRY* pstDirectoryBuffer;

	// Current Directory Pointer Position
	int iCurrentOffset;
} DIRECTORYHANDLE;


typedef struct kFileDirectoryHandleStruct
{
	// Data Structure Type : EMPTY, FILE, DIR
	BYTE bType;

	// Usage depends on bTypes
	union
	{
		// File Handle
		FILEHANDLE stFileHandle;

		// Directory Handle
		DIRECTORYHANDLE stDirectoryHandle;
	};
} FILE, DIR;


typedef struct kFileSystemManagerStruct
{
	// Whether File System mounted or not
	BOOL bMounted;

	// Sector Count, Start LBA Address of each area
	DWORD dwReservedSectorCount;
	DWORD dwClusterLinkAreaStartAddress;
	DWORD dwClusterLinkAreaSize;
	DWORD dwDataAreaStartAddress;

	// Total Cluster Count of Data Area
	DWORD dwTotalClusterCount;
	
	// Sector Offset of last allocated Cluster Link Table
	DWORD dwLastAllocatedClusterLinkSectorOffset;
	
	// Sync Object
	MUTEX stMutex;

	// Handle Pool Address
	FILE* pstHandlePool;

	// Cache enabled?
	BOOL bCacheEnable;
} FILESYSTEMMANAGER;


#pragma pack(pop)




// ===========================================
// Function
// ===========================================

BOOL kInitializeFileSystem(void);
BOOL kFormat(void);
BOOL kMount(void);
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation);


// ---------- Low-Level Function ----------

static BOOL  kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL  kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL  kReadCluster(DWORD dwOffset, BYTE* pbBuffer);
static BOOL  kWriteCluster(DWORD dwOffset, BYTE* pbBuffer);
static DWORD kFindFreeCluster(void);
static BOOL  kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData);
static BOOL  kGetClusterLinkData(DWORD dwClusterIndex, DWORD* pdwData);
static int	 kFindFreeDirectoryEntry(void);
static BOOL  kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static BOOL  kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static int   kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry);
void  		 kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager);

// functions for Cache, Cache method is Write-Back

static BOOL kInternalReadClusterLinkTableWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalReadClusterLinkTableWithCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterLinkTableWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterLinkTableWithCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalReadClusterWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalReadClusterWithCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterWithCache(DWORD dwOffset, BYTE* pbBuffer);

static CACHEBUFFER* kAllocateCacheBufferWithFlush(int iCacheTableIndex);
BOOL kFlushFileSystemCache(void);



// ---------- High-Level Function ----------

FILE* kOpenFile(const char* pcFileName, const char* pcMode);
DWORD kReadFile(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
DWORD kWriteFile(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
int	  kSeekFile(FILE* pstFile, int iOffset, int iOrigin);
int   kCloseFile(FILE* pstFile);
int	  kRemoveFile(const char* pcFileName);
DIR*  kOpenDirectory(const char* pcDirectoryName);
struct kDirectoryEntryStruct* kReadDirectory(DIR* pstDirectory);
void  kRewindDirectory(DIR* pstDirectory);
int   kCloseDirectory(DIR* pstDirectory);
BOOL  kWriteZero(FILE* pstFile, DWORD dwCount);
BOOL  kIsFileOpened(const DIRECTORYENTRY* pstEntry);

static void* kAllocateFileDirectoryHandle(void);
static void  kFreeFileDirectoryHandle(FILE* pstFile);
static BOOL  kCreateFile(const char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex);
static BOOL  kFreeClusterUntilEnd(DWORD dwClusterIndex);
static BOOL  kUpdateDirectoryEntry(FILEHANDLE* pstFileHandle);



#endif
