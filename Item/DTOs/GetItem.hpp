#pragma once

#include <string>
#include <optional>

namespace omnicore::dto 
{
    struct GetItem {
        std::optional<std::string> Code;
        std::optional<std::string> Name;
    };
}