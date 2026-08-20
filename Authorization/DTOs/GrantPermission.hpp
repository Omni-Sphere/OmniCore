#pragma once

#include <string>

namespace omnisphere::dtos
{
    struct GrantPermissionInput
    {
        std::string userCode;
        std::string module;
        std::string permission;
        std::string grantedByCode;
    };
} // namespace omnisphere::dtos
