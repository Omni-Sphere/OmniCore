#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models
{
    struct WhatsAppMessage
    {
        int entry = 0;
        std::optional<std::string> code;
        int conversationEntry = 0;
        std::string senderType = "OUTBOUND"; // "OUTBOUND" | "INBOUND"
        std::string messageType = "TEXT";    // "TEMPLATE" | "TEXT" | "IMAGE" | etc.
        std::optional<std::string> templateName;
        std::optional<std::string> content;
        std::optional<std::string> mediaUrl;
        std::string status = "SENT";         // "SENT" | "DELIVERED" | "READ" | "FAILED" | "RECEIVED"
        std::optional<std::string> responsePayload;
        std::optional<std::string> errorMessage;
        int sentBy = 1;
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppMessage, (), (
        entry,
        code,
        conversationEntry,
        senderType,
        messageType,
        templateName,
        content,
        mediaUrl,
        status,
        responsePayload,
        errorMessage,
        sentBy,
        createDate
    ))
}
