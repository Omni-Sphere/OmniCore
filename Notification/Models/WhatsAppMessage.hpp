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
        std::optional<std::string> whatsAppId;
        int conversationEntry = 0;
        std::string senderType = "OUTBOUND"; // "OUTBOUND" | "INBOUND"
        std::string messageType = "TEXT";    // "TEMPLATE" | "TEXT" | "IMAGE" | etc.
        std::optional<std::string> templateName;
        std::optional<std::string> content;
        std::optional<std::string> mediaUrl;
        std::optional<std::string> mediaId;
        std::optional<std::string> mediaMimeType;
        std::optional<std::string> mediaFileName;
        std::optional<std::string> contextWhatsAppId;
        std::optional<std::string> messageTimestamp;
        std::optional<std::string> deliveredDate;
        std::optional<std::string> readDate;
        std::string status = "SENT";         // "SENT" | "DELIVERED" | "READ" | "FAILED" | "RECEIVED"
        std::optional<std::string> responsePayload;
        std::optional<std::string> errorMessage;
        int sentBy = 1;
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppMessage, (), (
        entry,
        code,
        whatsAppId,
        conversationEntry,
        senderType,
        messageType,
        templateName,
        content,
        mediaUrl,
        mediaId,
        mediaMimeType,
        mediaFileName,
        contextWhatsAppId,
        messageTimestamp,
        deliveredDate,
        readDate,
        status,
        responsePayload,
        errorMessage,
        sentBy,
        createDate
    ))
}
