#pragma once
#include "LogMessage.hpp"

class ILogSink
{
private:
public:
    virtual void write(const LogMessage &message) = 0;
    ILogSink() = default;
    virtual ~ILogSink() = default;
};
