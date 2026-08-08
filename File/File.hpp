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
#include <memory>
#include <optional>

namespace omnisphere::services
{
    class File
    {
    public:
        File();
        ~File();

        // List directory contents (local or network SMB/NFS)
        std::vector<omnisphere::models::DirectoryItem> ListDirectory(const omnisphere::dtos::ListDirectory& input) const;

        // Validate read/write access by creating and deleting a 0B probe file
        omnisphere::models::DirectoryPermissions ValidateDirectoryPermissions(const omnisphere::dtos::ValidateDirectoryPermissions& input) const;

        // Connect to an SMB or NFS network share
        std::string ConnectNetworkShare(const omnisphere::dtos::ConnectNetworkShare& input) const;

        // Get active OS mounted SMB/NFS network shares
        std::vector<std::string> GetMountedShares() const;

        // Create a new directory
        std::string CreateDirectory(const omnisphere::dtos::CreateDirectory& input) const;

        // Read a file and return it as a Base64 Data URL
        omnisphere::models::FileContent ReadFile(const omnisphere::dtos::ReadFile& input) const;

        // Save a file from a Base64 encoded content string
        omnisphere::models::FileContent SaveFile(const omnisphere::dtos::SaveFile& input) const;

        // Resolve raw path (local or network URI) to an accessible filesystem path
        static std::string ResolvePath(const std::string& rawPath);

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl;

        static std::string DetectMimeType(const std::string& fileName);
        static std::string Base64Encode(const std::vector<unsigned char>& data);
        static std::vector<unsigned char> Base64Decode(const std::string& in);
    };
} // namespace omnisphere::services
