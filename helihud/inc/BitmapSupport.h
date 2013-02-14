/// Cross Platform Bitmap functions and bitmap data structures - Sandy Barbour 2003

#ifndef BitmapSupportH
#define BitmapSupportH

/// These need to be aligned
#pragma pack(push, ident, 2)

typedef struct tagBMPFILEHEADER
{
    short   bfType;
    long    bfSize;
    short   bfReserved1;
    short   bfReserved2;
    long    bfOffBits;
} BMPFILEHEADER;

typedef struct tagBMPINFOHEADER
{
   long     biSize;
   long     biWidth;
   long     biHeight;
   short    biPlanes;
   short    biBitCount;
   long     biCompression;
   long     biSizeImage;
   long     biXPelsPerMeter;
   long     biYPelsPerMeter;
   long     biClrUsed;
   long     biClrImportant;
} BMPINFOHEADER;

typedef struct	tagIMAGEDATA
{
	unsigned char *	pData;
	long			Width;
	long			Height;
	long			Padding;
	short			Channels;
} IMAGEDATA;

#pragma pack(pop, ident)

int		BitmapLoader(const char *FilePath, IMAGEDATA *ImageData, int pChannels);
void    SwapEndian(short *Data);
void    SwapEndian(long *Data);
void    SwapRedBlue(IMAGEDATA *ImageData);

#endif


