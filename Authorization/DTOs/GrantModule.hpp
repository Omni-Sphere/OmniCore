#pragma once

#include <string>
#include "Authorization/Enums/ModuleType.hpp"

namespace omnisphere::dtos
{
    struct GrantUserModuleInput
    {
        std::string userCode;
        omnisphere::enums::ModuleType module;
        std::string grantedByCode;
    };
} // namespace omnisphere::dtos
