#include "telemetry/FileTelemetrySourceImpl.hpp"

FileTelemetrySrc::FileTelemetrySrc(string path)
    : path(std::move(path))
{
}

bool FileTelemetrySrc::openSource()
{
    file = SafeFile(path);
    return true;
}

bool FileTelemetrySrc::readSource(string &out)
{
    if (!file)
    {
        return false;
    }

    return file.value().readLine(out);
}