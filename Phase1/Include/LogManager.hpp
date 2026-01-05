#pragma once

#include <vector>
#include <memory>
#include "../Include/LogMessage.hpp"
#include "../Include/ILogSink.hpp"


class LogManager
{
private:
    std::vector<std::unique_ptr<ILogSink>> sinks;
    std::vector<LogMessage> messages;
public:
    void add_sink(std::unique_ptr<ILogSink> sink);
    void log(const LogMessage& message);
    LogManager& operator<<(const LogMessage& message);
    LogManager() = default;
    ~LogManager() = default;
};

