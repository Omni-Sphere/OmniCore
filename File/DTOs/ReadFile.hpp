#pragma once
#include <string>
#include <optional>

namespace omnisphere::dtos
{
    struct ReadFile
    {
        std::string FileName;
        std::optional<std::string> Path;
    };
} // namespace omnisphere::dtos
