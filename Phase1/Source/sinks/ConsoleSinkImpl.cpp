#include"../../Include/sinks/ConsoleSinkImpl.hpp"
#include <iostream>


void ConsoleSinkImpl::write(const LogMessage& message) {
    std::cout<<message;
}