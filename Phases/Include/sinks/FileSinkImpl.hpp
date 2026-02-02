#pragma once

#include "ILogSink.hpp"
#include "fstream"

class FileSinkImpl : public ILogSink
{
private:
    std::ofstream file;

public:
    void write(const LogMessage &message) override;
    FileSinkImpl(const std::string &filename);
    virtual ~FileSinkImpl() = default;
};