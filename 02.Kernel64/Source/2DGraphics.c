#include "2DGraphics.h"
#include "VBE.h"
#include "Font.h"
#include "Utility.h"



// Is (x, y) in Rectangle?
BOOL kIsInRectangle(const RECT* pstArea, int iX, int iY)
{
	// if out of area to draw, no drawing
	if( (iX < pstArea->iX1) || (pstArea->iX2 < iX) || (iY < pstArea->iY1) || (pstArea->iY2 < iY) )
	{
		return FALSE;
	}

	return TRUE;
}


// Return width of Rectangle
int kGetRectangleWidth(const RECT* pstArea)
{
	int iWidth;

	iWidth = pstArea->iX2 - pstArea->iX1 + 1;


	if(iWidth < 0)
	{
		return -iWidth;
	}


	return iWidth;
}


// Return height of Rectangle
int kGetRectangleHeight(const RECT* pstArea)
{
	int iHeight;


	iHeight = pstArea->iY2 - pstArea->iY1 + 1;


	if(iHeight < 0)
	{
		return -iHeight;
	}


	return iHeight;
}


// Check if two Rectangle overlapped
BOOL kIsRectangleOverlapped(const RECT* pstArea1, const RECT* pstArea2)
{
	// The case of not overlapped Rectangle
	//		1. Left Top X of Area1 	   > Right Bottom X of Area2 (Area1 at rear of Area1)
	//		2. Right Bottom X of Area1 < Left Top X of Area2     (Area1 at front of Area2)
	//		3. Left Top Y of Area1 	   > Right Bottom of Area2	 (Area1 at below of Area2)
	//		4. Right Bottom of Area1   < Left Top of Area2		 (Area1 at above of Area2)
	if( (pstArea1->iX1 > pstArea2->iX2) || (pstArea1->iX2 < pstArea2->iX1) ||
		(pstArea1->iY1 > pstArea2->iY2) || (pstArea1->iY2 < pstArea2->iY1) )
	{
		return FALSE;
	}


	return TRUE;
}


BOOL kGetOverlappedRectangle(const RECT* pstArea1, const RECT* pstArea2, RECT* pstIntersection)
{
	int iMaxX1;
	int iMinX2;
	int iMaxY1;
	int iMinY2;

	
	// Start Point X is maximum value on Left Top X
	iMaxX1 = MAX(pstArea1->iX1, pstArea2->iX1);
	
	// End Point X is minimum value on Right Bottom X
	iMinX2 = MIN(pstArea1->iX2, pstArea2->iX2);

	// if Start Point bigger than End Point, it means not overlapped
	if(iMinX2 < iMaxX1)
	{
		return FALSE;
	}


	// Start Point Y is maximum value on Left Top Y
	iMaxY1 = MAX(pstArea1->iY1, pstArea2->iY1);

	// End Point Y is minimum value on Right Bottom Y
	iMinY2 = MIN(pstArea1->iY2, pstArea2->iY2);

	// if Start Point bigger than End Point, it means not overlapped
	if(iMinY2 < iMaxY1)
	{
		return FALSE;
	}


	// save information of overlapped area
	pstIntersection->iX1 = iMaxX1;
	pstIntersection->iY1 = iMaxY1;
	pstIntersection->iX2 = iMinX2;
	pstIntersection->iY2 = iMinY2;


	return TRUE;
}


// Fill information into RECT Data Structure
// 		save coordinates as X1 < X2, Y1 < Y2
void kSetRectangleData(int iX1, int iY1, int iX2, int iY2, RECT* pstRect)
{
	// make X coord to X1 < X2
	if(iX1 < iX2)
	{
		pstRect->iX1 = iX1;
		pstRect->iX2 = iX2;
	}
	else
	{
		pstRect->iX1 = iX2;
		pstRect->iX2 = iX1;
	}


	// make X coord to X1 < X2
	if(iY1 < iY2)
	{
		pstRect->iY1 = iY1;
		pstRect->iY2 = iY2;
	}
	else
	{
		pstRect->iY1 = iY2;
		pstRect->iY2 = iY1;
	}
}


void kInternalDrawPixel(const RECT* pstMemoryArea, COLOR* pstMemoryAddress,
							   int iX, int iY, COLOR stColor)
{
	int iWidth;


	// Clipping
	// 		if out of area to draw, no drawing
	if(kIsInRectangle(pstMemoryArea, iX, iY) == FALSE)
	{
		return;
	}


	// get width of Rectangle to print
	iWidth = kGetRectangleWidth(pstMemoryArea);


	// get Pixel Offset, print Pixel
	*(pstMemoryAddress + (iWidth * iY) + iX) = stColor;
}



