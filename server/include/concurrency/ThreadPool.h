#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <concurrency/ThreadSafeQueue.h>
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>

class ThreadPool
{
private:
    ThreadSafeQueue<std::function<void()>> m_queue;
    std::vector<std::jthread> workers{};

public:

    explicit ThreadPool(size_t threadCount = 5);
    ~ThreadPool();

    void run();
};

#endif
