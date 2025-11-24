#pragma once

#include <string>
#include <optional>

#include "Enums/LogoutReason.hpp"

namespace omnicore::dto
{
    struct Logout
    {
        std::string SessionUUID;
        std::string EndDate;
        enums::LogoutReason Reason;
        std::optional<std::string> Message;
    };
}