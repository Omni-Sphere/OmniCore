#pragma once
#include <string>
#include <vector>
#include <boost/describe.hpp>
#include "Authorization/Enums/ModuleType.hpp"

namespace omnisphere::dtos
{
    struct SetRoleModulesInput
    {
        std::string roleCode;
        std::vector<omnisphere::enums::ModuleType> modules;
        std::vector<omnisphere::enums::ModuleType> overrideModules;
    };
    BOOST_DESCRIBE_STRUCT(SetRoleModulesInput, (), (roleCode, modules, overrideModules))
} // namespace omnisphere::dtos
