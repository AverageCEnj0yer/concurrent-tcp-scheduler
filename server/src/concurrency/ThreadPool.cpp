#include <concurrency/ThreadPool.h>

#include <iostream>

ThreadPool::ThreadPool(size_t threadCount) :
    workers{std::vector<std::jthread>(threadCount)}
{
    run();
}

ThreadPool::~ThreadPool()
{
    m_queue.close(); // Wakes up wait_and_pop(), which returns std::nullopt
}

void ThreadPool::run()
{
    for(size_t i{}; i < workers.size(); ++i)
    {
        workers[i] = std::jthread([this] {
        while (true) {
            auto task = m_queue.wait_and_pop();
            if (!task) {
                break; // Queue is closed and empty — exit thread so ~jthread() can join
            }
            (*task)();
            std::cout << "Executed by: " << std::this_thread::get_id() << "\n";
        }
    });
    }
    
}
