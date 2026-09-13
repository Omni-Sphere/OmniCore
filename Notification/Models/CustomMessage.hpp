#pragma once
#include "Notification/Models/CustomButton.hpp"
#include "Notification/Models/CustomMessageParameter.hpp"
#include <boost/describe.hpp>
#include <optional>
#include <string>
#include <vector>

namespace omnisphere::models
{
    struct CustomMessage
    {
        int entry = 0;
        std::string code;
        std::string title;
        std::string messageType = "INTERACTIVE_BUTTON"; // "TEXT", "INTERACTIVE_BUTTON", "INTERACTIVE_LIST"
        std::string headerType = "NONE";                 // "NONE", "TEXT", "IMAGE"
        std::optional<std::string> headerContent;
        std::string bodyTemplate;
        std::optional<std::string> footerText;
        bool isActive = true;
        int createdBy = 1;
        std::optional<std::string> createDate;
        std::optional<int> lastUpdatedBy;
        std::optional<std::string> updateDate;

        // Relations loaded at runtime
        std::vector<CustomButton> buttons;
        std::vector<CustomMessageParameter> parameters;
    };

    BOOST_DESCRIBE_STRUCT(CustomMessage, (), (
        entry,
        code,
        title,
        messageType,
        headerType,
        headerContent,
        bodyTemplate,
        footerText,
        isActive,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
}
