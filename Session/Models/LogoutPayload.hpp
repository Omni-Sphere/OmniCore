#pragma once

#include <iostream>
#include "Enums/LogoutReason.hpp"

namespace omnicore::model
{
    class LogoutPayload
    {
    public:
        std::string SessionUUID;
        std::string StartDate;
        std::string EndDate;
        int Duration;
        enums::LogoutReason Reason;
        std::optional<std::string> Message;
    };
}