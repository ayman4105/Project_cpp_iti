#pragma once 

#include"ILogSink.hpp"

class ConsoleSinkImpl : public ILogSink
{
private:
    
public:
    void write(const LogMessage& message) override;
    ConsoleSinkImpl() = default;
    virtual ~ConsoleSinkImpl() = default;
};




