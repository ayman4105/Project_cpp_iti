#include "LogManager.hpp"

void LogManager::add_sink(std::unique_ptr<ILogSink> sink)
{
    sinks.push_back(std::move(sink));
}

void LogManager::log(const LogMessage &message)
{

    if (!messages.tryPush(message))
    {
        std::cout << "[LogManager] buffer full, message dropped\n";
    }
    pool->push_task([this](){ this->write(); });
}

void LogManager::write()
{
    while (auto maybe_msg = messages.trypop())
    {
        for (auto &sink : sinks)
        {
            sink->write(maybe_msg.value());
        }
    }
}

LogManager &LogManager::operator<<(const LogMessage &message)
{
    this->log(message);
    return *this;
}