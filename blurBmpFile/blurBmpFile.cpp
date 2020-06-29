#include "pch.h"
#include <iostream>

#include <exception>
#include <fstream>
#include <thread>
#include <string>
#include "blurBmpFile.h"
#include <windows.h>
#include <time.h>
#include "LogBuffer.h"
#include "LogFileWriter.h"
using namespace  std;
CRITICAL_SECTION CRITICALSECTION;
LogBuffer<float> LOGBUFFER;
LogFileWriter<float> FILEWRITER;


int const AREA = 50;
int const BUFFERSIZE = 10000;


struct ImgInfo
{
	unsigned width;
	unsigned height;
	_RGBQUAD** initialRgbInfo;
	_RGBQUAD** blurredRgbInfo;

};

struct ThreadData
{
	unsigned startingIndex;
	unsigned height;
	unsigned threadNumber;
	ImgInfo imgInfo;
};

struct ThreadWrite
{
	int WorkingVariable = 0;
};




_RGBQUAD** readBmpFile(const string & fileName, _BITMAPFILEHEADER& fileHeader, _BITMAPINFOHEADER& fileInfoHeader)
{
	ifstream fileStream(fileName, ifstream::binary);
	if (!fileStream)
	{
		throw runtime_error("Usage: " + fileName + " file_name");
	}

	// заголовк изображения
	read(fileStream, fileHeader.bfType, sizeof(fileHeader.bfType));
	read(fileStream, fileHeader.bfSize, sizeof(fileHeader.bfSize));
	read(fileStream, fileHeader.bfReserved1, sizeof(fileHeader.bfReserved1));
	read(fileStream, fileHeader.bfReserved2, sizeof(fileHeader.bfReserved2));
	read(fileStream, fileHeader.bfOffBits, sizeof(fileHeader.bfOffBits));

	if (fileHeader.bfType != 0x4D42) {
		throw runtime_error("Error: '" + fileName + "' is not BMP file.");
		return 0;
	}

	// информация изображения
	read(fileStream, fileInfoHeader.biSize, sizeof(fileInfoHeader.biSize));



	// bmp core
	if (fileInfoHeader.biSize >= 12) {
		read(fileStream, fileInfoHeader.biWidth, sizeof(fileInfoHeader.biWidth));
		read(fileStream, fileInfoHeader.biHeight, sizeof(fileInfoHeader.biHeight));
		read(fileStream, fileInfoHeader.biPlanes, sizeof(fileInfoHeader.biPlanes));
		read(fileStream, fileInfoHeader.biBitCount, sizeof(fileInfoHeader.biBitCount));
	}
	// получаем информацию о битности
	int colorsCount = fileInfoHeader.biBitCount >> 3;
	if (colorsCount < 3)
	{
		colorsCount = 3;
	}

	int bitsOnColor = fileInfoHeader.biBitCount / colorsCount;
	int maskValue = (1 << bitsOnColor) - 1;

	// bmp v1
	if (fileInfoHeader.biSize >= 40)
	{
		read(fileStream, fileInfoHeader.biCompression, sizeof(fileInfoHeader.biCompression));
		read(fileStream, fileInfoHeader.biSizeImage, sizeof(fileInfoHeader.biSizeImage));
		read(fileStream, fileInfoHeader.biXPelsPerMeter, sizeof(fileInfoHeader.biXPelsPerMeter));
		read(fileStream, fileInfoHeader.biYPelsPerMeter, sizeof(fileInfoHeader.biYPelsPerMeter));
		read(fileStream, fileInfoHeader.biClrUsed, sizeof(fileInfoHeader.biClrUsed));
		read(fileStream, fileInfoHeader.biClrImportant, sizeof(fileInfoHeader.biClrImportant));
	}

	// bmp v2
	fileInfoHeader.biRedMask = 0;
	fileInfoHeader.biGreenMask = 0;
	fileInfoHeader.biBlueMask = 0;

	if (fileInfoHeader.biSize >= 52)
	{
		read(fileStream, fileInfoHeader.biRedMask, sizeof(fileInfoHeader.biRedMask));
		read(fileStream, fileInfoHeader.biGreenMask, sizeof(fileInfoHeader.biGreenMask));
		read(fileStream, fileInfoHeader.biBlueMask, sizeof(fileInfoHeader.biBlueMask));
	}

	// если маска не задана, то ставим маску по умолчанию
	if (fileInfoHeader.biRedMask == 0 || fileInfoHeader.biGreenMask == 0 || fileInfoHeader.biBlueMask == 0)
	{
		fileInfoHeader.biRedMask = maskValue << (bitsOnColor * 2);
		fileInfoHeader.biGreenMask = maskValue << bitsOnColor;
		fileInfoHeader.biBlueMask = maskValue;
	}

	// bmp v3
	if (fileInfoHeader.biSize >= 56)
	{
		read(fileStream, fileInfoHeader.biAlphaMask, sizeof(fileInfoHeader.biAlphaMask));
	}
	else
	{
		fileInfoHeader.biAlphaMask = maskValue << (bitsOnColor * 3);
	}

	// bmp v4
	if (fileInfoHeader.biSize >= 108)
	{
		read(fileStream, fileInfoHeader.biCSType, sizeof(fileInfoHeader.biCSType));
		read(fileStream, fileInfoHeader.biEndpoints, sizeof(fileInfoHeader.biEndpoints));
		read(fileStream, fileInfoHeader.biGammaRed, sizeof(fileInfoHeader.biGammaRed));
		read(fileStream, fileInfoHeader.biGammaGreen, sizeof(fileInfoHeader.biGammaGreen));
		read(fileStream, fileInfoHeader.biGammaBlue, sizeof(fileInfoHeader.biGammaBlue));
	}

	// bmp v5
	if (fileInfoHeader.biSize >= 124)
	{
		read(fileStream, fileInfoHeader.biIntent, sizeof(fileInfoHeader.biIntent));
		read(fileStream, fileInfoHeader.biProfileData, sizeof(fileInfoHeader.biProfileData));
		read(fileStream, fileInfoHeader.biProfileSize, sizeof(fileInfoHeader.biProfileSize));
		read(fileStream, fileInfoHeader.biReserved, sizeof(fileInfoHeader.biReserved));
	}

	// проверка на поддерку этой версии формата
	if (fileInfoHeader.biSize != 12 && fileInfoHeader.biSize != 40 && fileInfoHeader.biSize != 52 &&
		fileInfoHeader.biSize != 56 && fileInfoHeader.biSize != 108 && fileInfoHeader.biSize != 124)
	{
		throw runtime_error("Error: Unsupported BMP format.");
	}

	if (fileInfoHeader.biBitCount != 16 && fileInfoHeader.biBitCount != 24 && fileInfoHeader.biBitCount != 32)
	{
		throw runtime_error("Error: Unsupported BMP format.");

	}

	if (fileInfoHeader.biCompression != 0 && fileInfoHeader.biCompression != 3)
	{
		throw runtime_error("Error: Unsupported BMP format.");

	}

	// rgb info
	_RGBQUAD** rgbInfo = new _RGBQUAD*[fileInfoHeader.biHeight];

	for (unsigned i = 0; i < fileInfoHeader.biHeight; i++)
	{
		rgbInfo[i] = new _RGBQUAD[fileInfoHeader.biWidth];
	}

	// определение размера отступа в конце каждой строки
	int linePadding = ((fileInfoHeader.biWidth * (fileInfoHeader.biBitCount / 8)) % 4) & 3;

	// чтение
	unsigned bufer;

	for (unsigned i = 0; i < fileInfoHeader.biHeight; ++i)
	{
		for (unsigned j = 0; j < fileInfoHeader.biWidth; ++j)
		{
			read(fileStream, bufer, fileInfoHeader.biBitCount / 8);

			rgbInfo[i][j].rgbRed = bitextract(bufer, fileInfoHeader.biRedMask);
			rgbInfo[i][j].rgbGreen = bitextract(bufer, fileInfoHeader.biGreenMask);
			rgbInfo[i][j].rgbBlue = bitextract(bufer, fileInfoHeader.biBlueMask);
			rgbInfo[i][j].rgbReserved = bitextract(bufer, fileInfoHeader.biAlphaMask);
		}
		fileStream.seekg(linePadding, ios_base::cur);
	}

	return rgbInfo;
}


