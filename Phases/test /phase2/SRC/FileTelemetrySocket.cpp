#include"../INC/FileTelemetrySocket.hpp"


SocketTelemetrySrc::SocketTelemetrySrc(string _ip, uint16_t _port)
: ip(std::move(_ip)), port(_port)
{

}


bool SocketTelemetrySrc::open_source()
{
    sock = SafeSocket(ip, port);
    return true;
    
}

bool SocketTelemetrySrc::read_source(string& out)
{
    if (!sock)
    {
        return false;
    }
    
    return sock.value().receiveLine(out);
    
}