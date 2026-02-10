#pragma once

#include <atomic>
#include <mutex>
#include <string>

#include <CommonAPI/CommonAPI.hpp>
#include "ITelemetrySource.hpp"
#include "v1/omnimetron/gpu/GpuUsageDataProxy.hpp"

class SomeIPTelemetrySourceImpl : public ITelemetrySource
{
private:
    SomeIPTelemetrySourceImpl();

    // CommonAPI
    std::shared_ptr<CommonAPI::Runtime> runtime;
    std::shared_ptr<v1::omnimetron::gpu::GpuUsageDataProxy<>> proxy;

    // last received value from event
    std::mutex data_mutex;
    float last_usage;
    std::atomic<bool> has_new_data;

public:
    // Singleton access
    static SomeIPTelemetrySourceImpl &instance();

    bool start();

    // ITelemetrySource
    bool openSource() override;
    bool readSource(std::string &out) override;
};