void writeBmpFile(const string& fileName, _RGBQUAD** rgbInfo, _BITMAPFILEHEADER& fileHeader, _BITMAPINFOHEADER& fileInfoHeader)
{
	ofstream fileStream(fileName, ifstream::binary);


	write(fileStream, fileHeader.bfType, sizeof(fileHeader.bfType));
	write(fileStream, fileHeader.bfSize, sizeof(fileHeader.bfSize));
	write(fileStream, fileHeader.bfReserved1, sizeof(fileHeader.bfReserved1));
	write(fileStream, fileHeader.bfReserved2, sizeof(fileHeader.bfReserved2));
	write(fileStream, fileHeader.bfOffBits, sizeof(fileHeader.bfOffBits));

	write(fileStream, fileInfoHeader.biSize, sizeof(fileInfoHeader.biSize));

	write(fileStream, fileInfoHeader.biWidth, sizeof(fileInfoHeader.biWidth));
	write(fileStream, fileInfoHeader.biHeight, sizeof(fileInfoHeader.biHeight));
	write(fileStream, fileInfoHeader.biPlanes, sizeof(fileInfoHeader.biPlanes));
	write(fileStream, fileInfoHeader.biBitCount, sizeof(fileInfoHeader.biBitCount));

	write(fileStream, fileInfoHeader.biCompression, sizeof(fileInfoHeader.biCompression));
	write(fileStream, fileInfoHeader.biSizeImage, sizeof(fileInfoHeader.biSizeImage));
	write(fileStream, fileInfoHeader.biXPelsPerMeter, sizeof(fileInfoHeader.biXPelsPerMeter));
	write(fileStream, fileInfoHeader.biYPelsPerMeter, sizeof(fileInfoHeader.biYPelsPerMeter));
	write(fileStream, fileInfoHeader.biClrUsed, sizeof(fileInfoHeader.biClrUsed));
	write(fileStream, fileInfoHeader.biClrImportant, sizeof(fileInfoHeader.biClrImportant));

	write(fileStream, fileInfoHeader.biRedMask, sizeof(fileInfoHeader.biRedMask));
	write(fileStream, fileInfoHeader.biGreenMask, sizeof(fileInfoHeader.biGreenMask));
	write(fileStream, fileInfoHeader.biBlueMask, sizeof(fileInfoHeader.biBlueMask));

	write(fileStream, fileInfoHeader.biAlphaMask, sizeof(fileInfoHeader.biAlphaMask));

	write(fileStream, fileInfoHeader.biCSType, sizeof(fileInfoHeader.biCSType));
	write(fileStream, fileInfoHeader.biEndpoints, sizeof(fileInfoHeader.biEndpoints));
	write(fileStream, fileInfoHeader.biGammaRed, sizeof(fileInfoHeader.biGammaRed));
	write(fileStream, fileInfoHeader.biGammaGreen, sizeof(fileInfoHeader.biGammaGreen));
	write(fileStream, fileInfoHeader.biGammaBlue, sizeof(fileInfoHeader.biGammaBlue));

	write(fileStream, fileInfoHeader.biIntent, sizeof(fileInfoHeader.biIntent));
	write(fileStream, fileInfoHeader.biProfileData, sizeof(fileInfoHeader.biProfileData));
	write(fileStream, fileInfoHeader.biProfileSize, sizeof(fileInfoHeader.biProfileSize));
	write(fileStream, fileInfoHeader.biReserved, sizeof(fileInfoHeader.biReserved));

	for (unsigned i = 0; i < fileInfoHeader.biHeight; ++i)
	{
		for (unsigned j = 0; j < fileInfoHeader.biWidth; ++j)
		{
			write(fileStream, rgbInfo[i][j].rgbBlue, sizeof(rgbInfo[i][j].rgbBlue));
			write(fileStream, rgbInfo[i][j].rgbGreen, sizeof(rgbInfo[i][j].rgbGreen));
			write(fileStream, rgbInfo[i][j].rgbRed, sizeof(rgbInfo[i][j].rgbRed));
			write(fileStream, rgbInfo[i][j].rgbReserved, sizeof(rgbInfo[i][j].rgbReserved));
		}
	}

}

