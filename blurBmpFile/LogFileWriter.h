#pragma once

#include "LogBuffer.h"
#include <fstream>
#include <string>

template<typename T>
class LogFileWriter
{
public:
	LogFileWriter()
		: m_outFile("buffOut.txt")
	{
	}

	void write(const LogBuffer<T>& logBuffer)
	{
		for (T data : logBuffer)
		{
			m_outFile << data << std::endl;
		}
	}

private:
	std::ofstream m_outFile;
};
