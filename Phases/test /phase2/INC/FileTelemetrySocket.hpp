#pragma once 

#include"ITelemetrySource.hpp"
#include"socket.hpp"

class SocketTelemetrySrc: public ITelemetrySource
{
    using string = std::string;
private:
    string ip;
    uint16_t port;
    std::optional<SafeSocket> sock;
    
public:
    SocketTelemetrySrc(string ip, uint16_t port);
    bool open_source() override;
    bool read_source(string& out) override;
    ~SocketTelemetrySrc() = default;
};