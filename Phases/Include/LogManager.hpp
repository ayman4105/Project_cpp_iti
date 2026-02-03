#pragma once

#include <vector>
#include <memory>
#include "LogMessage.hpp"
#include "sinks/ILogSink.hpp"
#include "RingBuffer.hpp"
#include "ThreadPool.hpp"

class LogManager
{
private:
    std::vector<std::unique_ptr<ILogSink>> sinks;
    RingBuffer<LogMessage> messages;
    std::unique_ptr<ThreadPool> pool;

public:
    LogManager(size_t thread_count, size_t capacity)
        : pool(std::make_unique<ThreadPool>(thread_count)), messages(capacity) {}
    void add_sink(std::unique_ptr<ILogSink> sink);
    void log(const LogMessage &message);
    void write();
    LogManager &operator<<(const LogMessage &message);
    ~LogManager() = default;
};
