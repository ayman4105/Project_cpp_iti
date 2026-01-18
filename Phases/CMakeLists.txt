cmake_minimum_required(VERSION 3.16)


project(ITI_cpp)


add_executable(
    ${PROJECT_NAME}
    Source/sinks/FileSinkImpl.cpp
    Source/LogManager.cpp
    Source/LogMessage.cpp
    Source/sinks/ConsoleSinkImpl.cpp
    Source/safe/SafeFile.cpp
    Source/safe/SafeSocket.cpp
    Source/telemetry/FileTelemetrySourceImpl.cpp
    Source/telemetry/SocketTelemetrySourceImpl.cpp
    main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE Include/)