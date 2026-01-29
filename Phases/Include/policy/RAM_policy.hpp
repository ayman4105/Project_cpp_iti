#pragma once 

#include <string_view>

#include "../Types_of_enums_data/severity_type.hpp"
#include "../Types_of_enums_data/telemetry_source.hpp"


struct RAM_policy
{
    static constexpr enum_telem_src context = enum_telem_src::RAM;
    static constexpr std::string_view unit = "%";

    static constexpr float Warning = 75.5f;
    static constexpr float Critical = 90.0f;

    static constexpr severity_level inferSeverity(float value) noexcept{
        return (value >= Critical) ? severity_level::Critical :
               (value >= Warning)  ? severity_level::Warning :
                                     severity_level::Info;
    }
    
};




