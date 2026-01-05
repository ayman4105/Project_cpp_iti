#include"../Include/FileSinkImpl.hpp"


FileSinkImpl::FileSinkImpl(const std::string& filename)
                            :file(std::ofstream(filename)){}


void FileSinkImpl::write(const LogMessage& message){
    file << message;
}