#include"../../Include/sinks/FileSinkImpl.hpp"


FileSinkImpl::FileSinkImpl(const std::string& filename)
                            :file(std::ofstream(filename)){}


void FileSinkImpl::write(const LogMessage& message){
    file << message;
}