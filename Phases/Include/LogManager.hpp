#pragma once

#include <vector>
#include <memory>
#include "LogMessage.hpp"
#include "sinks/ILogSink.hpp"
#include "RingBuffer.hpp"

class LogManager
{
private:
    std::vector<std::unique_ptr<ILogSink>> sinks;
    RingBuffer<LogMessage> messages;

public:
    LogManager(size_t capacity) : messages{capacity} {}
    void add_sink(std::unique_ptr<ILogSink> sink);
    void log(const LogMessage &message);
    void write();
    LogManager &operator<<(const LogMessage &message);
    ~LogManager() = default;
};
