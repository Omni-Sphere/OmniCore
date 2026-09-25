#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models
{
    struct CustomButton
    {
        int entry = 0;
        int messageEntry = 0;
        std::string buttonId;
        std::string title;
        std::string actionType = "TRIGGER_MESSAGE"; // "EXECUTE_COMMAND", "TRIGGER_MESSAGE", "URL_REDIRECT", "TRANSFER_TO_AGENT"
        std::optional<std::string> actionPayload;
        int sortOrder = 1;
        std::string createdBy = "SYSTEM";
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(CustomButton, (), (
        entry,
        messageEntry,
        buttonId,
        title,
        actionType,
        actionPayload,
        sortOrder,
        createdBy,
        createDate
    ))
}
