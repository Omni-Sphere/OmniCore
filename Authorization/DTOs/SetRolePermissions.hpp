#pragma once
#include <string>
#include <vector>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct SetRolePermissionsInput
    {
        std::string roleCode;
        std::vector<std::string> permissions;
        std::vector<std::string> overridePermissions;
    };
    BOOST_DESCRIBE_STRUCT(SetRolePermissionsInput, (), (roleCode, permissions, overridePermissions))
} // namespace omnisphere::dtos
