#pragma once
#include <string>
#include <optional>

namespace omnicore::dto
{
    struct SearchUsers
    {
        std::optional<std::string> CodeEqualsTo;
        std::optional<std::string> CodeContains;
        std::optional<std::string> NameEqualsTo;
        std::optional<std::string> NameContains;
        std::optional<std::string> EmailEqualsTo;
        std::optional<std::string> EmailContains;
        std::optional<std::string> PhoneEqualsTo;
        std::optional<std::string> PhoneContains;
    };
}