#pragma once

#include <string>

namespace omnisphere::dtos
{
    struct RevokeRolePermissionInput
    {
        std::string roleCode;
        std::string module;
        std::string permission;
    };
} // namespace omnisphere::dtos
