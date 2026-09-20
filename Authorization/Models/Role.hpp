#pragma once
#include <string>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct Role
    {
        int entry = 0;
        std::string code;
        std::string name;
        std::string description;
        bool isActive = true;
    };
    BOOST_DESCRIBE_STRUCT(Role, (), (entry, code, name, description, isActive))
} // namespace omnisphere::models
