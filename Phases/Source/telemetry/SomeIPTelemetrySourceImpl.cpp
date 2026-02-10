#include "telemetry/SomeIPTelemetrySourceImpl.hpp"

std::string DOMAIN = "local";
std::string INSTANCE = "omnimetron.gpu.GpuUsageData";

SomeIPTelemetrySourceImpl &SomeIPTelemetrySourceImpl::instance()
{
    static SomeIPTelemetrySourceImpl inst;
    return inst;
}

SomeIPTelemetrySourceImpl::SomeIPTelemetrySourceImpl()
    : last_usage(0.0f), has_new_data(false) {}

bool SomeIPTelemetrySourceImpl::openSource()
{
    runtime = CommonAPI::Runtime::get();

    proxy = runtime->buildProxy<v1::omnimetron::gpu::GpuUsageDataProxy>(
        DOMAIN, INSTANCE);

    return proxy != nullptr;
}

bool SomeIPTelemetrySourceImpl::start()
{
    if (!proxy)
        return false;

    auto &event = proxy->getNotifyGpuUsageDataChangeEvent();
    event.subscribe([this](const float &usage)
                    {
        std::lock_guard<std::mutex> lock(data_mutex);
        last_usage = usage;
        has_new_data.store(true); });

    return true;
}

bool SomeIPTelemetrySourceImpl::readSource(std::string &out)
{
    if (!proxy)
        return false;

    CommonAPI::CallStatus status;
    float usage = 0.0f;

    proxy->requestGpuUsageData(status, usage, nullptr);

    if (status != CommonAPI::CallStatus::SUCCESS)
        return false;

    out = std::to_string(usage);
    return true;
}
