#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models
{
    struct WhatsAppConversation
    {
        int entry = 0;
        std::string code;
        std::string customerPhone;
        std::optional<std::string> customerName;
        std::optional<std::string> lastMessageText;
        std::optional<std::string> lastMessageDate;
        int unreadCount = 0;
        std::string status = "OPEN";
        bool isActive = true;
        int createdBy = 1;
        std::optional<std::string> createDate;
        std::optional<int> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppConversation, (), (
        entry,
        code,
        customerPhone,
        customerName,
        lastMessageText,
        lastMessageDate,
        unreadCount,
        status,
        isActive,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
}
