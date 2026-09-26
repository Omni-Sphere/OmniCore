#pragma once
#include <string>
#include <vector>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct SetUserPermissionsInput
    {
        std::string userCode;
        std::vector<std::string> permissions;
        std::vector<std::string> overridePermissions;
        std::string grantedByCode;
    };
    BOOST_DESCRIBE_STRUCT(SetUserPermissionsInput, (), (userCode, permissions, overridePermissions, grantedByCode))
} // namespace omnisphere::dtos
