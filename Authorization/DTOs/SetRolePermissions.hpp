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
    };
    BOOST_DESCRIBE_STRUCT(SetRolePermissionsInput, (), (roleCode, permissions))
} // namespace omnisphere::dtos
