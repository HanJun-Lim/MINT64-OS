/**
 *  file    SerialTransferForWindow.c
 *  date    2009/06/06
 *  author  kkamagui 
 *          Copyright(c)2008 All rights reserved by kkamagui 
 *  brief   �ø��� ��Ʈ�� �����͸� �����ϴµ� ���α׷��� �ҽ� ����
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <windows.h>

// ��Ÿ ��ũ��
#define DWORD               unsigned long
#define BYTE                unsigned char
#define MIN( x, y )         ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )

// �ø��� ��Ʈ FIFO�� �ִ� ũ��
#define SERIAL_FIFOMAXSIZE  16

/**
 *  main �Լ�
 */
int main( int argc, char** argv )
{
    char vcFileName[ 256 ];
    char vcDataBuffer[ SERIAL_FIFOMAXSIZE ];
    HANDLE hSerialPort;
    DCB stDCB;
    BYTE bAck;
    DWORD dwDataLength;
    DWORD dwSentSize;
    DWORD dwTemp;
	DWORD dwProcessedSize;
    FILE* fp;
    
    //--------------------------------------------------------------------------
    // ���� ����
    //--------------------------------------------------------------------------
    // ���� �̸��� �Է����� �ʾ����� ���� �̸��� �Է� ����
    if( argc < 2 )
    {
        fprintf( stderr, "Input File Name: " );
        gets( vcFileName );
    }
    // ���� �̸��� ���� �ÿ� �Է��ߴٸ� ������
    else
    {
        strcpy( vcFileName, argv[ 1 ] );
    }

    // ���� ���� �õ�
    fp = fopen( vcFileName, "rb" );
    if( fp == NULL )
    {
        fprintf( stderr, "%s File Open Error\n", vcFileName );
        return 0;
    }
    
    // fseek�� ���� ������ �̵��Ͽ� ������ ���̸� ������ ��, �ٽ� ������ ó������ �̵�
    fseek( fp, 0, SEEK_END );
    dwDataLength = ftell( fp );
    fseek( fp, 0, SEEK_SET );
    fprintf( stderr, "File Name %s, Data Length %d Byte\n", vcFileName, 
            dwDataLength );
    
    //--------------------------------------------------------------------------
    // �ø��� ��Ʈ ����
    //--------------------------------------------------------------------------
    // �ø��� ��Ʈ ����
    hSerialPort = CreateFile( "COM1", GENERIC_READ|GENERIC_WRITE, 0, 0, 
                              OPEN_EXISTING, 0, 0 );
    if( hSerialPort == INVALID_HANDLE_VALUE )
    {
        fprintf( stderr, "COM1 Open Error\n" );
        return 0;
    }
    else
    {
        fprintf( stderr, "COM1 Open Success\n" );
    }
    
    // ��� �ӵ�, �и�Ƽ, ���� ��Ʈ ����
    memset( &stDCB, 0, sizeof( stDCB ) );
    stDCB.fBinary = TRUE;           // Binary Transfer
    stDCB.BaudRate = CBR_115200 ;   // 115200 bps
    stDCB.ByteSize = 8;             // 8bit Data
    stDCB.Parity = 0;               // No Parity
    stDCB.StopBits = ONESTOPBIT;    // 1 Stop Bit
    if( SetCommState( hSerialPort, &stDCB ) == FALSE )
    {
        fprintf( stderr, "COM1 Set Value Fail\n" );
        return 0;
    }
    
    //--------------------------------------------------------------------------
    // ������ ����
    //--------------------------------------------------------------------------
    // ������ ���̸� ����
    if( ( WriteFile( hSerialPort, &dwDataLength, 4, &dwProcessedSize, NULL ) == FALSE ) ||
		( dwProcessedSize != 4 ) )
    {
        fprintf( stderr, "Data Length Send Fail, [%d] Byte\n", dwDataLength );
        return 0;
    }
    else
    {
        fprintf( stderr, "Data Length Send Success, [%d] Byte\n", dwDataLength );
    }
    // Ack�� ���ŵ� ������ ���
    if( ( ReadFile( hSerialPort, &bAck, 1, &dwProcessedSize, 0 ) == FALSE ) ||
        ( dwProcessedSize != 1 ) )
    {
        fprintf( stderr, "Ack Receive Error\n" );
        return 0;
    }    
    
    // ������ ����
    fprintf( stderr, "Now Data Transfer..." );
    dwSentSize = 0;
    while( dwSentSize < dwDataLength )
    {
        // ���� ũ��� FIFO�� �ִ� ũ�� �߿��� ���� ���� ����
        dwTemp = MIN( dwDataLength - dwSentSize, SERIAL_FIFOMAXSIZE );
        dwSentSize += dwTemp;
        
        if( fread( vcDataBuffer, 1, dwTemp, fp ) != dwTemp )
        {
            fprintf( stderr, "File Read Error\n" );
            return 0;
        }
        
        // �����͸� ����
        if( ( WriteFile( hSerialPort, vcDataBuffer, dwTemp, &dwProcessedSize, 0 ) == FALSE ) ||
			( dwProcessedSize != dwTemp ) )
        {
            fprintf( stderr, "Socket Send Error\n" );
            return 0;
        }
        
        // Ack�� ���ŵ� ������ ���
        if( ( ReadFile( hSerialPort, &bAck, 1, &dwProcessedSize, 0 ) == FALSE ) ||
			( dwProcessedSize != 1 ) )
        {
            fprintf( stderr, "Ack Receive Error\n" );
            return 0;
        }
        // ���� ��Ȳ ǥ��
        fprintf( stderr, "#" );
    }
    
    // ���ϰ� �ø����� ����
    fclose( fp );
    CloseHandle( hSerialPort );
    
    // ������ �Ϸ�Ǿ����� ǥ���ϰ� ���� Ű�� ���
    fprintf( stderr, "\nSend Complete. Total Size [%d] Byte\n", dwSentSize );
    fprintf( stderr, "Press Enter Key To Exit\n" );
    getchar();
    
    return 0;
}
