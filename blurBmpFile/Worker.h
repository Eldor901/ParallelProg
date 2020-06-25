#pragma once

#include <vector>
#include "IWorker.h"
#include "ThreadProc.h"

class Worker : public IWorker
{
public:
	~Worker()
	{
		WaitForSingleObject(m_handle, INFINITE);
	}

	bool ExecuteTask(ITask* task) override
	{
		if (IsBusy())
		{
			return false;
		}

		m_handle = CreateThread(NULL, 0, &ThreadProc, task, CREATE_SUSPENDED, NULL);
		ResumeThread(m_handle);

		return true;
	}
	bool IsBusy() const override
	{
		if (m_handle == nullptr)
		{
			return false;
		}

		LPDWORD code = new DWORD;
		GetExitCodeThread(m_handle, code);

		if (*code == STILL_ACTIVE)
		{
			return true;
		}

		return false;
	}
	void Wait() const override
	{
		WaitForSingleObject(m_handle, INFINITE);
	}

private:
	HANDLE m_handle;
};
