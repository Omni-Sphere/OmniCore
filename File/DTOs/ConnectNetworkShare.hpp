#pragma once
#include <string>
#include <optional>

namespace omnisphere::dtos
{
    struct ConnectNetworkShare
    {
        std::string Protocol; // "smb" or "nfs"
        std::string Server;
        std::optional<std::string> Share;
        std::optional<std::string> Username;
        std::optional<std::string> Password;
        std::optional<std::string> Domain;
    };
} // namespace omnisphere::dtos
