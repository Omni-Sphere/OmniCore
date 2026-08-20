#pragma once

#include <string>

namespace omnisphere::dtos
{
    struct RevokePermissionInput
    {
        std::string userCode;
        std::string module;
        std::string permission;
    };
} // namespace omnisphere::dtos
