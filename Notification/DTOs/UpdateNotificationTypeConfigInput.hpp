#pragma once
#include <boost/describe.hpp>
#include <string>
#include <optional>

namespace omnisphere::dtos
{
    struct UpdateNotificationTypeConfigInput
    {
        std::string code;
        std::optional<std::string> name;
        std::optional<std::string> description;
        std::optional<std::string> requiredPermission;
        std::optional<bool> isEnabled;
        std::optional<std::string> rolesAllowed;
        std::optional<std::string> lastUpdatedBy;
        std::string updateDate;
    };

    BOOST_DESCRIBE_STRUCT(UpdateNotificationTypeConfigInput, (), (
        code,
        name,
        description,
        requiredPermission,
        isEnabled,
        rolesAllowed,
        lastUpdatedBy,
        updateDate
    ))
} // namespace omnisphere::dtos

