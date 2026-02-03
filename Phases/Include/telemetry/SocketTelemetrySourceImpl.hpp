#pragma onces

#include "telemetry/ITelemetrySource.hpp"
#include "safe/SafeSocket.hpp"

class SocketTelemetrySrc : public ITelemetrySource
{
    using string = std::string;

private:
    string ip;
    uint16_t port;
    std::optional<SafeSocket> sock;

public:
    SocketTelemetrySrc(string ip, uint16_t port);
    bool openSource() override;
    bool readSource(string &out) override;
    ~SocketTelemetrySrc() = default;
};