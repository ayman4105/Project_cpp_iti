#include "LoggingApp.hpp"

int main() {
    // create app with config path
    TelemetryLoggingApp app("/home/ayman/ITI/Project_cpp_iti/Phases/config.json");

    // start all sources & writer thread
    app.start();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
