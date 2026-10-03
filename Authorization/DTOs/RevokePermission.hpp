#pragma once

#include <string>
#include "Authorization/Enums/PermissionType.hpp"

namespace omnisphere::dtos
{
    struct RevokePermissionInput
    {
        std::string userCode;
        omnisphere::enums::PermissionType permission = omnisphere::enums::PermissionType::UNKNOWN;
    };
} // namespace omnisphere::dtos
