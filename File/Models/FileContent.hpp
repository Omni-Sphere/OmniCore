#pragma once
#include <string>
#include <optional>

namespace omnisphere::models
{
    struct FileContent
    {
        std::string FileName;
        std::string FullPath;
        std::optional<std::string> DataUrl;
        std::optional<std::string> MimeType;
    };
} // namespace omnisphere::models
