#pragma once
#include <OmniData/SQLParams.hpp>

namespace omnisphere::enums
{
    enum class PermissionMode
    {
        P,
        R
    };
}

namespace omnisphere::types
{
    inline SQLParam MakeSQLParam(omnisphere::enums::PermissionMode val)
    {
        return SQLParam{val == omnisphere::enums::PermissionMode::R ? std::string("R") : std::string("P")};
    }
}