void kInternalDrawLine(const RECT* pstMemoryArea, COLOR* pstMemoryAddress, 
					   int iX1, int iY1, int iX2, int iY2, COLOR stColor)
{
	int iDeltaX, iDeltaY;
	int iError = 0;
	int iDeltaError;
	int iX, iY;
	int iStepX, iStepY;
	RECT stLineArea;


	// Clipping
	// 		if no overlapped area between Memory Area and Line, no drawing
	kSetRectangleData(iX1, iY1, iX2, iY2, &stLineArea);

	if(kIsRectangleOverlapped(pstMemoryArea, &stLineArea) == FALSE)
	{
		return;
	}
	

	// calculate Change Amount
	iDeltaX = iX2 - iX1;
	iDeltaY = iY2 - iY1;

	
	// calculate X-axis direction of change, according to X-axis Change Amount
	// a line grows right to left
	if(iDeltaX < 0)
	{
		iDeltaX = -iDeltaX;
		iStepX  = -1;
	}
	// a line grows left to right
	else
	{
		iStepX  = 1;
	}


	// calculate Y-axis direction of change, according to Y-axis Change Amount
	// a line grows up to down
	if(iDeltaY < 0)
	{
		iDeltaY = -iDeltaY;
		iStepY  = -1;
	}
	// a line grows down to up
	else
	{
		iStepY  = 1;
	}


	// if Change Amount of X-axis larger than Change Amount of Y-axis, (in case that the slope is gentle)
	// 		draw a line focusing on X-axis
	if(iDeltaX > iDeltaY)
	{
		// Error to add per Pixel, Change Amount of Y * 2
		//		* 2 replaced by Lshift (optimization)
		iDeltaError = iDeltaY << 1;

		iY = iY1;

		for(iX = iX1; iX != iX2; iX += iStepX)
		{
			// draw Pixel
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iX, iY, stColor);

			// accumulate Error
			iError += iDeltaError;


			// if accumulated Error bigger than Change Amount of X, 
			// 		choose upper Pixel, and update Error based on upper Pixel
			if(iError >= iDeltaX)
			{
				iY += iStepY;

				// Error = Error - Change Amount of X * 2
				//		* 2 replaced by Lshift (optimization)
				iError -= iDeltaX << 1;
			}
		}

		// draw Pixel at the position where iX == iX2
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iX, iY, stColor);
	}

	// if Change Amount of Y-axis larger than Change Amount of X-axis, (in case that the slope is steep)
	// 		draw a line focusing on Y-axis
	else
	{
		// Error to add per Pixel, Change Amount of X * 2
		// 		* 2 replaced by Lshift (optimization)
		iDeltaError = iDeltaX << 1;

		iX = iX1;

		for(iY = iY1; iY != iY2; iY += iStepY)
		{
			// draw Pixel
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iX, iY, stColor);

			// accumulate Error
			iError += iDeltaError;

			
			// if accumulated Error bigger than Change Amount of Y,
			// 		choose upper Pixel, and update Error based on upper Pixel
			if(iError >= iDeltaY)
			{
				iX += iStepX;

				// Error = Error - Change Amount of Y * 2
				// 		* 2 replaced by Lshift (optimization)
				iError -= iDeltaY << 1;
			}
		}

		// draw Pixel at the position where iY == iY2
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iX, iY, stColor);
	}
}


void kInternalDrawRect(const RECT* pstMemoryArea, COLOR* pstMemoryAddress, 
					   int iX1, int iY1, int iX2, int iY2, COLOR stColor, BOOL bFill)
{
	int iWidth;
	int iTemp;
	int iY;
	int iMemoryAreaWidth;
	RECT stDrawRect;
	RECT stOverlappedArea;

	
	// branch code by checking bFill
	if(bFill == FALSE)
	{
		// connect neighbor Pixels with a line
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, iX1, iY1, iX2, iY1, stColor);
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, iX1, iY1, iX1, iY2, stColor);
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, iX2, iY1, iX2, iY2, stColor);
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, iX1, iY2, iX2, iY2, stColor);
	}
	else
	{
		// save the information of Rectangle to print to RECT Data Structure
		kSetRectangleData(iX1, iY1, iX2, iY2, &stDrawRect);


		// Clipping
		// 		by calculate overlapped area
		if(kGetOverlappedRectangle(pstMemoryArea, &stDrawRect, &stOverlappedArea) == FALSE)
		{
			// if there's no intersection, no need to draw
			return;
		}


		// calculate width of clipped Rectangle
		iWidth = kGetRectangleWidth(&stOverlappedArea);


		// calculate width of Memory Area to print
		iMemoryAreaWidth = kGetRectangleWidth(pstMemoryArea);


		// calculate Start Address of Memory Address to print
		//		draw based on clipped Rectangle
		pstMemoryAddress += stOverlappedArea.iY1 * iMemoryAreaWidth + stOverlappedArea.iX1;


		// fill Data by looping for Y
		for(iY = stOverlappedArea.iY1; iY < stOverlappedArea.iY2; iY++)
		{
			// fill Data as much as width of overlapped area
			kMemSetWord(pstMemoryAddress, stColor, iWidth);


			// update Video Memory Address to print
			//		calculate Y of next line by using X resolution
			pstMemoryAddress += iMemoryAreaWidth;
		}


		// fill Data as much as width of overlapped area
		kMemSetWord(pstMemoryAddress, stColor, iWidth);
	}	
}


