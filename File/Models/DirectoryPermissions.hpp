#pragma once
#include <string>

namespace omnisphere::models
{
    struct DirectoryPermissions
    {
        std::string Path;
        bool HasReadAccess;
        bool HasWriteAccess;
    };
} // namespace omnisphere::models
