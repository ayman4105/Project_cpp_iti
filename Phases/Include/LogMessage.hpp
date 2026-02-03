#pragma once

#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include "Types_of_enums_data/severity_type.hpp"
#include "magic_enum/magic_enum.hpp"


class LogMessage
{

public:
    severity_level level;
    std::string app_name;
    std::string time;
    std::string message;
    std::string context;

    LogMessage(const std::string &app, const std::string &cntxt, const std::string &msg, severity_level sev, std::string time);
    ~LogMessage() = default;
};

std::ostream &operator<<(std::ostream &os, const LogMessage &msg);
