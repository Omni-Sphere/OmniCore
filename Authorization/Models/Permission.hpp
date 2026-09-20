#pragma once
#include <string>
#include <vector>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct PermissionItem
    {
        std::string code;
        std::string name;
        std::string description;
        std::string moduleCode;
    };
    BOOST_DESCRIBE_STRUCT(PermissionItem, (), (code, name, description, moduleCode))

    struct PermissionModule
    {
        std::string code;
        std::string name;
        std::string icon;
        std::vector<PermissionItem> permissions;
    };
    BOOST_DESCRIBE_STRUCT(PermissionModule, (), (code, name, icon, permissions))
} // namespace omnisphere::models
