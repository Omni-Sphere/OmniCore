#pragma once
#include <string>
#include <vector>
#include <boost/describe.hpp>
#include "Authorization/Enums/PermissionType.hpp"

namespace omnisphere::dtos
{
    struct SetRolePermissionsInput
    {
        std::string roleCode;
        std::vector<omnisphere::enums::PermissionType> permissions;
        std::vector<omnisphere::enums::PermissionType> overridePermissions;
    };
    BOOST_DESCRIBE_STRUCT(SetRolePermissionsInput, (), (roleCode, permissions, overridePermissions))
} // namespace omnisphere::dtos
