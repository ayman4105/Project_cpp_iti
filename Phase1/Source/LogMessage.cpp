#include"../Include/LogMessage.hpp"


LogMessage::LogMessage(const std::string& app, const std::string& cntxt, const std::string& msg, severity sev, std::chrono::system_clock::time_point time)
                     :app_name(app), context(cntxt), message(msg), level(sev), time(time){
}



std::string LogMessage::get_level() const{
    switch (level)
    {
        case severity::debug:
            return "Debug";
        case severity::info:
            return "Info";
        case severity::warning:
            return "Warning";
        case severity::error:
            return "Error";
        default:
            return "Unknown";    
    }
} 


std::string timePointToString(const std::chrono::system_clock::time_point& timePoint) {
    std::time_t rawTime = std::chrono::system_clock::to_time_t(timePoint);

    std::tm localTime = *std::localtime(&rawTime);

    std::stringstream ss;

    ss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

    return ss.str();
}


std::ostream& operator<<(std::ostream& os, const LogMessage& msg){
    os <<"[" << msg.app_name << "] "
    <<"[" << timePointToString(msg.time) << "] " 
    <<"[" << msg.context << "] "
    <<"[" << msg.get_level() << "] "
    <<"[" << msg.message << "]"
    << std::endl;
    return os;
}