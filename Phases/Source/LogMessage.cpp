#include "../Include/LogMessage.hpp"

LogMessage::LogMessage(const std::string &app, const std::string &cntxt, const std::string &msg, severity_level sev, std::string time)
    : app_name(app), context(cntxt), message(msg), level(sev), time(time)
{
}

std::ostream &operator<<(std::ostream &os, const LogMessage &msg)
{
    os << "[" << msg.app_name << "] "
       << "[" << msg.time << "] "
       << "[" << msg.context << "] "
       << "[" << magic_enum::enum_name(msg.level) << "] "
       << "[" << msg.message << "]"
       << std::endl;
    return os;
}