void kInternalDrawCircle(const RECT* pstMemoryArea, COLOR* pstMemoryAddress,
						 int iX, int iY, int iRadius, COLOR stColor, BOOL bFill)
{
	// iX, iY means Intersection Point between X, Y (Midpoint of Circle)
	int iCircleX, iCircleY;			// means current X, Y Pixel pos
	int iDistance;


	// if Radius is negative, no need to draw
	if(iRadius < 0)
	{
		return;
	}

	
	// starts from (0, R)
	iCircleY = iRadius;


	// branch code by checking bFill
	if(bFill == FALSE)
	{
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, 0 + iX		 , iRadius + iY , stColor);
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, 0 + iX		 , -iRadius + iY, stColor);
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, iRadius + iX , 0 + iY		, stColor);
		kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, -iRadius + iX, 0 + iY		, stColor);
	}
	else
	{
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, 0 + iX	   , iRadius + iY, 0 + iX 		, -iRadius + iY, stColor);
		kInternalDrawLine(pstMemoryArea, pstMemoryAddress, iRadius + iX, 0 + iY		 , -iRadius + iX, 0 + iY	   , stColor);
	}


	// distance between Midpoint of starting point and Circle
	// 		d = -(r^2)  ->  d_old
	iDistance = -iRadius;


	// draw Circle
	for(iCircleX = 1; iCircleX <= iCircleY; iCircleX++)		// iCircle <= iCircleY means being in angle of 45 degree (1/8)
	{
		// get distance from Circle
		// 		'*' replaced by Lshift (optimization)
		iDistance += (iCircleX << 1) - 1;			// 2 * iCircleX - 1  ->  d_new1

		
		// if Midpoint is not in range of Circle, choose below point
		if(iDistance >= 0)		// Discriminant, '=' added for correction of error
		{
			iCircleY--;


			// calculate distance from Circle at new target point
			iDistance += (-iCircleY << 1) + 2;		// -2 * iCircleY + 2  -> d_new2
		}


		if(bFill == FALSE)
		{
			// draw Pixel in all direction (8 directions)
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress,  iCircleX + iX,  iCircleY + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress,  iCircleX + iX, -iCircleY + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, -iCircleX + iX,  iCircleY + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, -iCircleX + iX, -iCircleY + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress,  iCircleY + iX,  iCircleX + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress,  iCircleY + iX, -iCircleX + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, -iCircleY + iX,  iCircleX + iY, stColor);
			kInternalDrawPixel(pstMemoryArea, pstMemoryAddress, -iCircleY + iX, -iCircleX + iY, stColor);
		}
		else
		{
			// find symmetrical point, draw filled Circle by drawing a parallel line
			// parallel line can be drawn faster by kInternalDrawRect()
			kInternalDrawRect(pstMemoryArea, pstMemoryAddress, 
							  -iCircleX + iX,  iCircleY + iY, iCircleX + iX,  iCircleY + iY, stColor, TRUE);
			kInternalDrawRect(pstMemoryArea, pstMemoryAddress,
							  -iCircleX + iX, -iCircleY + iY, iCircleX + iX, -iCircleY + iY, stColor, TRUE);
			kInternalDrawRect(pstMemoryArea, pstMemoryAddress,
							  -iCircleY + iX,  iCircleX + iY, iCircleY + iX,  iCircleX + iY, stColor, TRUE);
			kInternalDrawRect(pstMemoryArea, pstMemoryAddress,
							  -iCircleY + iX, -iCircleX + iY, iCircleY + iX, -iCircleX + iY, stColor, TRUE);
		}
	}
}


