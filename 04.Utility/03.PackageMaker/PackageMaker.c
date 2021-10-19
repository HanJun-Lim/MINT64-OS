#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>


// ---------- Macro ----------

// byte size of one sector
#define BYTESOFSECTOR		512

// package signature
#define PACKAGESIGNATURE	"MINT64OSPACKAGE "

// max length of file name (same as FILESYSTEM_MAXFILENAMELENGTH)
#define MAXFILENAMELENGTH	24

// define DWORD type
#define DWORD				unsigned int



// ---------- Structure ----------

#pragma pack(push, 1)


// entry of package file
typedef struct PackageItemStruct
{
	// file name
	char vcFileName[MAXFILENAMELENGTH];

	// file size
	DWORD dwFileLength;
} PACKAGEITEM;


// package header
typedef struct PackageHeaderStruct
{
	// signature of MINT64 OS package file
	char vcSignature[16];

	// package header size
	DWORD dwHeaderSize;

	// first entry of package file
	PACKAGEITEM vstItem[0];
} PACKAGEHEADER;


#pragma pack(pop)



// ---------- Function ----------

int AdjustInSectorSize(int iFd, int iSourceSize);
int CopyFile(int iSourceFd, int iTargetFd);



int main(int argc, char* argv[])
{
	int iSourceFd;
	int iTargetFd;
	int iSourceSize;
	int i;
	struct stat   stFileData;
	PACKAGEHEADER stHeader;
	PACKAGEITEM	  stItem;


	// check commmand line option
	if(argc < 2)
	{
		fprintf(stderr, "[ERROR] PackageMaker.exe app1.elf app2.elf data.txt ...\n");
		exit(-1);
	}


	// create Package.img file
	if( (iTargetFd = open("Package.img", O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE)) == -1 )
	{
		fprintf(stderr, "[ERROR] Package.img open fail\n");
		exit(-1);
	}


	// --------------------------------------------------------------
	// make package header first by file name
	// --------------------------------------------------------------
	
	printf("[INFO] Create Package Header...\n");


	// copy signature, calculate header size
	memcpy(stHeader.vcSignature, PACKAGESIGNATURE, sizeof(stHeader.vcSignature));
	stHeader.dwHeaderSize = sizeof(PACKAGEHEADER) + (argc - 1) * sizeof(PACKAGEITEM);

	// write them to file
	if( write(iTargetFd, &stHeader, sizeof(stHeader)) != sizeof(stHeader) )
	{
		fprintf(stderr, "[ERROR] Data write fail\n");
		exit(-1);
	}


	// fill package header information
	for(i=1; i<argc; i++)
	{
		// check file info
		if( stat(argv[i], &stFileData) != 0 )
		{
			fprintf(stderr, "[ERROR] %s file open fail\n");
			exit(-1);
		}


		// store file name, file length
		memset(stItem.vcFileName, 0, sizeof(stItem.vcFileName));

		strncpy(stItem.vcFileName, argv[i], sizeof(stItem.vcFileName));
		stItem.vcFileName[sizeof(stItem.vcFileName) - 1] = '\0';

		stItem.dwFileLength = stFileData.st_size;


		// write them to file
		if( write(iTargetFd, &stItem, sizeof(stItem)) != sizeof(stItem) )
		{
			fprintf(stderr, "[ERROR] Data write fail\n");
			exit(-1);
		}


		printf("[%d] file: %s, size: %d Byte\n", i, argv[i], stFileData.st_size);
	}

	printf("[INFO] Create complete\n");


	// --------------------------------------------------------------
	// copy file data behind of package header
	// --------------------------------------------------------------

	printf("[INFO] Copy data file to package...\n");


	// fill file data into img file
	iSourceSize = 0;

	for(i=1; i<argc; i++)
	{
		// open data file
		if( (iSourceFd = open(argv[i], O_RDONLY | O_BINARY)) == -1 )
		{
			fprintf(stderr, "[ERROR] %s open fail\n", argv[i]);
			exit(-1);
		}


		// copy file data into package file, then close
		iSourceSize += CopyFile(iSourceFd, iTargetFd);
		
		close(iSourceFd);
	}


	// align img file by 512 byte
	AdjustInSectorSize(iTargetFd, iSourceSize + stHeader.dwHeaderSize);


	// print success message
	printf("[INFO] Total %d Byte copy complete\n", iSourceSize);
	printf("[INFO] Package file create complete\n");


	close(iTargetFd);

	return 0;
}



int AdjustInSectorSize(int iFd, int iSourceSize)
{
	int i;
	int iAdjustSizeToSector;
	char cCh;
	int iSectorCount;


	iAdjustSizeToSector = iSourceSize % BYTESOFSECTOR;
	cCh = 0x00;


	if(iAdjustSizeToSector != 0)
	{
		iAdjustSizeToSector = 512 - iAdjustSizeToSector;

		for(i=0; i<iAdjustSizeToSector; i++)
		{
			write(iFd, &cCh, 1);
		}
	}
	else
	{
		printf("[INFO] File size is aligned 512 byte\n");
	}


	// return adjusted sector count
	iSectorCount = (iSourceSize + iAdjustSizeToSector) / BYTESOFSECTOR;

	return iSectorCount;
}


int CopyFile(int iSourceFd, int iTargetFd)
{
	int iSourceFileSize;
	int iRead;
	int iWrite;
	char vcBuffer[BYTESOFSECTOR];

	
	iSourceFileSize = 0;

	
	while(1)
	{
		iRead  = read(iSourceFd, vcBuffer, sizeof(vcBuffer));
		iWrite = write(iTargetFd, vcBuffer, iRead);


		if(iRead != iWrite)
		{
			fprintf(stderr, "[ERROR] iRead != iWrite..\n");
			exit(-1);
		}

		iSourceFileSize += iRead;


		if(iRead != sizeof(vcBuffer))
		{
			break;
		}
	}


	return iSourceFileSize;
}



