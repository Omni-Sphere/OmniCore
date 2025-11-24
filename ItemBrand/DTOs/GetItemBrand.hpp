#pragma once

#include <string>
#include <optional>

namespace omnicore::dto
{
    struct GetItemBrand
    {
        std::optional<int> Entry;
        std::optional<std::string> Code;
        std::optional<std::string> Name;
    };
}