unsigned char bitextract(const unsigned int byte, const unsigned int mask) {
	if (mask == 0) {
		return 0;
	}

	int
		maskBufer = mask,
		maskPadding = 0;

	while (!(maskBufer & 1)) {
		maskBufer >>= 1;
		maskPadding++;
	}

	// применение маски и смещение
	return (byte & mask) >> maskPadding;
}


DWORD WINAPI logSizeMonitoringThread(LPVOID lpParam)
{
	LogBuffer<float>* tmpBuffer = (LogBuffer<float>*)lpParam;
	FILEWRITER.write(*tmpBuffer);
	delete tmpBuffer;
	ExitThread(0);
}



DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
	ThreadData* threadData = (ThreadData*)lpParam;
	ofstream fileOut(to_string(threadData->threadNumber) + ".txt");

	float prevTime = 0;
	for (unsigned i = threadData->startingIndex; i < threadData->startingIndex + threadData->height; ++i)
	{
		for (unsigned j = AREA; j < threadData->imgInfo.width - AREA; ++j)
		{
			int red = 0;
			int blue = 0;
			int green = 0;

			for (unsigned k = 0; k < AREA; ++k)
			{
				red += (threadData->imgInfo.initialRgbInfo[i][j].rgbRed + threadData->imgInfo.initialRgbInfo[i + k][j].rgbRed
					+ threadData->imgInfo.initialRgbInfo[i - k][j].rgbRed + threadData->imgInfo.initialRgbInfo[i][j + k].rgbRed
					+ threadData->imgInfo.initialRgbInfo[i][j - k].rgbRed);
				blue += (threadData->imgInfo.initialRgbInfo[i][j].rgbBlue + threadData->imgInfo.initialRgbInfo[i + k][j].rgbBlue
					+ threadData->imgInfo.initialRgbInfo[i - k][j].rgbBlue
					+ threadData->imgInfo.initialRgbInfo[i][j + k].rgbBlue + threadData->imgInfo.initialRgbInfo[i][j - k].rgbBlue);
				green += (threadData->imgInfo.initialRgbInfo[i][j].rgbBlue + threadData->imgInfo.initialRgbInfo[i + k][j].rgbGreen
					+ threadData->imgInfo.initialRgbInfo[i - k][j].rgbGreen + threadData->imgInfo.initialRgbInfo[i][j + k].rgbGreen
					+ threadData->imgInfo.initialRgbInfo[i][j - k].rgbGreen);
			}

			threadData->imgInfo.blurredRgbInfo[i][j].rgbRed = red / (AREA * 5);
			threadData->imgInfo.blurredRgbInfo[i][j].rgbBlue = blue / (AREA * 5);
			threadData->imgInfo.blurredRgbInfo[i][j].rgbGreen = green / (AREA * 5);


			float curtime = ((float)clock()) / CLOCKS_PER_SEC;

			fileOut << curtime << endl;
			LOGBUFFER.Log(curtime);
			prevTime = curtime;


			EnterCriticalSection(&CRITICALSECTION);
			if (LOGBUFFER.GetSize() >= BUFFERSIZE)
			{
				LogBuffer<float>* tmpBuffer = new LogBuffer<float>(move(LOGBUFFER));
				HANDLE handle = CreateThread(NULL, 0, &logSizeMonitoringThread, tmpBuffer, CREATE_SUSPENDED, NULL);
				ResumeThread(handle);
			}
			LeaveCriticalSection(&CRITICALSECTION);
		}
	}



	ExitThread(0); // функция устанавливает код завершения потока в 0
}




