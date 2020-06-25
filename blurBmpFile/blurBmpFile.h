#pragma once
#include <fstream>
#include <string>

int const AREA = 50;
const int TITLEMINLENGTH = 4;




struct _CIEXYZ {
	int ciexyzX;
	int ciexyzY;
	int ciexyzZ;
};

struct _CIEXYZTRIPLE {
	_CIEXYZ  ciexyzRed;
	_CIEXYZ  ciexyzGreen;
	_CIEXYZ  ciexyzBlue;
};

// bitmap file header
struct _BITMAPFILEHEADER {
	unsigned short bfType;
	unsigned int   bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned int   bfOffBits;
};

// bitmap info header
struct _BITMAPINFOHEADER {
	unsigned int   biSize;
	unsigned int   biWidth;
	unsigned int   biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned int   biCompression;
	unsigned int   biSizeImage;
	unsigned int   biXPelsPerMeter;
	unsigned int   biYPelsPerMeter;
	unsigned int   biClrUsed;
	unsigned int   biClrImportant;
	unsigned int   biRedMask;
	unsigned int   biGreenMask;
	unsigned int   biBlueMask;
	unsigned int   biAlphaMask;
	unsigned int   biCSType;
	_CIEXYZTRIPLE   biEndpoints;
	unsigned int   biGammaRed;
	unsigned int   biGammaGreen;
	unsigned int   biGammaBlue;
	unsigned int   biIntent;
	unsigned int   biProfileData;
	unsigned int   biProfileSize;
	unsigned int   biReserved;
};

// rgb quad
struct _RGBQUAD {
	unsigned char  rgbBlue;
	unsigned char  rgbGreen;
	unsigned char  rgbRed;
	unsigned char  rgbReserved;
};

// read bytes
template <typename Type>
void read(std::ifstream &fp, Type &result, std::size_t size) {
	fp.read(reinterpret_cast<char*>(&result), size);
}

template <typename Type>
void write(std::ostream& strm, const Type& source, size_t size)
{
	strm.write(reinterpret_cast<const char*>(&source), size);
}

// bit extract
unsigned char bitextract(const unsigned int byte, const unsigned int mask);
