// DAY 1 - thread safe queue, atomics, conditional variables, locks

#ifndef THREADSAFEQUEUE
#define THREADSAFEQUEUE

#include <utility>
#include <queue>
#include <condition_variable>
#include <mutex>

template<typename T>
class ThreadSafeQueue
{
private:
    std::queue<T> m_queue{};
    std::mutex m_mutex{};
    std::condition_variable m_cv{};

public:
    void push(T value)
    {
        {
            std::lock_guard lock{m_mutex};
            m_queue.push(std::move(value));
        }
        m_cv.notify_one();
    }

    T wait_and_pop()
    {
        std::unique_lock lock{m_mutex};

        m_cv.wait(lock, [&] {
            return !m_queue.empty();
        });

        T value{std::move(m_queue.front())};
        m_queue.pop();
        return value;
    }
};


#endif
