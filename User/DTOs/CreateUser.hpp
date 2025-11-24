#pragma once
#include <string>
#include <optional>

namespace omnicore::dto
{
    struct CreateUser
    {
        std::string Code;
        std::optional<std::string> Name;
        std::optional<std::string> Email;
        std::optional<std::string> Phone;
        std::optional<int> Employee;
        std::string Password;
        bool SuperUser;
        bool ChangePasswordNextLogin;
        bool PasswordNeverExpires;
        int CreatedBy;
        std::string CreateDate;
    };
}