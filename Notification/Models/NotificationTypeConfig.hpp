#pragma once
#include <boost/describe.hpp>
#include <string>
#include <optional>

namespace omnisphere::models
{
    struct NotificationTypeConfig
    {
        int entry = 0;
        std::string code;
        std::string name;
        std::optional<std::string> description;
        std::string moduleCode;
        std::string requiredPermission;
        bool isEnabled = true;
        std::optional<std::string> rolesAllowed;
        std::string createdBy = "SYSTEM";
        std::optional<std::string> createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(NotificationTypeConfig, (), (
        entry,
        code,
        name,
        description,
        moduleCode,
        requiredPermission,
        isEnabled,
        rolesAllowed,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
} // namespace omnisphere::models

