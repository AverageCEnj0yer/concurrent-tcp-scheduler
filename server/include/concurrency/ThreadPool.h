#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <concurrency/ThreadSafeQueue.h>
#include <cstddef>
#include <future>
#include <functional>
#include <memory>
#include <type_traits>
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

    //the insane abstraction that makes this so so cool
    auto submitTask(auto&& func)
    {
        using ReturnType = std::invoke_result_t<decltype(func)>;
        auto task{ std::make_shared<std::packaged_task<ReturnType()>>(
            std::forward<decltype(func)>(func)
        )};
        std::future<ReturnType> result{(*task).get_future()};
        m_queue.push([task]{
            (*task)();
        });
        return result;
    }

    void run();
};

#endif
