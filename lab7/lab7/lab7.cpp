

#include "pch.h"
#include <windows.h>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include "ITask.h"
#include "IWorker.h"
#include <vector>

using namespace std;




int main(int argc, char *argv[])
{
	int num = stoi(argv[1]);
	Worker* worker = new Worker[num];

	for (int i = 0; i < num; ++i)
	{
		ITask* task = new Task(i + 1);
		worker[i].ExecuteTask(*task);
	}

	return 0;
}
