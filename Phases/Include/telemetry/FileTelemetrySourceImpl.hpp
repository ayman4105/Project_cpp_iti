#pragma once

#include "telemetry/ITelemetrySource.hpp"
#include "safe/SafeFile.hpp"
#include"fstream"

class FileTelemetrySrc : public ITelemetrySource
{
    using string = std::string;

private:
    std::string path;
    std::optional<SafeFile> file;
    std::ifstream fileStream;

public:
    FileTelemetrySrc(string path);
    bool openSource() override;
    bool readSource(string &out) override;
    ~FileTelemetrySrc() = default;
};