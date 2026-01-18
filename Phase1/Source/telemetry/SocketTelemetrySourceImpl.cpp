#include"../../Include/telemetry/SocketTelemetrySourceImpl.hpp"


SocketTelemetrySrc::SocketTelemetrySrc(string _ip, uint16_t _port)
: ip(std::move(_ip)), port(_port)
{

}


bool SocketTelemetrySrc::openSource()
{
    sock = SafeSocket(ip, port);
    return true;
    
}

bool SocketTelemetrySrc::readSource(string& out)
{
    if (!sock)
    {
        return false;
    }
    
    return sock.value().receiveLine(out);
    
}