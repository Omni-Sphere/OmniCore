#pragma once
#include <string>
#include <optional>

namespace omnisphere::dtos
{
    struct ListDirectory
    {
        std::optional<std::string> Path;
        std::optional<std::string> Username;
        std::optional<std::string> Password;
        std::optional<std::string> Domain;
    };
} // namespace omnisphere::dtos