_RGBQUAD** blurFile(_RGBQUAD** rgbInfo, _BITMAPINFOHEADER& fileInfoHeader, const int unsigned threadsCount, const int unsigned processorsCount, int* threadsPriorities)
{

	if (!InitializeCriticalSectionAndSpinCount(&CRITICALSECTION, 0x00000400))
	{
		throw runtime_error("Failed to initialize critical section");
	}

	HANDLE* handles = new HANDLE[threadsCount];
	ThreadData* threadsData = new ThreadData[threadsCount];

	unsigned threadHeight = (fileInfoHeader.biHeight - 2 * AREA) / threadsCount;
	unsigned startingIndex = AREA;

	ThreadWrite threadWrite;
	threadWrite.WorkingVariable = 0;


	_RGBQUAD** rgbblur = new _RGBQUAD*[fileInfoHeader.biHeight];


	for (unsigned i = 0; i < fileInfoHeader.biHeight; i++)
	{
		rgbblur[i] = new _RGBQUAD[fileInfoHeader.biWidth];
	}

	LOGBUFFER.AddCriticalSection(&CRITICALSECTION);

	unsigned affinityMask = (1 << processorsCount) - 1;

	for (unsigned i = 0; i < threadsCount; ++i)
	{

		threadsData[i].imgInfo.width = fileInfoHeader.biWidth;
		threadsData[i].imgInfo.height = fileInfoHeader.biHeight;
		threadsData[i].imgInfo.initialRgbInfo = rgbInfo;
		threadsData[i].imgInfo.blurredRgbInfo = rgbblur;

		threadsData[i].startingIndex = startingIndex;
		threadsData[i].height = threadHeight;
		threadsData[i].threadNumber = i + 1;
		handles[i] = CreateThread(NULL, 0, &ThreadProc, &(threadsData[i]), CREATE_SUSPENDED, NULL);

		SetThreadAffinityMask(handles[i], affinityMask);

		SetThreadPriority(handles[i], threadsPriorities[i]);


		startingIndex += threadHeight;
	}

	for (int i = 0; i < threadsCount; i++)
	{
		ResumeThread(handles[i]);
	}

	WaitForMultipleObjects(threadsCount, handles, true, INFINITE);

	DeleteCriticalSection(&CRITICALSECTION);


	LogBuffer<float>* tmpBuffer = new LogBuffer<float>(move(LOGBUFFER));
	HANDLE handle = CreateThread(NULL, 0, &logSizeMonitoringThread, tmpBuffer, CREATE_SUSPENDED, NULL);
	ResumeThread(handle);


	return rgbblur;

}

