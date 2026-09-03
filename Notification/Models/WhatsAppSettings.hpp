#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models
{
    struct WhatsAppSettings
    {
        int entry = 0;
        std::string code = "DEFAULT";
        std::string name = "MetaConfig";
        std::string phoneId;
        std::string apiToken;
        std::string businessAccountId;
        std::string webhookVerifyToken;
        std::string apiVersion = "v24.0";
        bool isActive = true;
        int createdBy = 1;
        std::optional<std::string> createDate;
        std::optional<int> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(WhatsAppSettings, (), (
        entry,
        code,
        name,
        phoneId,
        apiToken,
        businessAccountId,
        webhookVerifyToken,
        apiVersion,
        isActive,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
}
