#pragma once

#include <string>

namespace omnisphere::dtos
{
    struct GrantRolePermissionInput
    {
        std::string roleCode;
        std::string module;
        std::string permission;
    };
} // namespace omnisphere::dtos
