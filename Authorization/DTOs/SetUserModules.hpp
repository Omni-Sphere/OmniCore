#pragma once
#include <string>
#include <vector>
#include <boost/describe.hpp>
#include "Authorization/Enums/ModuleType.hpp"

namespace omnisphere::dtos
{
    struct SetUserModulesInput
    {
        std::string userCode;
        std::vector<omnisphere::enums::ModuleType> modules;
        std::vector<omnisphere::enums::ModuleType> overrideModules;
        std::string grantedByCode;
    };
    BOOST_DESCRIBE_STRUCT(SetUserModulesInput, (), (userCode, modules, overrideModules, grantedByCode))
} // namespace omnisphere::dtos
