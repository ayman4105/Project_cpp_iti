#pragma once

#include "telemetry/ITelemetrySource.hpp"
#include "safe/SafeFile.hpp"

class FileTelemetrySrc : public ITelemetrySource
{
    using string = std::string;

private:
    std::string path;
    std::optional<SafeFile> file;

public:
    FileTelemetrySrc(string path);
    bool openSource() override;
    bool readSource(string &out) override;
    ~FileTelemetrySrc() = default;
};