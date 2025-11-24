#pragma once
#include <string>
#include <optional>

namespace omnicore::model
{
    /**
     * @brief Model representing global configuration settings
     * 
     * This model maps to the GlobalConfiguration table in the database.
     */
    class GlobalConfiguration
    {
    public:
        int ConfEntry; ///< Configuration entry ID (Primary Key)
        std::optional<std::string> ImagePath; ///< Path for storing images
        std::optional<std::string> PDFPath; ///< Path for storing PDF files
        std::optional<std::string> XMLPath; ///< Path for storing XML files
        int PasswordExpirationDays; ///< Number of days before password expires (0 = never expires)
    };
}
