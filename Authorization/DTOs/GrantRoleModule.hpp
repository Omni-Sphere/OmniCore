#pragma once

#include <string>
#include "Authorization/Enums/ModuleType.hpp"

namespace omnisphere::dtos
{
    struct GrantRoleModuleInput
    {
        std::string roleCode;
        omnisphere::enums::ModuleType module;
    };
} // namespace omnisphere::dtos
