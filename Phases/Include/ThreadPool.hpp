#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool
{

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop_flag;

    void worker_loop()
    {
        while (true)
        {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                condition.wait(lock, [this]()
                               { return stop_flag || !tasks.empty(); });

                if (stop_flag && tasks.empty())
                    return;

                task = std::move(tasks.front());
                tasks.pop();
            }

            task();
        }
    }

public:
    explicit ThreadPool(size_t thread_count)
        : stop_flag(false)
    {
        for (size_t i = 0; i < thread_count; ++i)
        {
            workers.emplace_back([this]()
                                 { worker_loop(); });
        }
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop_flag = true;
        }

        condition.notify_all();

        for (auto &t : workers)
        {
            if (t.joinable())
                t.join();
        }
    }

    void push_task(std::function<void()> task)
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            tasks.push(std::move(task));
        }
        condition.notify_one();
    }
};
