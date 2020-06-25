#pragma once
#include "ITask.h"
#include "ThreadProc.h"
#include <vector>

class Pool
{
public:
	Pool(std::vector<ITask*> tasks, unsigned threadsCount)
	{
		m_tasksCount = tasks.size();
		m_handles = new HANDLE[m_tasksCount];
		for (size_t i = 0; i < m_tasksCount; i++)
		{
			m_handles[i] = CreateThread(NULL, 0, &ThreadProc, tasks[i], CREATE_SUSPENDED, NULL);
		}
		m_threadsCount = threadsCount;
	}

	void ExecuteTasks()
	{
		unsigned count = 0;
		for (size_t i = 0; i < m_tasksCount; i++)
		{
			ResumeThread(m_handles[i]);
			count++;
			if (count == m_threadsCount)
			{
				WaitForMultipleObjects(i + 1, m_handles, true, INFINITE);
				count = 0;
			}
		}

		WaitForMultipleObjects(m_tasksCount, m_handles, true, INFINITE);
	}

private:
	HANDLE* m_handles;
	unsigned m_tasksCount;
	unsigned m_threadsCount;
};
