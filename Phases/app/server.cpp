#include <v1/omnimetron/gpu/GpuUsageDataStubDefault.hpp>
#include <CommonAPI/CommonAPI.hpp>
#include <random>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

std::string DOMAIN = "local";
std::string INSTANCE = "omnimetron.gpu.GpuUsageData";

class SimpleGpuServer : public v1::omnimetron::gpu::GpuUsageDataStubDefault
{
private:
    // Generate random float between 0-100
    float randomFloat(float min = 0.0f, float max = 100.0f)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    float getGpuUsage()
    {
        return randomFloat();
    }

public:
    // Client sends request -> respond once
    void requestGpuUsageData(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        requestGpuUsageDataReply_t _reply) override
    {
        float usage = getGpuUsage();
        std::cout << "[Server] Client requested GPU usage: " << usage << "%" << std::endl;
        _reply(usage); // send response
    }

    // Broadcast GPU usage events to all clients
    void broadcastGpuUsageChange()
    {
        float usage = getGpuUsage();
        std::cout << "[Server] Broadcasting GPU usage: " << usage << "%" << std::endl;
        fireNotifyGpuUsageDataChangeEvent(usage);
    }
};

int main()
{
    // Get CommonAPI runtime
    auto runtime = CommonAPI::Runtime::get();

    // Create and register service
    auto service = std::make_shared<SimpleGpuServer>();
    runtime->registerService(std::string(DOMAIN), std::string(INSTANCE), service);

    std::cout << "[Server] GPU Service running..." << std::endl;

    // Loop: keep sending events every 1 second
    while (true)
    {
        std::this_thread::sleep_for(1s);
        service->broadcastGpuUsageChange();
    }

    return 0;
}