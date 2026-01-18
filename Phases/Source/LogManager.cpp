#include"../Include/LogManager.hpp"



void LogManager:: add_sink(std::unique_ptr<ILogSink> sink){
    sinks.push_back(std::move(sink));   
}


void LogManager:: log(const LogMessage& message){
    for (auto &sink : sinks)
    {
        sink->write(message);
    }
       
}


LogManager&  LogManager::operator<<(const LogMessage& message){
    std::cout<< "entered operator <<\n";
    this->log(message);
    return *this;
}