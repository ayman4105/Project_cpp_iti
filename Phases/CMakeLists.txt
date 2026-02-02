cmake_minimum_required(VERSION 3.22) # better make the version 3.22 and above

project(ITI_cpp)


# collect all source files into one variable
file(GLOB SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/sinks/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/safe/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/telemetry/*.cpp
)

# create executable by passing all source files
add_executable(
    ${PROJECT_NAME}
    ${SRC_FILES}
)

# this command makes the directory named "Include" visible for all source files
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include/)

# meaning that to include "Include/LogManager.hpp" simply write it as
# #include "LogManager.hpp"
# Ex: to include "Include/Types_of_enums_data/severity_type.hpp" into ANY file in the project, write it like this:
# #include "Types_of_enums/severity_type.hpp"