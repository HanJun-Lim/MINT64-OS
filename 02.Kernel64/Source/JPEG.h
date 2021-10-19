/*
 *	JPEG decoding engine for DCT(Discrete Cosine Transform)-baseline
 *
 *	
 *	JPEG Encoding/Decoding Process
 *
 *  Encoding : RGB -> YCbCr (Down sampling) >> Discrete Cosine Transform >> Quantization >> Entropy Encoding >> JPEG Image
 *  Decoding : JPEG Image >> Entropy Decoding >> inverse Quantization >> inverse Discrete Cosine Transform >> YCbCr -> RGB
 *	
 */


#ifndef __JPEG_H__
#define __JPEG_H__

#include "Types.h"
#include "2DGraphics.h"



// ---------------------------------------------
// Structures
// ---------------------------------------------

// Huffman Table

typedef struct
{
	int 		   elem;			// element count
	unsigned short code[256];
	unsigned char  size[256];
	unsigned char  value[256];
} HUFF;


// for JPEG Decoding, contains important information used in Encoding Process
typedef struct
{
	// SOF (Start Of Frame)
	int width;					// image width
	int height;					// image height


	// MCU (Minimum Coded Unit)
	int mcu_width;
	int mcu_height;

	int max_h, max_v;
	int compo_count;
	int compo_id[3];
	int compo_sample[3];
	int compo_h[3];
	int compo_v[3];
	int compo_qt[3];


	// SOS (Start Of Scan)
	int scan_count;
	int scan_id[3];
	int scan_ac[3];
	int scan_dc[3];
	int scan_h[3];				// sampling element count
	int scan_v[3];				// sampling element count
	int scan_qt[3];				// quantization table index
	

	// DRI (Define Restart Interval)
	int  interval;

	int  mcu_buf[32 * 32 * 4];	// buffer
	int* mcu_yuv[4];
	int  mcu_preDC[3];


	// DQT (Define Quantization Table)
	int dqt[3][64];
	int n_dqt;


	// DHT (Define Huffman Tables)
	HUFF huff[2][3];			// 6 Huffman tables, about 6 KB


	// I/O
	unsigned char* data;
	int 		   data_index;
	int 		   data_size;

	unsigned long  bit_buff;
	int 		   bit_remain;

} JPEG;



// ---------------------------------------------
// Function
// ---------------------------------------------

BOOL kJPEGInit(JPEG* jpeg, BYTE* pbFileBuffer, DWORD dwFileSize);
BOOL kJPEGDecode(JPEG* jpeg, COLOR* rgb);



#endif