void kInternalDrawText(const RECT* pstMemoryArea, COLOR* pstMemoryAddress,
					   int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor, const char* pcString, int iLength)
{
	int i;
	int j;


	for(i=0; i<iLength; )
	{
		// if the string is not hangul, find the end of english text
		if( (pcString[i] & 0x80) == 0 )
		{
			for(j=i; j<iLength; j++)
			{
				// break if the next character is hangul
				if(pcString[j] & 0x80)
				{
					break;
				}
			}


			// call drawing function of english, update current position
			kInternalDrawEnglishText(pstMemoryArea, pstMemoryAddress, iX + (i * FONT_ENGLISHWIDTH), iY,
									 stTextColor, stBackgroundColor, pcString + i, j - i);

			i = j;
		}
		
		// if the string is hangle, find the end of hangle text
		else
		{
			for(j=i; j<iLength; j++)
			{
				// break if the next character is english
				if( (pcString[j] & 0x80) == 0 )
				{
					break;
				}
			}


			// call drawing function of hangul, update current position
			kInternalDrawHangulText(pstMemoryArea, pstMemoryAddress, iX + i * FONT_ENGLISHWIDTH, iY,
									stTextColor, stBackgroundColor, pcString + i, j - i);

			i = j;
		}
	}
}


void kInternalDrawEnglishText(const RECT* pstMemoryArea, COLOR* pstMemoryAddress, 
					  		  int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor, const char* pcString, int iLength)
{
	int  iCurrentX, iCurrentY;
	int  i, j, k;
	BYTE bBitmap;
	BYTE bCurrentBitmask;
	int  iBitmapStartIndex;
	int  iMemoryAreaWidth;
	RECT stFontArea;
	RECT stOverlappedArea;
	int  iStartYOffset;
	int  iStartXOffset;
	int  iOverlappedWidth;
	int  iOverlappedHeight;

	
	// X pos to print Character
	iCurrentX = iX;


	// calculate width of Memory Area
	iMemoryAreaWidth = kGetRectangleWidth(pstMemoryArea);

	
	// loop for number of Character
	for(k=0; k<iLength; k++)
	{
		// get Y pos to print Character
		iCurrentY = iY * iMemoryAreaWidth;

		
		// set current Font Printing Area to RECT Data Structure
		kSetRectangleData(iCurrentX, iY, iCurrentX + FONT_ENGLISHWIDTH - 1, iY + FONT_ENGLISHHEIGHT - 1, &stFontArea);


		// if current Character to draw is not overlapped with Memory Area, move to next Character
		if(kGetOverlappedRectangle(pstMemoryArea, &stFontArea, &stOverlappedArea) == FALSE)
		{
			// move to next Character coordinate
			iCurrentX += FONT_ENGLISHWIDTH;

			continue;
		}


		// calculate Start pos of Character Bitmap in Bitmap Font Data
		// 		Start Offset of Bitmap Character = ASCII of Character to print * FONT_HEIGHT
		// 		
		// 		ex) "Hello"
		// 			pcString[0] means 'H', pcString[1] means 'e'...
		iBitmapStartIndex = pcString[k] * FONT_ENGLISHHEIGHT;


		// get X, Y offset, Width, Heignt to print by using overlapped area
		iStartXOffset = stOverlappedArea.iX1 - iCurrentX;
		iStartYOffset = stOverlappedArea.iY1 - iY;
		iOverlappedWidth  = kGetRectangleWidth(&stOverlappedArea);
		iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);


		// calculate Start pos of Character Bitmap in Bitmap Font Data
		iBitmapStartIndex += iStartYOffset;


		// print one Character
		for(j=iStartYOffset; j<iOverlappedHeight; j++)
		{
			// calculate Bit Offset, Font Bitmap to print on current line
			bBitmap = g_vucEnglishFont[iBitmapStartIndex++];
			bCurrentBitmask = 0x01 << (FONT_ENGLISHWIDTH - 1 - iStartXOffset);

			
			// print from Overlapped Area X to Overlapped width
			for(i=iStartXOffset; i<iOverlappedWidth; i++)
			{
				// if bit is set (1), print Text Color
				if( bBitmap & bCurrentBitmask )
				{
					pstMemoryAddress[iCurrentY + iCurrentX + i] = stTextColor;
				}
				// if bit is not set (0), print Background Color
				else
				{
					pstMemoryAddress[iCurrentY + iCurrentX + i] = stBackgroundColor;
				}

				bCurrentBitmask = bCurrentBitmask >> 1;
			}

			// move to next line
			iCurrentY += iMemoryAreaWidth;
		}

		// print next Character
		iCurrentX += FONT_ENGLISHWIDTH;
	}
}


