#pragma once 

#include<string>
#include<chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

enum class severity{
    debug,
    info,
    warning,
    error
};

class LogMessage
{
  
public:
    severity level;
    std::string app_name;
    std::chrono::time_point<std::chrono::system_clock> time;
    std::string message;
    std::string context;

    LogMessage(const std::string& app, const std::string& cntxt, const std::string& msg, severity sev, std::chrono::system_clock::time_point time = std::chrono::system_clock::now());
    ~LogMessage()= default;
    std::string get_level() const;
};

    std::ostream& operator<<(std::ostream& os, const LogMessage& msg);

