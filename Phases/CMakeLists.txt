cmake_minimum_required(VERSION 3.22) # better make the version 3.22 and above

project(ITI_cpp)

# CommonAPI
find_package(CommonAPI REQUIRED)
find_package(CommonAPI-SomeIP REQUIRED)
find_package(vsomeip3 REQUIRED)

include_directories(
    ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/Include/
    ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/gen_src/src-gen

)

# collect all source files into one variable
file(GLOB SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/sinks/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/safe/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/telemetry/*.cpp
)

set(GENERATED_SOMEIP_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPDeployment.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPProxy.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gen_src/src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPStubAdapter.cpp
)

add_executable(${PROJECT_NAME}
    app/main.cpp
    ${SRC_FILES}
    ${GENERATED_SOMEIP_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)

add_executable(server
    app/server.cpp
    ${GENERATED_SOMEIP_SOURCES}

)

target_link_libraries(server
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)