int* getPrioritiesforThreads(int threadsCount, char* argv[])
{
	int* threadsPriorities = new int[threadsCount];

	for (unsigned i = 0; i < threadsCount; ++i)
	{
		if (string(argv[5 + i]) == "above_normal")
		{
			threadsPriorities[i] = THREAD_PRIORITY_ABOVE_NORMAL;
		}
		else if (string(argv[5 + i]) == "normal")
		{
			threadsPriorities[i] = THREAD_PRIORITY_NORMAL;
		}
		else if (string(argv[5 + i]) == "below_normal")
		{
			threadsPriorities[i] = THREAD_PRIORITY_BELOW_NORMAL;
		}
		else
		{
			throw runtime_error("You must specify a priority  below_normal, normal, above_normal");
		}
	}

	return threadsPriorities;
}



int main(int argc, char *argv[])
{
	if (argc < 5)
	{
		cout << "Usage: " << " program.exe input_file_name output_file_name threads processors" << endl;
		return -1;
	}


	_BITMAPFILEHEADER fileHeader;
	_BITMAPINFOHEADER fileInfoHeader;
	int* threadsPriorities;

	try
	{
		_RGBQUAD** rgbInfo = readBmpFile(argv[1], fileHeader, fileInfoHeader);
		const unsigned int threadsCount = stoi(argv[3]);
		const unsigned int processorsCount = stoi(argv[4]);

		if (argc < 5 + threadsCount)
		{
			cout << "You must specify priority for each thread" << endl;
			return -1;
		}

		threadsPriorities = getPrioritiesforThreads(threadsCount, argv);


		_RGBQUAD** blurredRgbInfo = blurFile(rgbInfo, fileInfoHeader, threadsCount, processorsCount, threadsPriorities);

		writeBmpFile(argv[2], blurredRgbInfo, fileHeader, fileInfoHeader);

	}
	catch (exception & e)
	{
		cout << e.what();
	}

	cout << ((float)clock()) / CLOCKS_PER_SEC << endl;
}
