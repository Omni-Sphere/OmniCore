#pragma once

#include <string>
#include "Authorization/Enums/PermissionType.hpp"

namespace omnisphere::dtos
{
    struct RevokeRolePermissionInput
    {
        std::string roleCode;
        omnisphere::enums::PermissionType permission = omnisphere::enums::PermissionType::UNKNOWN;
    };
} // namespace omnisphere::dtos
