#pragma once

#include <string>
#include "Authorization/Enums/ModuleType.hpp"

namespace omnisphere::dtos
{
    struct RevokeUserModuleInput
    {
        std::string userCode;
        omnisphere::enums::ModuleType module;
    };
} // namespace omnisphere::dtos
