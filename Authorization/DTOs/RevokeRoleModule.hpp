#pragma once

#include <string>
#include "Authorization/Enums/ModuleType.hpp"

namespace omnisphere::dtos
{
    struct RevokeRoleModuleInput
    {
        std::string roleCode;
        omnisphere::enums::ModuleType module;
    };
} // namespace omnisphere::dtos
