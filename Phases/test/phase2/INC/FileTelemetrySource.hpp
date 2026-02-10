#pragma once 

#include"ITelemetrySource.hpp"
#include"safefile.hpp"

using string = std::string;


class FileTelemetrySource : public ITelemetrySource{
    private:
        std::optional<safefile> file;
        string path;

    public:
        FileTelemetrySource(string path);
        ~FileTelemetrySource() = default;

        bool read_source(string& out) override;
        bool open_source() override;




}; 


