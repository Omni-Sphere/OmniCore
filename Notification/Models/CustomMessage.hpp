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
        std::optional<std::string> metaTemplateId;
        std::optional<std::string> metaStatus;
        std::optional<std::string> metaCategory;
        std::optional<std::string> metaRejectReason;
        bool isActive = true;
        std::string createdBy = "SYSTEM";
        std::optional<std::string> createDate;
        std::optional<std::string> lastUpdatedBy;
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
        metaTemplateId,
        metaStatus,
        metaCategory,
        metaRejectReason,
        isActive,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
}
