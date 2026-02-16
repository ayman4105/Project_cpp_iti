#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <fstream>
#include "nlohmann_json/json.hpp"
#include "sinks/ILogSink.hpp"
#include "LogManager.hpp"
#include "telemetry/ITelemetrySource.hpp"
#include "telemetry/FileTelemetrySourceImpl.hpp"
#include "telemetry/SocketTelemetrySourceImpl.hpp"
#include "Formatter.hpp"
#include "telemetry/SomeIPTelemetrySourceImpl.hpp"

class TelemetryLoggingApp
{
private:
    void loadConfig(const std::string &path);
    void setupSinks();
    void setupTelemetrySources();
    void startWriterThread();

    nlohmann::json config;
    std::unique_ptr<LogManager> logger;

    std::vector<std::unique_ptr<ILogSink>> sinks;
    std::vector<std::unique_ptr<ITelemetrySource>> sources;
    std::vector<std::thread> sourceThreads;

    std::thread writerThread_;
    int buffer_capacity;
    int thread_pool_size;
    int sink_flush_rate_ms;

    std::atomic<bool> isRunning{false};

public:
    explicit TelemetryLoggingApp(const std::string &configPath);
    void start();

    static void signalHandler(int);

    ~TelemetryLoggingApp();

};