void kInternalDrawHangulText(const RECT* pstMemoryArea, COLOR* pstMemoryAddress,
						 	 int iX, int iY, COLOR stTextColor, COLOR stBackgroundColor, const char* pcString, int iLength)
{
	int iCurrentX, iCurrentY;
	WORD wHangul;
	WORD wOffsetInGroup;
	WORD wGroupIndex;
	int i, j, k;
	WORD wBitmap;
	WORD wCurrentBitmask;
	int iBitmapStartIndex;
	int iMemoryAreaWidth;
	RECT stFontArea;
	RECT stOverlappedArea;
	int iStartYOffset;
	int iStartXOffset;
	int iOverlappedWidth;
	int iOverlappedHeight;


	iCurrentX = iX;
	

	// calculate width of memory area
	iMemoryAreaWidth = kGetRectangleWidth(pstMemoryArea);


	// loop for length of hangle text
	for(k=0; k<iLength; k += 2)
	{
		// get Y coord to print character
		iCurrentY = iY * iMemoryAreaWidth;

		
		// save information of hangul text area into RECT structure
		kSetRectangleData(iCurrentX, iY, iCurrentX + FONT_HANGULWIDTH - 1, iY + FONT_HANGULHEIGHT - 1, &stFontArea);


		// if a character to draw has not overlapped area with Memory area, move to next character (clipping)
		if( kGetOverlappedRectangle(pstMemoryArea, &stFontArea, &stOverlappedArea) == FALSE )
		{
			// move to next character
			iCurrentX += FONT_HANGULWIDTH;

			continue;
		}


		// calculate the start position of text to print in bitmap font data (g_vusHangulFont)
		//		2 byte * FONT_HEIGHT
		wHangul = ((WORD)pcString[k] << 8) | (BYTE)(pcString[k+1]);


		// if not 자/모 (완성형 가~힝), add 자/모 offset
		if( (0xB0A1 <= wHangul) && (wHangul <= 0xC8FE) )
		{
			wOffsetInGroup = (wHangul - 0xB0A1) & 0xFF;
			wGroupIndex = ((wHangul - 0xB0A1) >> 8) & 0xFF;

			// 94 letters in one group, else 51 letters contains 자/모 (that is not 완성형)
			wHangul = (wGroupIndex * 94) + wOffsetInGroup + 51;
		}
		
		// if 자/모, get offset by subtracting "ㄱ" (first of 자음)
		else if( (0xA4A1 <= wHangul) && (wHangul <= 0xA4D3) )
		{
			wHangul = wHangul - 0xA4A1;
		}

		// other case is unsupported : move to next character
		else
		{
			continue;
		}


		iBitmapStartIndex = wHangul * FONT_HANGULHEIGHT;


		// calculate X, Y offset and width/height to print 
		// 		.. using overlapped area between memory area and character print area
		iStartXOffset = stOverlappedArea.iX1 - iCurrentX;
		iStartYOffset = stOverlappedArea.iY1 - iY;
		iOverlappedWidth = kGetRectangleWidth(&stOverlappedArea);
		iOverlappedHeight = kGetRectangleHeight(&stOverlappedArea);


		// except bitmap data as much as excepted Y offset to print
		iBitmapStartIndex += iStartYOffset;


		// print letter(s)
		for(j = iStartYOffset; j < iOverlappedHeight; j++)
		{
			// calculate font bitmap, bit offset to print
			wBitmap = g_vusHangulFont[iBitmapStartIndex++];
			wCurrentBitmask = 0x01 << (FONT_HANGULWIDTH - 1 - iStartXOffset);


			// print to current line from X offset to width of overlapped area
			for(i = iStartXOffset; i < iOverlappedWidth; i++)
			{
				// if the bit set, fill text color
				if(wBitmap & wCurrentBitmask)
				{
					pstMemoryAddress[iCurrentY + iCurrentX + i] = stTextColor;
				}

				// if the bit clear, fill background color
				else
				{
					pstMemoryAddress[iCurrentY + iCurrentX + i] = stBackgroundColor;
				}


				// check next bit
				wCurrentBitmask = wCurrentBitmask >> 1;

			}	// pixel drawing loop end


			// check next line
			iCurrentY += iMemoryAreaWidth;

		}	// letter print loop end


		// print next letter
		iCurrentX += FONT_HANGULWIDTH;

	}	// hangul text loop end
}



