#pragma once
#include <string>
#include <optional>

namespace omnicore::dto
{
    struct ChangePassword
    {
        std::optional<int> Entry;
        std::optional<std::string> Code;
        std::string OldPassword;
        std::string NewPassword;
        std::string UpdateDate;
        int UpdatedBy;
    };
}