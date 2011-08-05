/// Cross Platform Bitmap functions - Sandy Barbour 2003
#if IBM
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "BitmapSupport.h"

/// Functions to handle endian differeneces between windows, linux and mac.
#if APL
short Endian(short Data)
{
  unsigned char *pBuffer = (unsigned char *)&Data;
  short Result = (short)(pBuffer[0] & 0xff) + ( (short)(pBuffer[1] & 0xff) << 8) ;
  return(Result);
}

long Endian(long Data)
{
  unsigned char *pBuffer = (unsigned char *)&Data;

  long Result =   (long)(pBuffer[0] & 0xff)
                  + ( (long)(pBuffer[1] & 0xff) << 8)
                  + ( (long)(pBuffer[2] & 0xff) << 16)
                  + ( (long)(pBuffer[3] & 0xff) << 24);

  return(Result);
}

void SwapEndian(short *Data)
{
  *Data = Endian(*Data);
}

void SwapEndian(long *Data)
{
  *Data = Endian(*Data);
}
#else
/// Only the mac needs these so dummy functions for windows and linux.
void SwapEndian(short *Data){
}
void SwapEndian(long *Data){
}
#endif

/// Swap the red and blue pixels.
void SwapRedBlue(IMAGEDATA *ImageData)
{
  unsigned char  * srcPixel;
  long  count;
  long x,y;
  unsigned char sTemp;

  /// Does not support 4 channels.
  if (ImageData->Channels == 4)
    return;

  /// Do the swap
  srcPixel = ImageData->pData;
  count = ImageData->Width * ImageData->Height;
  for (y = 0; y < ImageData->Height; ++y)
    for (x = 0; x < ImageData->Width; ++x)
    {
      sTemp = srcPixel[0];
      srcPixel[0] = srcPixel[2];
      srcPixel[2] = sTemp;

      srcPixel += 3;
      if (x == (ImageData->Width - 1))
        srcPixel += ImageData->Padding;
    }
}

/// Generic bitmap loader to handle all platforms
int BitmapLoader(const char * FilePath, IMAGEDATA * ImageData)
{
  BMPFILEHEADER   Header;
  BMPINFOHEADER ImageInfo;
  long     Padding;
  FILE *     BitmapFile = NULL;
  int RetCode = 0;

  ImageData->pData = NULL;

  BitmapFile = fopen(FilePath, "rb");
  if (BitmapFile != NULL)
  {
    if (fread(&Header, sizeof(Header), 1, BitmapFile) == 1)
    {
      if (fread(&ImageInfo, sizeof(ImageInfo), 1, BitmapFile) == 1)
      {
        /// Handle Header endian.
        SwapEndian(&Header.bfSize);
        SwapEndian(&Header.bfOffBits);

        /// Handle ImageInfo endian.
        SwapEndian(&ImageInfo.biWidth);
        SwapEndian(&ImageInfo.biHeight);
        SwapEndian(&ImageInfo.biBitCount);

        /// Make sure that it is a bitmap.
#if APL && defined(__POWERPC__)

        if (((Header.bfType & 0xff) == 'M') &&
                (((Header.bfType >> 8) & 0xff) == 'B') &&
#else
                if (((Header.bfType & 0xff) == 'B') &&
                          (((Header.bfType >> 8) & 0xff) == 'M') &&
#endif
                          (ImageInfo.biBitCount == 24) &&
                          (ImageInfo.biWidth > 0) &&
                          (ImageInfo.biHeight > 0))
                {
                  /// Sandy Barbour - I have found that "Header.bfSize" does not always agree
                  /// with the actual file size and can sometimes be "ImageInfo.biSize"  smaller.
                  /// So add it in for good measure
                  if ((Header.bfSize + ImageInfo.biSize - Header.bfOffBits) >= (ImageInfo.biWidth * ImageInfo.biHeight * 3))
                    {
                      Padding = (ImageInfo.biWidth * 3 + 3) & ~3;
                      Padding -= ImageInfo.biWidth * 3;

                      ImageData->Width = ImageInfo.biWidth;
                      ImageData->Height = ImageInfo.biHeight;
                      ImageData->Padding = Padding;

                      /// Allocate memory for the actual image.
                      ImageData->Channels = 3;
                      ImageData->pData = (unsigned char *) malloc(ImageInfo.biWidth * ImageInfo.biHeight * ImageData->Channels + ImageInfo.biHeight * Padding);

                      if (ImageData->pData != NULL)
                      {
                        /// Get the actual image.
                        if (fread(ImageData->pData, ImageInfo.biWidth * ImageInfo.biHeight * ImageData->Channels + ImageInfo.biHeight * Padding, 1, BitmapFile) == 1)
                        {
                          RetCode = 1;
                        }
                      }
                    }
                  }
              }
            }
          }
          if (BitmapFile != NULL)
            fclose(BitmapFile);
  return RetCode;
}


