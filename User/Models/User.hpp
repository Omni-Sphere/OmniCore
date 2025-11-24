#pragma once
#include <string>
#include <optional>

namespace omnicore::model
{
    /**
     * @brief Model representing a user account
     * 
     * This model maps to the Users table in the database.
     */
    class User
    {
    public:
        int Entry; ///< User entry ID (Primary Key)
        std::string Code; ///< Unique user code
        std::optional<std::string> Name; ///< User's full name
        std::optional<std::string> Email; ///< User's email address
        std::optional<std::string> Phone; ///< User's phone number
        std::optional<int> Employee; ///< Associated employee entry ID
        bool SuperUser; ///< Whether user has super user privileges
        bool IsLocked; ///< Whether the account is locked
        bool IsActive; ///< Whether the account is active
        bool ChangePasswordNextLogin; ///< Whether user must change password on next login
        bool PasswordNeverExpires; ///< Whether the password never expires
        int CreatedBy; ///< Entry ID of user who created this account
        std::string CreateDate; ///< Account creation date
        std::optional<int> LastUpdatedBy; ///< Entry ID of user who last updated this account
        std::optional<std::string> UpdateDate; ///< Last update date
    };
}