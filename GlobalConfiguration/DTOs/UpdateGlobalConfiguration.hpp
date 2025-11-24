#pragma once
#include <string>
#include <optional>

namespace omnicore::dto
{
    /**
     * @brief DTO for updating global configuration settings
     * 
     * All fields are optional. Only the fields with values will be updated.
     */
    struct UpdateGlobalConfiguration
    {
        std::optional<std::string> ImagePath; ///< New path for storing images
        std::optional<std::string> PDFPath; ///< New path for storing PDF files
        std::optional<std::string> XMLPath; ///< New path for storing XML files
        std::optional<int> PasswordExpirationDays; ///< New password expiration period in days
    };
}
