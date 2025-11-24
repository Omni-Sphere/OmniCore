#pragma once

#include <string>
#include <optional>

namespace omnicore::dto
{
    struct Login
    {
        std::optional<std::string> Code;
        std::optional<std::string> Email;
        std::optional<std::string> Phone;
        std::string StartDate;
        std::string DeviceIP;
        std::string HostName;
        std::string Password;
    };
}