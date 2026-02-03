#include <iostream>
#include "Include/sinks/ConsoleSinkImpl.hpp"
#include "Include/sinks/FileSinkImpl.hpp"
#include "Include/sinks/ILogSink.hpp"
#include "Include/LogManager.hpp"
#include "Include/LogMessage.hpp"
#include "Include/telemetry/FileTelemetrySourceImpl.hpp"
#include "Include/telemetry/SocketTelemetrySourceImpl.hpp"
#include "Include/Formatter.hpp"
#include"ThreadPool.hpp"

std::string getTelemetryValue()
{
    static int counter = 0;
    counter = counter + 8;
    return std::to_string(50 + counter);
}

int main()
{

    LogManager logger(2,10); // threads=2, 10 capacity

    logger.add_sink(std::make_unique<ConsoleSinkImpl>());

    logger.add_sink(std::make_unique<FileSinkImpl>("logs.txt"));

    for (int i = 0; i < 5; i++)
    {
        std::string raw_value = getTelemetryValue();

        auto log_opt = Formatter<CPU_policy>::format(raw_value);

        if (log_opt.has_value())
        {
            logger << LogMessage(log_opt.value());
        }
        else
        {
            std::cout << "Failed to parse telemetry data." << std::endl;
        }
    }

    logger.write();

    std::cout << "Telemetry logs sent to console and file.\n";

    return 0;
}
