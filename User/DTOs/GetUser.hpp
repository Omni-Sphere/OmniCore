#pragma once
#include <string>
#include <optional>

namespace omnicore::dto
{
    struct GetUser
    {
        std::optional<int> Entry;
        std::optional<std::string> Code;
        std::optional<std::string> Name;
        std::optional<std::string> Email;
        std::optional<std::string> Phone;
        std::optional<std::string> SuperUser;
    };
}