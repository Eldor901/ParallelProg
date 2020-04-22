#pragma once
#include <windows.h>
#include <string>
#include <iostream>

using namespace std;

class ITask
{
public:

	virtual void Execute() = 0;
};


class Task : public ITask {

public:
	Task(int num)
	{
		m_num = num;
	}

	void Execute()
	{
		cout << "Thread Number " << m_num << " Does it's job" << endl;
		ExitThread(0);
	}

private:
	int m_num;

};
