#pragma once
#include "File/DTOs/ListDirectory.hpp"
#include "File/DTOs/ValidateDirectoryPermissions.hpp"
#include "File/DTOs/ConnectNetworkShare.hpp"
#include "File/DTOs/CreateDirectory.hpp"
#include "File/DTOs/ReadFile.hpp"
#include "File/DTOs/SaveFile.hpp"
#include "File/Models/DirectoryItem.hpp"
#include "File/Models/FileContent.hpp"
#include "File/Models/DirectoryPermissions.hpp"
#include <string>
#include <vector>
#include <optional>

namespace omnisphere::repositories
{
    class File
    {
    public:
        File() = default;
        ~File() = default;

        // Returns the expanded real path for network or local URIs
        static std::string ResolvePath(const std::string& rawPath);

        // List directory entries at the given path (resolves SMB/NFS URIs via GVFS/gio)
        std::vector<omnisphere::models::DirectoryItem> ReadDir(const omnisphere::dtos::ListDirectory& input) const;

        // Validate read and write permissions on a directory by creating/deleting a 0B probe file
        omnisphere::models::DirectoryPermissions TestPermissions(const omnisphere::dtos::ValidateDirectoryPermissions& input) const;

        // Mount SMB/NFS network share via gio mount on Linux, net use on Windows
        std::string MountShare(const omnisphere::dtos::ConnectNetworkShare& input, std::string& outError) const;

        // Get all active SMB/NFS network shares currently mounted on the OS
        std::vector<std::string> GetMountedShares() const;

        // Create a new directory under parentPath
        std::string MakeDir(const omnisphere::dtos::CreateDirectory& input) const;

        // Read a file and return its bytes
        std::vector<unsigned char> ReadBytes(const std::string& fullPath) const;

        // Write bytes to a file (overwrites)
        void WriteBytes(const std::string& fullPath, const std::vector<unsigned char>& data) const;

    private:
        // Internal helpers
        std::string AutoMountNetworkUri(const std::string& uri, const std::string& inputUser = "", const std::string& inputPass = "", const std::string& inputDom = "") const;
        static std::string GetUserHomeDirectory();
        static std::string ToLower(const std::string& s);
        static std::string EscapeShellArg(const std::string& arg);
        static std::vector<std::string> GioList(const std::string& uri);
        static std::string GetNetworkParentPath(const std::string& path);
        static bool IsNetworkUri(const std::string& path);
    };
} // namespace omnisphere::repositories
