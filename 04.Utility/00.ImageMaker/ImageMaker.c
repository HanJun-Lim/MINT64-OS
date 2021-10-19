#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define BYTESOFSECTOR	512


int AdjustInSectorSize(int iFd, int iSourceSize);
void WriteKernelInformation(int iTargetFd, int iTotalKernelSectorCount, int iKernel32SectorCount);
int CopyFile(int iSourceFd, int iTargetFd);


int main(int argc, char* argv[])
{
	int iSourceFd;
	int iTargetFd;
	int iBootLoaderSize;
	int iKernel32SectorCount;
	int iKernel64SectorCount;
	int iSourceSize;
	

	// check command line option
	if(argc < 4)		// ImageMaker.exe BootLoader.bin Kernel32.bin Kernel64.bin
	{
		fprintf(stderr, "[ERROR] ImageMaker.exe BootLoader.bin Kernel32.bin Kernel64.bin\n");
		exit(-1);
	}

	// create Disk.img file
	if((iTargetFd = open("Disk.img", O_RDWR|O_CREAT|O_TRUNC|
				O_BINARY, S_IREAD|S_IWRITE)) == -1)
	{
		fprintf(stderr, "[ERROR] Disk.img open failed\n");
		exit(-1);
	}
	
	
	//------------------------------------------------------------------
	// Open Boot Loader file, Copy all contents to Image file
	//------------------------------------------------------------------
	printf("[INFO] Copy boot loader to image file\n");
	
	if((iSourceFd = open(argv[1], O_RDONLY|O_BINARY)) == -1)
	{
		fprintf(stderr, "[ERROR] %s open failed\n", argv[1]);
		exit(-1);
	}

	iSourceSize = CopyFile(iSourceFd, iTargetFd);
	close(iSourceFd);


	// fill the rest with 0x00 to set file size to sector size (512 Byte)
	iBootLoaderSize = AdjustInSectorSize(iTargetFd, iSourceSize);
	printf("[INFO] %s size = [%d] and sector count = [%d]\n",
			argv[1], iSourceSize, iBootLoaderSize);


	//--------------------------------------------------------------------
	// open 32-bit Kernel file and copy all contents to Disk Image file
	//--------------------------------------------------------------------
	printf("[INFO] Copy protected mode kernel to image file\n");
	
	if((iSourceFd = open(argv[2], O_RDONLY|O_BINARY)) == -1)
	{
		fprintf(stderr, "[ERROR] %s open failed\n", argv[2]);
		exit(-1);
	}

	iSourceSize = CopyFile(iSourceFd, iTargetFd);
	close(iSourceFd);

	// fill the rest with 0x00 to set file size to sector size (512 Byte)
	iKernel32SectorCount = AdjustInSectorSize(iTargetFd, iSourceSize);
	
	printf("[INFO] %s size = [%d] and sector count = [%d]\n",
			argv[2], iSourceSize, iKernel32SectorCount);

	
	//--------------------------------------------------------------------
	// open 64-bit Kernel file and copy all contents to Disk Image file
	//--------------------------------------------------------------------
	printf("[INFO] Copy IA-32e mode kernel to image file\n");

	if( (iSourceFd = open(argv[3], O_RDONLY|O_BINARY)) == -1)
	{
		fprintf(stderr, "[ERROR] %s open failed\n", argv[3]);
		exit(-1);
	}

	iSourceSize = CopyFile(iSourceFd, iTargetFd);
	close(iSourceFd);

	// fill the rest with 0x00 to set file size to sector size (512 byte)
	iKernel64SectorCount = AdjustInSectorSize(iTargetFd, iSourceSize);

	printf("[INFO] %s size = [%d] and sector count = [%d]\n",
			argv[3], iSourceSize, iKernel64SectorCount);
	
	//-----------------------------------------------------
	// update kernel information to Disk Image
	//-----------------------------------------------------
	printf("[INFO] Start to write kernel information\n");

	// write kernel information from 5th byte of boot sector
	WriteKernelInformation(iTargetFd, iKernel32SectorCount + iKernel64SectorCount, iKernel32SectorCount);
	printf("[INFO] Image file create complete\n");

	close(iTargetFd);
	return 0;
}


// fill 0x00 until BYTESOFSECTOR
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
		printf("[INFO] File size [%lu] and fill [%u] byte\n", 
				iSourceSize, iAdjustSizeToSector);

		for(i=0; i<iAdjustSizeToSector; i++)
			write(iFd, &cCh, 1);
	}
	else
	{
		printf("[INFO] File size is aligned 512 byte\n");
	}

	// return the number of sector
	iSectorCount = (iSourceSize + iAdjustSizeToSector) / BYTESOFSECTOR;
	return iSectorCount;
}


// put kernel information into Boot Loader (update TOTALSECTORCOUNT)
void WriteKernelInformation(int iTargetFd, int iTotalKernelSectorCount, int iKernel32SectorCount)
{
	unsigned short usData;
	long lPosition;

	// the range from start to 5 byte means the number of sector in kernel
	lPosition = lseek(iTargetFd, (off_t)5, SEEK_SET);
	
	if(lPosition == -1)
	{
		fprintf(stderr, "lseek failed, return value = %d, errno = %d, %d\n",
				lPosition, errno, SEEK_SET);
		exit(-1);
	}

	// save total sector number and Kernel32 sector number except Boot Loader
	usData = (unsigned short)iTotalKernelSectorCount;
	write(iTargetFd, &usData, 2);
	
	usData = (unsigned short)iKernel32SectorCount;
	write(iTargetFd, &usData, 2);

	
	printf("[INFO] Total sector count except boot loader [%d]\n", iTotalKernelSectorCount);
	printf("[INFO] Total sector count of Protected Mode Kernel [%d]\n", iKernel32SectorCount);
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
		iRead = read(iSourceFd, vcBuffer, sizeof(vcBuffer));
		iWrite = write(iTargetFd, vcBuffer, iRead);

		if(iRead != iWrite)
		{
			fprintf(stderr, "[ERROR] iRead != iWrite.. \n");
			exit(-1);
		}
		iSourceFileSize += iRead;

		if(iRead != sizeof(vcBuffer))	// iRead != 512
		{
			break;
		}
	}

	return iSourceFileSize;
}
