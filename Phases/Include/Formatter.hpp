#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <sstream>
#include <iomanip>
#include "LogMessage.hpp"
#include "Types_of_enums_data/severity_type.hpp"
#include "magic_enum/magic_enum.hpp"
#include "policy/CPU_policy.hpp"
#include "policy/GPU_policy.hpp"
#include "policy/RAM_policy.hpp"

template <typename Policy>
class Formatter
{
public:
    static std::optional<LogMessage> format(const std::string &raw_value)
    {
        float value;
        try
        {
            value = std::stof(raw_value);
        }
        catch (std::exception error)
        {
            return std::nullopt;
        }

        auto sev = Policy::inferSeverity(value);
        std::string description = valueDescription(value, sev);
        std::string app_name = std::string(magic_enum::enum_name(Policy::context));
        std::string contextStr = std::string(magic_enum::enum_name(Policy::context));

        LogMessage msg{
            app_name,
            contextStr,
            description,
            sev,
            currentTimeStamp()};

        return msg;
    }

private:
    static std::string valueDescription(float value, severity_level sev)
    {
        std::string val_str = std::to_string(value);
        std::string unit = std::string(Policy::unit);

        switch (sev)
        {
        case severity_level::Critical:
            return "Critical: " + val_str + unit;
        case severity_level::Warning:
            return "Warning: " + val_str + unit;
        case severity_level::Info:
        default:
            return "Normal: " + val_str + unit;
        }
    }

    static std::string currentTimeStamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&t);
        std::ostringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};
