#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>

class ThreadPool {
public:
	ThreadPool(size_t threads);
	~ThreadPool();
	
	template<class F>
	void enqueue(F&& f);
private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;

	std::mutex queue_mutex;
	std::condition_variable cv;
	bool stop;

	void workerThread();
};


template <class F>
void ThreadPool::enqueue(F&& f)
{
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		tasks.emplace(std::forward<F>(f));
		std::cout << "Task enqueued. Total tasks in queue: " << tasks.size() << std::endl;
	}
	cv.notify_all();
}

#endif

