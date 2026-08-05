#pragma once
#include <string>

namespace omnisphere::dtos
{
    struct CreateDirectory
    {
        std::string ParentPath;
        std::string FolderName;
    };
} // namespace omnisphere::dtos
