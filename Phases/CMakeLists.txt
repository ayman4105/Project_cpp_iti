cmake_minimum_required(VERSION 3.22)
project(ITI_cpp)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# C++ Standard
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Qt Configuration
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 modules
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# CommonAPI / SOME/IP
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
find_package(CommonAPI REQUIRED)
find_package(CommonAPI-SomeIP REQUIRED)
find_package(vsomeip3 REQUIRED)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Include directories
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_src/src-gen
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Source files
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
file(GLOB SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/sinks/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/safe/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/telemetry/*.cpp
)

# Generated SOME/IP sources
set(GENERATED_SOMEIP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPDeployment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPProxy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPStubAdapter.cpp
)

# GUI sources
set(GUI_SOURCES
    gui/main_gui.cpp
    gui/LogModel.cpp
    gui/TelemetryBackend.cpp
)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# QML Resources
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
qt6_add_resources(QML_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/qml.qrc)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Console Application
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
add_executable(ITI_cpp
    app/main.cpp
    ${SRC_FILES}
    ${GENERATED_SOMEIP_SOURCES}
)

target_link_libraries(ITI_cpp
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# GUI Application
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
add_executable(ITI_cpp_gui
    ${GUI_SOURCES}
    ${SRC_FILES}
    ${GENERATED_SOMEIP_SOURCES}
    ${QML_RESOURCES}
)

target_link_libraries(ITI_cpp_gui
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Server
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
add_executable(server
    app/server.cpp
    ${GENERATED_SOMEIP_SOURCES}
)

target_link_libraries(server
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)