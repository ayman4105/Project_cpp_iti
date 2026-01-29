#pragma once

#include<string>
#include <optional>


using string = std::string;

class ITelemetrySource{
    private:

    public:
        virtual bool open_source() = 0;
        virtual bool read_source(string& out) = 0;

        virtual ~ITelemetrySource() = default;
};