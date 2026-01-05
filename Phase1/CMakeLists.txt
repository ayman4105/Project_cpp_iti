cmake_minimum_required(VERSION 3.16)


project(ITI_cpp)


add_executable(
    ${PROJECT_NAME}
    Source/FileSinkImpl.cpp
    Source/LogManager.cpp
    Source/LogMessage.cpp
    Source/ConsoleSinkImpl.cpp
    main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE Include/)