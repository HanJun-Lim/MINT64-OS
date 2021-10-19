#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <arpa/inet.h>
#include <unistd.h>


// other macros
#define DWORD				unsigned int
#define BYTE				unsigned char
#define MIN(x,y)			( (x) < (y) ? (x) : (y) )


// max Serial FIFO size
#define SERIAL_FIFOMAXSIZE	16


int main(int argc, char** argv)
{
	char  vcFileName[256];
	char  vcDataBuffer[SERIAL_FIFOMAXSIZE];
	struct sockaddr_in stSocketAddr;
	int   iSocket;
	BYTE  bAck;
	DWORD dwDataLength;
	DWORD dwSentSize;
	DWORD dwTemp;
	FILE* fp;


	// =================================================
	// Open File
	// =================================================
	
	// if File name not given, input File name
	if(argc < 2)
	{
		fprintf(stderr, "Input File Name: ");
		gets(vcFileName);
	}
	// if File name given, copy
	else
	{
		strcpy(vcFileName, argv[1]);
	}


	fp = fopen(vcFileName, "rb");
	
	if(fp == NULL)
	{
		fprintf(stderr, "%s File Open Error\n", vcFileName);
		return 0;
	}

	
	// measure File Length using fseek()
	fseek(fp, 0, SEEK_END);

	dwDataLength = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	fprintf(stderr, "File Name %s, Data Length %d Byte\n", vcFileName, dwDataLength);


	// =================================================
	// Connect to Network
	// =================================================
	
	// set Virtual Machine Address
	stSocketAddr.sin_family 	 = AF_INET;					// Address Family - Internet Protocol v4
	stSocketAddr.sin_port		 = htons(4444);				// convert Host Byte order to Network Byte order
	stSocketAddr.sin_addr.s_addr = inet_addr("127.0.0.1");	// convert "127.0.0.1" to binary data in Network Byte order

	
	// create Socket, and try connection to Virtual Machine (Server)
	iSocket = socket(AF_INET, SOCK_STREAM, 0);

	if(connect(iSocket, (struct sockaddr*)&stSocketAddr, sizeof(stSocketAddr)) == -1)
	{
		fprintf(stderr, "Socket Connect Error, IP 127.0.0.1, Port 4444\n");
		return 0;
	}
	else
	{
		fprintf(stderr, "Socket Connect Success, IP 127.0.0.1, Port 4444\n");
	}


	// =================================================
	// Send Data
	// =================================================
	
	// 1. send Data length
	if(send(iSocket, &dwDataLength, 4, 0) != 4)
	{
		fprintf(stderr, "Data Length Send Fail, [%d] Byte\n", dwDataLength);
		return 0;
	}
	else
	{
		fprintf(stderr, "Data Length Send Success, [%d] Byte\n", dwDataLength);	
	}


	// 2. wait for ACK
	if(recv(iSocket, &bAck, 1, 0) != 1)
	{
		fprintf(stderr, "Ack Receive Error\n");
		return 0;
	}


	// 3. send Data
	fprintf(stderr, "Now Data Transfer...");

	dwSentSize = 0;

	while(dwSentSize < dwDataLength)
	{
		dwTemp = MIN(dwDataLength - dwSentSize, SERIAL_FIFOMAXSIZE);
		dwSentSize += dwTemp;

		// read data and load it to Data Buffer
		if(fread(vcDataBuffer, 1, dwTemp, fp) != dwTemp)
		{
			fprintf(stderr, "File Read Error\n");
			return 0;
		}

		// send loaded data
		if(send(iSocket, vcDataBuffer, dwTemp, 0) != dwTemp)
		{
			fprintf(stderr, "Socket Send Error\n");
			return 0;
		}

		// wait for ACK
		if(recv(iSocket, &bAck, 1, 0) != 1)
		{
			fprintf(stderr, "Ack Receive Error\n");
			return 0;
		}

		// printf current progress
		fprintf(stderr, "#");
	}	// send Data loop end

	
	// close File and Socket
	fclose(fp);
	close(iSocket);


	fprintf(stderr, "\nSend Complete, [%d] Byte\n", dwSentSize);
	fprintf(stderr, "Press Enter Key To Exit\n");
	getchar();

	return 0;
}


