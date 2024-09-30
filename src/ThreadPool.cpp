#include "ThreadPool.hpp"
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back(&ThreadPool::workerThread, this);  // Launch a worker thread
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stop = true;
    }
    cv.notify_all();
    for (std::thread &worker : workers) {
        worker.join();
    }
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop && tasks.empty()) return;  // If we're stopping and no tasks remain, exit

            task = std::move(tasks.front());
            tasks.pop();
            std::cout << "Task started by thread: " << std::this_thread::get_id() << std::endl;
        }

        task();
        std::cout << "Task completed by thread: " << std::this_thread::get_id() << std::endl;
    }
}



