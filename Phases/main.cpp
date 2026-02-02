#include <iostream>
#include "Include/sinks/ConsoleSinkImpl.hpp"
#include "Include/sinks/FileSinkImpl.hpp"
#include "Include/sinks/ILogSink.hpp"
#include "Include/LogManager.hpp"
#include "Include/LogMessage.hpp"
// #include"Include/safe/SafeFile.hpp"
// #include"Include/safe/SafeSocket.hpp"
#include "Include/telemetry/FileTelemetrySourceImpl.hpp"
#include "Include/telemetry/SocketTelemetrySourceImpl.hpp"
#include "Include/Formatter.hpp"

// to run socket write in terminal nc -l 12345
// source = std::make_unique<SocketTelemetrySrc>("127.0.0.1", 12345);

// simulated telemetry for testing
std::string getTelemetryValue()
{
    static int counter = 0;
    counter = counter + 5;
    return std::to_string(50 + counter); // simulate increasing CPU usage
}

int main()
{

    LogManager logger(10); // ring buffer size

    // add console sink
    logger.add_sink(std::make_unique<ConsoleSinkImpl>());

    // add file sink
    logger.add_sink(std::make_unique<FileSinkImpl>("logs.txt"));

    // simulate 5 telemetry readings
    for (int i = 0; i < 5; i++)
    {
        std::string raw_value = getTelemetryValue();

        auto log_opt = Formatter<CPU_policy>::format(raw_value);

        if (log_opt.has_value())
        {
            // send message to logger
            logger << LogMessage(log_opt.value());
        }
        else
        {
            std::cout << "Failed to parse telemetry data." << std::endl;
        }
    }

    // write all messages to sinks
    logger.write();

    std::cout << "Telemetry logs sent to console and file.\n";

    return 0;
}
