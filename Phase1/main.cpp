#include<iostream>
#include"Include/ConsoleSinkImpl.hpp"
#include"Include/FileSinkImpl.hpp"
#include"Include/ILogSink.hpp"
#include"Include/LogManager.hpp"
#include"Include/LogMessage.hpp"

int main()
{
    LogManager manager;
    manager.add_sink(std::make_unique<ConsoleSinkImpl>());
    manager.add_sink(std::make_unique<FileSinkImpl>("log.txt"));
    std::cout<< "reached print\n";
    manager << LogMessage("app", "context", "message",severity::debug);


    
    return 0;
}
