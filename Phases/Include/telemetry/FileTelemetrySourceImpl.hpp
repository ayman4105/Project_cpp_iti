#pragma once

#include "../../Include/telemetry/ITelemetrySource.hpp"
#include "../../Include/safe/SafeFile.hpp"

class FileTelemetrySrc: public ITelemetrySource
{
    using string = std::string;
private:
    std::string path;
    std::optional<SafeFile> file;
    
public:
    FileTelemetrySrc(string path);
    bool openSource() override;
    bool readSource(string& out) override;
    ~FileTelemetrySrc() = default;
};