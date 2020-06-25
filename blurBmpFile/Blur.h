#pragma once
#include "ITask.h"
#include "blurBmpFile.h"

using namespace std;


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



class Blur : public ITask
{
public:
	Blur(CONST LPVOID lpParam)
	{
		threadData = (ThreadData*)lpParam;
	}

	void Execute() override
	{

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
			}
		}
	}

private:
	ThreadData* threadData;
};
