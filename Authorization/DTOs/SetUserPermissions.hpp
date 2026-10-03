#pragma once
#include <string>
#include <vector>
#include <boost/describe.hpp>
#include "Authorization/Enums/PermissionType.hpp"

namespace omnisphere::dtos
{
    struct SetUserPermissionsInput
    {
        std::string userCode;
        std::vector<omnisphere::enums::PermissionType> permissions;
        std::vector<omnisphere::enums::PermissionType> overridePermissions;
        std::string grantedByCode;
    };
    BOOST_DESCRIBE_STRUCT(SetUserPermissionsInput, (), (userCode, permissions, overridePermissions, grantedByCode))
} // namespace omnisphere::dtos
