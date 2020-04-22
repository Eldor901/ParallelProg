#pragma once

#include "ITask.h"



DWORD WINAPI TreadProc(CONST LPVOID lpParam)
{
	ITask *task = (ITask*)lpParam;

	task->Execute();

	ExitThread(0);
}

class IWorker
{
public:
	virtual ~IWorker() = default;

	virtual bool ExecuteTask(ITask & task) = 0;
	virtual bool IsBusy() = 0;
};


class Worker : public IWorker {
public:

	bool ExecuteTask(ITask & task) override {

		if (IsBusy())
		{
			return false;
		}

		m_handle = CreateThread(NULL, 0, &TreadProc, &(task), CREATE_SUSPENDED, NULL);
		ResumeThread(m_handle);

		return true;

	}

	bool IsBusy() override {

		LPDWORD code = new DWORD;
		GetExitCodeThread(m_handle, code);

		if (*code == STILL_ACTIVE)
		{
			return true;
		}


		return false;
	}

	~Worker()
	{
		WaitForSingleObject(m_handle, INFINITE);
	}
private:
	HANDLE m_handle;

};
