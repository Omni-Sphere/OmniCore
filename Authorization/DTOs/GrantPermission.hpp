#pragma once

#include <string>
#include "Authorization/Enums/PermissionType.hpp"

namespace omnisphere::dtos
{
    struct GrantPermissionInput
    {
        std::string userCode;
        omnisphere::enums::PermissionType permission = omnisphere::enums::PermissionType::UNKNOWN;
        std::string grantedByCode;
    };
} // namespace omnisphere::dtos
