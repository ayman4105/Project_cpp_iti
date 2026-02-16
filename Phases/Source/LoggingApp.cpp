#include "LoggingApp.hpp"
#include "sinks/ConsoleSinkImpl.hpp"
#include "sinks/FileSinkImpl.hpp"
#include "LogMessage.hpp"
#include <iostream>
#include <chrono>
#include <optional>
#include <csignal>

// Global pointer to handle signals
static TelemetryLoggingApp* g_app_instance = nullptr;

TelemetryLoggingApp::TelemetryLoggingApp(const std::string &configPath)
{
    loadConfig(configPath);
    setupSinks();

    logger = std::make_unique<LogManager>(buffer_capacity, thread_pool_size);

    // add sinks to logger
    for (auto &sink : sinks)
    {
        logger->add_sink(std::move(sink));
    }

    g_app_instance = this;

    // handle Ctrl+C
    std::signal(SIGINT, TelemetryLoggingApp::signalHandler);
    std::signal(SIGTERM, TelemetryLoggingApp::signalHandler);
}

TelemetryLoggingApp::~TelemetryLoggingApp()
{
    isRunning = false;
    for (auto &t : sourceThreads)
        if (t.joinable())
            t.join();
    if (writerThread_.joinable())
        writerThread_.join();
}

void TelemetryLoggingApp::loadConfig(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open config: " + path);
    file >> config;

    buffer_capacity = config["log_manager"].value("buffer_capacity", 100);
    thread_pool_size = config["log_manager"].value("thread_pool_size", 2);
    sink_flush_rate_ms = config["log_manager"].value("sink_flush_rate_ms", 500);
}

void TelemetryLoggingApp::setupSinks()
{
    // console sink
    if (config["sinks"]["console"].value("enabled", false))
    {
        sinks.push_back(std::make_unique<ConsoleSinkImpl>());
    }

    // file sinks
    if (config["sinks"].contains("files"))
    {
        for (auto &f : config["sinks"]["files"])
        {
            if (f.value("enabled", false))
            {
                std::string path = f.value("path", "");
                if (!path.empty())
                    sinks.push_back(std::make_unique<FileSinkImpl>(path));
            }
        }
    }
}

void TelemetryLoggingApp::setupTelemetrySources()
{
    // FILE source
    if (config["sources"]["file"].value("enabled", false))
    {
        std::string path = config["sources"]["file"].value("path", "");
        int rate = config["sources"]["file"].value("parse_rate_ms", 1000);
        std::string policy = config["sources"]["file"].value("policy", "cpu");

        sourceThreads.emplace_back([this, path, rate, policy]()
        {
            auto source = std::make_unique<FileTelemetrySrc>(path);

            if (!source->openSource())
                return;

            while (isRunning)
            {
                std::string raw;

                if (source->readSource(raw))
                {
                    std::optional<LogMessage> msg;

                    if (policy == "cpu")
                        msg = Formatter<CPU_policy>::format(raw);
                    else if (policy == "ram")
                        msg = Formatter<RAM_policy>::format(raw);
                    else if (policy == "gpu")
                        msg = Formatter<GPU_policy>::format(raw);

                    if (msg.has_value())
                        logger->log(msg.value());
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(rate));
            }
        });
    }

    // to run soket use this command nc -lk 12345 in  terminal 
    // SOCKET source
    if (config["sources"]["socket"].value("enabled", false))
    {
        std::string ip = config["sources"]["socket"].value("ip", "127.0.0.1");
        uint16_t port = config["sources"]["socket"].value("port", 12345);
        int rate = config["sources"]["socket"].value("parse_rate_ms", 1000);
        std::string policy = config["sources"]["socket"].value("policy", "ram");

        sourceThreads.emplace_back([this, ip, port, rate, policy]()
                                   {
        auto source = std::make_unique<SocketTelemetrySrc>(ip, port); // دلوقتي صح
        while (isRunning) {
            if (source->openSource()) {
                std::string raw;
                if (source->readSource(raw)) {
                    std::optional<LogMessage> msg;
                    if (policy == "cpu") msg = Formatter<CPU_policy>::format(raw);
                    else if (policy == "ram") msg = Formatter<RAM_policy>::format(raw);
                    else if (policy == "gpu") msg = Formatter<GPU_policy>::format(raw);

                    if (msg.has_value()) logger->log(msg.value());
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(rate));
        } });
    }

    // SOMEIP source
    if (config["sources"]["someip"].value("enabled", false))
    {
        int rate = config["sources"]["someip"].value("parse_rate_ms", 1000);
        std::string policy = config["sources"]["someip"].value("policy", "gpu");

        sourceThreads.emplace_back([this, rate, policy]()
                                   {
            auto &source = SomeIPTelemetrySourceImpl::instance();
            if (!source.openSource()) return;

            while (isRunning) {
                std::string raw;
                if (source.readSource(raw)) {
                    std::optional<LogMessage> msg;
                    if (policy == "cpu") msg = Formatter<CPU_policy>::format(raw);
                    else if (policy == "ram") msg = Formatter<RAM_policy>::format(raw);
                    else if (policy == "gpu") msg = Formatter<GPU_policy>::format(raw);

                    if (msg.has_value()) logger->log(msg.value());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(rate));
            } });
    }
}

void TelemetryLoggingApp::startWriterThread()
{
    writerThread_ = std::thread([this]()
    {
        while (isRunning)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(sink_flush_rate_ms));
            logger->write(); // flush messages to sinks
        }
        logger->write(); // flush remaining messages on shutdown
    });
}

void TelemetryLoggingApp::signalHandler(int signal) { 
    if (g_app_instance) 
        g_app_instance->isRunning = false; 
    exit(0);
}

void TelemetryLoggingApp::start()
{
    isRunning = true;

    // writer thread
    startWriterThread();
    setupTelemetrySources(); 

    // join all threads (blocking main)
    for (auto &t : sourceThreads)
        if (t.joinable())
            t.join();
    if (writerThread_.joinable())
        writerThread_.join();
}
