#include <File/File.hpp>
#include <File/Repositories/File.hpp>
#include <filesystem>
#include <stdexcept>
#include <memory>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace omnisphere::services
{

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

struct File::Impl
{
    omnisphere::repositories::File repo;
    Impl() = default;
};

File::File() : pimpl(std::make_unique<Impl>()) {}
File::~File() = default;

// ---------------------------------------------------------------------------
// Base64 helpers
// ---------------------------------------------------------------------------

std::string File::Base64Encode(const std::vector<unsigned char>& data)
{
    static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    int val = 0, valb = -6;
    for (unsigned char c : data)
    {
        val  = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            out.push_back(lookup[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::vector<unsigned char> File::Base64Decode(const std::string& in)
{
    std::vector<unsigned char> out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++)
        T[static_cast<unsigned char>("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i])] = i;
    int val = 0, valb = -8;
    for (unsigned char c : in)
    {
        if (T[c] == -1) break;
        val  = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            out.push_back(static_cast<unsigned char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::string File::DetectMimeType(const std::string& fileName)
{
    std::string ext = fs::path(fileName).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png")  return "image/png";
    if (ext == ".gif")  return "image/gif";
    if (ext == ".webp") return "image/webp";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".pdf")  return "application/pdf";
    if (ext == ".txt")  return "text/plain";
    if (ext == ".json") return "application/json";
    if (ext == ".xml")  return "application/xml";
    if (ext == ".csv")  return "text/csv";
    return "application/octet-stream";
}

// ---------------------------------------------------------------------------
// ResolvePath (delegated to repository static)
// ---------------------------------------------------------------------------

std::string File::ResolvePath(const std::string& rawPath)
{
    return omnisphere::repositories::File::ResolvePath(rawPath);
}

// ---------------------------------------------------------------------------
// ListDirectory
// ---------------------------------------------------------------------------

std::vector<omnisphere::models::DirectoryItem> File::ListDirectory(const omnisphere::dtos::ListDirectory& input) const
{
    try
    {
        return pimpl->repo.ReadDir(input);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("[File::ListDirectory] ") + e.what());
    }
}

// ---------------------------------------------------------------------------
// ValidateDirectoryPermissions
// ---------------------------------------------------------------------------

omnisphere::models::DirectoryPermissions File::ValidateDirectoryPermissions(const omnisphere::dtos::ValidateDirectoryPermissions& input) const
{
    try
    {
        return pimpl->repo.TestPermissions(input);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("[File::ValidateDirectoryPermissions] ") + e.what());
    }
}

// ---------------------------------------------------------------------------
// ConnectNetworkShare
// ---------------------------------------------------------------------------

std::string File::ConnectNetworkShare(const omnisphere::dtos::ConnectNetworkShare& input) const
{
    try
    {
        std::string err;
        std::string uri = pimpl->repo.MountShare(input, err);
        if (uri.empty())
            throw std::runtime_error(err);
        return uri;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("[File::ConnectNetworkShare] ") + e.what());
    }
}

// ---------------------------------------------------------------------------
// GetMountedShares
// ---------------------------------------------------------------------------

std::vector<std::string> File::GetMountedShares() const
{
    try
    {
        return pimpl->repo.GetMountedShares();
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("[File::GetMountedShares] ") + e.what());
    }
}

// ---------------------------------------------------------------------------
// CreateDirectory
// ---------------------------------------------------------------------------

std::string File::CreateDirectory(const omnisphere::dtos::CreateDirectory& input) const
{
    try
    {
        return pimpl->repo.MakeDir(input);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("[File::CreateDirectory] ") + e.what());
    }
}

// ---------------------------------------------------------------------------
// ReadFile
// ---------------------------------------------------------------------------

omnisphere::models::FileContent File::ReadFile(const omnisphere::dtos::ReadFile& input) const
{
    try
    {
        std::string dirPath  = input.Path.value_or("");
        std::string resolved = ResolvePath(dirPath);

        std::string fullPath;
        if (!resolved.empty() && fs::is_directory(resolved))
            fullPath = (fs::path(resolved) / input.FileName).string();
        else if (!resolved.empty())
            fullPath = resolved;
        else
            fullPath = input.FileName;

        auto bytes    = pimpl->repo.ReadBytes(fullPath);
        std::string mime    = DetectMimeType(input.FileName);
        std::string b64     = Base64Encode(bytes);
        std::string dataUrl = "data:" + mime + ";base64," + b64;

        omnisphere::models::FileContent result;
        result.FileName = input.FileName;
        result.FullPath = fullPath;
        result.DataUrl  = dataUrl;
        result.MimeType = mime;
        return result;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("[File::ReadFile] ") + e.what());
    }
}

// ---------------------------------------------------------------------------
// SaveFile
// ---------------------------------------------------------------------------

omnisphere::models::FileContent File::SaveFile(const omnisphere::dtos::SaveFile& input) const
{
    try
    {
        std::string dirPath  = input.Path.value_or("");
        std::string resolved = ResolvePath(dirPath);

        std::string fullPath;
        if (!resolved.empty() && fs::is_directory(resolved))
            fullPath = (fs::path(resolved) / input.FileName).string();
        else if (!resolved.empty())
            fullPath = resolved;
        else
            fullPath = input.FileName;

        // Strip "data:<mime>;base64," prefix if present
        std::string rawContent = input.Content;
        size_t commaPos = rawContent.find(',');
        if (commaPos != std::string::npos && rawContent.rfind("data:", 0) == 0)
            rawContent = rawContent.substr(commaPos + 1);

        auto bytes = Base64Decode(rawContent);
        pimpl->repo.WriteBytes(fullPath, bytes);

        omnisphere::models::FileContent result;
        result.FileName = input.FileName;
        result.FullPath = fullPath;
        return result;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("[File::SaveFile] ") + e.what());
    }
}

} // namespace omnisphere::services
