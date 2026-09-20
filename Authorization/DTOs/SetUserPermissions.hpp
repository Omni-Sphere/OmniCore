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
        std::string grantedByCode;
    };
    BOOST_DESCRIBE_STRUCT(SetUserPermissionsInput, (), (userCode, permissions, grantedByCode))
} // namespace omnisphere::dtos
