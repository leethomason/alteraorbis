#ifndef GRINLIZ_THREADPOOL
#define GRINLIZ_THREADPOOL

#include <thread>
#include <mutex>
#include <condition_variable>
#include "glcontainer.h"

namespace grinliz {

class ThreadPool
{
public:
	typedef int(*FUNC_PTR)(void*, void*, void*, void*);

	ThreadPool() {
		for (int i = 0; i < NTHREAD; ++i) {
			threads[i] = std::thread([this]{
				for (;;) {
					Task task;
					{
						std::unique_lock<std::mutex> lock(this->taskMutex);
						this->taskCondition.wait(lock, [this] {
							return this->stop || !this->tasks.Empty();
						});
						if (this->stop && this->tasks.Empty())
							return;
						task = tasks.Pop();
					}
					int r = task.func(task.data1, task.data2, task.data3, task.data4);
					bool notifyTodo = false;
					{
						std::unique_lock<std::mutex> lock(this->todoMutex);
						results.Push(r);
						--todo;
						notifyTodo = (todo == 0);
					}
					// Notify that a task is done.
					taskCondition.notify_one();
					// If needed, notify our work is done.
					if (notifyTodo) {
						todoCondition.notify_one();
					}
				}
			});
		}
	}

	~ThreadPool() {
		{
			std::unique_lock<std::mutex> lock(this->taskMutex);
			stop = true;
		}
		taskCondition.notify_all();
		for (int i = 0; i < NTHREAD; ++i) {
			threads[i].join();
		}
		todo = 0;	// safe: all the threads are gone.
		todoCondition.notify_all();
	}

	void Add(FUNC_PTR func, void* data1, void* data2=nullptr, void* data3=nullptr, void* data4=nullptr) {
		{
			std::unique_lock<std::mutex> lock(taskMutex);
			Task t(func, data1, data2, data3, data4);
			tasks.Push(t);
		}
		{
			std::unique_lock<std::mutex> lock(todoMutex);
			++todo;
		}
		taskCondition.notify_one();
	}

	void Wait(CDynArray<int>* resultsArray) {
		std::unique_lock<std::mutex> lock(todoMutex);
		todoCondition.wait(lock, [this] {
			return this->todo == 0;
		});
		if (resultsArray) {
			resultsArray->Clear();
			if (!results.Empty()) {
				int* mem = resultsArray->PushArr(results.Size());
				memcpy(mem, results.Mem(), sizeof(int)*results.Size());
			}
		}
	}

private:
	enum {NTHREAD = 4};
	std::thread threads[NTHREAD];
	std::mutex taskMutex;
	std::mutex todoMutex;
	std::condition_variable taskCondition;
	std::condition_variable todoCondition;
	CDynArray<int> results;

	struct Task {
		Task() : func(nullptr), data1(nullptr), data2(nullptr), data3(nullptr), data4(nullptr) {}
		Task(FUNC_PTR _func, void* _data1, void* _data2, void* _data3, void* _data4) 
			: func(_func), data1(_data1), data2(_data2), data3(_data3), data4(_data4) {}

		// This caused all kinds of trouble I didn't expect. And does
		// much more than I want:
		//std::function<int(void*)> func;
		FUNC_PTR func;
		void* data1;
		void* data2;
		void* data3;
		void* data4;
	};
	CDynArray<Task> tasks;

	bool stop = false;
	int todo = 0;
};

};
#endif // GRINLIZ_THREADPOOL
