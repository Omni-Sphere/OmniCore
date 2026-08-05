#pragma once
#include <string>
#include <optional>

namespace omnisphere::dtos
{
    struct SaveFile
    {
        std::string FileName;
        std::optional<std::string> Path;
        std::string Content; // Base64 encoded content
    };
} // namespace omnisphere::dtos
