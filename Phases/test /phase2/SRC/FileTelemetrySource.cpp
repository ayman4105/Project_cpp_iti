#include"../INC/FileTelemetrySource.hpp"


FileTelemetrySource::FileTelemetrySource(string path) : path{std::move(path)}{

}

bool FileTelemetrySource::open_source(){
    // file = safefile(path);
    // return true;

    try {
    file.emplace(path);
    return true;
    } catch(...) {
        return false;
    }

}


bool FileTelemetrySource::read_source(string& out){
    if(!file){
        return false;
    }

    return file->readline(out);
}