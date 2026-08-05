#pragma once
#include <string>

namespace omnisphere::models
{
    struct DirectoryItem
    {
        std::string Name;
        std::string Path;
        bool IsDirectory;
    };
} // namespace omnisphere::models
