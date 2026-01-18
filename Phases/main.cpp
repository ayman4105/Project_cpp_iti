#include<iostream>
#include"Include/sinks/ConsoleSinkImpl.hpp"
#include"../Include/sinks/FileSinkImpl.hpp"
#include"Include/sinks/ILogSink.hpp"
#include"Include/LogManager.hpp"
#include"Include/LogMessage.hpp"
// #include"Include/safe/SafeFile.hpp"
// #include"Include/safe/SafeSocket.hpp"
#include"Include/telemetry/FileTelemetrySourceImpl.hpp"
#include"Include/telemetry/SocketTelemetrySourceImpl.hpp"




int main()
{
    LogManager manager;
    manager.add_sink(std::make_unique<ConsoleSinkImpl>());
    manager.add_sink(std::make_unique<FileSinkImpl>("log.txt"));
    std::cout<< "reached print\n";
    manager << LogMessage("app", "context", "message",severity::debug);


    


    
    return 0;
}
