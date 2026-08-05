#include <File/Repositories/File.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unordered_map>
#include <mutex>

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

namespace omnisphere::repositories
{

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

std::string File::ToLower(const std::string& s)
{
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return std::tolower(c); });
    return r;
}

std::string File::EscapeShellArg(const std::string& arg)
{
    std::string escaped = "'";
    for (char c : arg)
    {
        if (c == '\'') escaped += "'\\''";
        else           escaped += c;
    }
    escaped += "'";
    return escaped;
}

std::string File::GetUserHomeDirectory()
{
#ifdef _WIN32
    const char* up = std::getenv("USERPROFILE");
    if (up && *up) return std::string(up);
    const char* hd = std::getenv("HOMEDRIVE");
    const char* hp = std::getenv("HOMEPATH");
    if (hd && hp) return std::string(hd) + std::string(hp);
    return "C:\\";
#else
    const char* home = std::getenv("HOME");
    if (home && *home) return std::string(home);
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir && *(pw->pw_dir)) return std::string(pw->pw_dir);
    return "/home";
#endif
}



std::vector<std::string> File::GioList(const std::string& rawUri)
{
    std::vector<std::string> results;
    if (rawUri.empty()) return results;

    std::string uri = rawUri;
    if (uri.find(' ') != std::string::npos)
    {
        std::string encoded = "";
        for (char c : uri)
        {
            if (c == ' ') encoded += "%20";
            else          encoded += c;
        }
        uri = encoded;
    }

    std::string cmd = "gio list " + EscapeShellArg(uri) + " 2>/dev/null";
    FILE* pipe = ::popen(cmd.c_str(), "r");
    if (!pipe) return results;

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL)
    {
        std::string line(buffer);
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
            line.pop_back();
        while (!line.empty() && line.front() == ' ')
            line.erase(line.begin());
        if (!line.empty() && line[0] != '.')
            results.push_back(line);
    }
    ::pclose(pipe);
    return results;
}

bool File::IsNetworkUri(const std::string& path)
{
    std::string lower = ToLower(path);
    return lower.rfind("smb://", 0) == 0 ||
           lower.rfind("nfs://", 0) == 0 ||
           lower.rfind("\\\\", 0) == 0 ||
           lower.find("/gvfs/") != std::string::npos;
}

std::string File::GetNetworkParentPath(const std::string& path)
{
    if (path.empty()) return "";
    std::string clean = path;
    while (clean.length() > 1 && (clean.back() == '/' || clean.back() == '\\'))
        clean.pop_back();

    if (clean.rfind("smb://", 0) == 0)
    {
        std::string sub = clean.substr(6);
        size_t lastSlash = sub.find_last_of('/');
        if (lastSlash == std::string::npos) return "";
        return "smb://" + sub.substr(0, lastSlash);
    }
    if (clean.rfind("nfs://", 0) == 0)
    {
        std::string sub = clean.substr(6);
        size_t lastSlash = sub.find_last_of('/');
        if (lastSlash == std::string::npos) return "";
        return "nfs://" + sub.substr(0, lastSlash);
    }
    if (clean.rfind("\\\\", 0) == 0)
    {
        std::string sub = clean.substr(2);
        size_t lastSlash = sub.find_last_of("\\/");
        if (lastSlash == std::string::npos) return "";
        return "\\\\" + sub.substr(0, lastSlash);
    }

    fs::path p(clean);
    return p.has_parent_path() ? p.parent_path().string() : "";
}

// ---------------------------------------------------------------------------
// ResolvePath: translate network URIs -> local GVFS/mount paths
// ---------------------------------------------------------------------------

std::string File::ResolvePath(const std::string& rawPath)
{
    if (rawPath.empty()) return "";

    std::string path = rawPath;
    if (fs::exists(path) && fs::is_directory(path))
        return path;

    if (path.rfind("smb:", 0) == 0 || path.rfind("nfs:", 0) == 0)
        std::replace(path.begin(), path.end(), '\\', '/');

    if (path.rfind("smb://", 0) == 0)
    {
        std::string smbSub = path.substr(6);
        size_t atPos = smbSub.find('@');
        if (atPos != std::string::npos) smbSub = smbSub.substr(atPos + 1);

        size_t slash1 = smbSub.find('/');
        std::string server = (slash1 != std::string::npos) ? smbSub.substr(0, slash1) : smbSub;
        std::string rest   = (slash1 != std::string::npos) ? smbSub.substr(slash1 + 1) : "";
        size_t slash2 = rest.find('/');
        std::string share   = (slash2 != std::string::npos) ? rest.substr(0, slash2) : rest;
        std::string subPath = (slash2 != std::string::npos) ? rest.substr(slash2) : "";

#ifndef _WIN32
        uid_t uid = getuid();
        std::string gvfsDir = "/run/user/" + std::to_string(uid) + "/gvfs";
        if (fs::exists(gvfsDir))
        {
            std::string lowerServer = ToLower(server);
            std::string lowerShare  = ToLower(share);
            std::error_code ec;

            if (lowerServer.empty()) return gvfsDir;

            std::vector<fs::path> matchingServerEntries;

            for (const auto& entry : fs::directory_iterator(gvfsDir, ec))
            {
                std::string dirName = entry.path().filename().string();
                std::string lowerDir = ToLower(dirName);

                bool serverMatches = (lowerDir.find("server=" + lowerServer) != std::string::npos || lowerDir.find(lowerServer) != std::string::npos);

                if (serverMatches)
                {
                    matchingServerEntries.push_back(entry.path());

                    if (!lowerShare.empty())
                    {
                        bool shareMatches = (lowerDir.find("share=" + lowerShare) != std::string::npos || lowerDir.find(lowerShare) != std::string::npos);
                        if (shareMatches)
                        {
                            std::string target = entry.path().string() + (subPath.empty() || subPath[0] == '/' ? subPath : "/" + subPath);
                            std::error_code checkEc;
                            if (subPath.empty() || fs::exists(target, checkEc)) return target;
                        }
                    }
                }
            }

            // Pass 2: If share didn't match share= parameter, check if share is a subfolder inside ANY matching server mount
            if (!lowerShare.empty() && !matchingServerEntries.empty())
            {
                std::string relPath = share + (subPath.empty() || subPath[0] == '/' ? subPath : "/" + subPath);
                for (const auto& sPath : matchingServerEntries)
                {
                    std::string cand = sPath.string() + "/" + relPath;
                    std::error_code checkEc;
                    if (fs::exists(cand, checkEc)) return cand;
                }
            }

            // Pass 3: If lowerShare was empty, do NOT resolve to a specific share mount.
            if (lowerShare.empty())
            {
                return "";
            }
        }
        if (!share.empty())
        {
            for (auto&& p : {"/mnt/" + server + "/" + share + subPath, "/mnt/" + share + subPath, "/media/" + share + subPath})
                if (fs::exists(p)) return p;
        }
        else
        {
            std::string mp = "/mnt/" + server;
            if (fs::exists(mp)) return mp;
        }
#endif
    }

    if (path.rfind("nfs://", 0) == 0)
    {
        std::string sub = path.substr(6);
        size_t slash1 = sub.find('/');
        std::string server = (slash1 != std::string::npos) ? sub.substr(0, slash1) : sub;
        std::string exportPath = (slash1 != std::string::npos) ? sub.substr(slash1) : "";
        if (!exportPath.empty())
        {
            for (auto&& p : {"/mnt/" + server + exportPath, "/mnt" + exportPath, "/net/" + server + exportPath})
                if (fs::exists(p)) return p;
        }
        else
        {
            std::string mp = "/mnt/" + server;
            if (fs::exists(mp)) return mp;
        }
    }

    if (path.rfind("\\\\", 0) == 0)
    {
        if (fs::exists(path)) return path;
        std::string uncSub = path.substr(2);
        std::replace(uncSub.begin(), uncSub.end(), '\\', '/');
        std::string resolved = ResolvePath("smb://" + uncSub);
        if (resolved != "smb://" + uncSub) return resolved;
    }

    return "";
}

std::string File::AutoMountNetworkUri(const std::string& uri, const std::string& inputUser, const std::string& inputPass, const std::string& inputDom) const
{
    if (uri.empty() || !IsNetworkUri(uri)) return "";

    std::string lower = ToLower(uri);
    if (lower.rfind("smb://", 0) != 0 && lower.rfind("nfs://", 0) != 0) return "";

    std::string protocol = lower.rfind("nfs://", 0) == 0 ? "nfs" : "smb";
    std::string raw = uri.substr(6);

    std::string username = inputUser;
    std::string password = inputPass;
    std::string domain   = inputDom;

    size_t atPos = raw.find('@');
    if (atPos != std::string::npos)
    {
        std::string userPart = raw.substr(0, atPos);
        raw = raw.substr(atPos + 1);
        size_t colonPos = userPart.find(':');
        if (colonPos != std::string::npos)
        {
            username = userPart.substr(0, colonPos);
            if (password.empty()) password = userPart.substr(colonPos + 1);
        }
        else
        {
            username = userPart;
        }
    }

    size_t slash = raw.find('/');
    std::string server = (slash != std::string::npos) ? raw.substr(0, slash) : raw;
    std::string share  = (slash != std::string::npos) ? raw.substr(slash + 1) : "";
    size_t slash2 = share.find('/');
    if (slash2 != std::string::npos) share = share.substr(0, slash2);

    omnisphere::dtos::ConnectNetworkShare dto;
    dto.Protocol = protocol;
    dto.Server   = server;
    if (!share.empty()) dto.Share = share;
    if (!username.empty()) dto.Username = username;
    if (!password.empty()) dto.Password = password;
    if (!domain.empty())   dto.Domain   = domain;

    std::string dummyErr;
    MountShare(dto, dummyErr);

    return ResolvePath(uri);
}

// ---------------------------------------------------------------------------
// ReadDir
// ---------------------------------------------------------------------------

std::vector<omnisphere::models::DirectoryItem> File::ReadDir(const omnisphere::dtos::ListDirectory& input) const
{
    std::vector<omnisphere::models::DirectoryItem> items;

    std::string requestedPath = input.Path.value_or("");
    std::string resolvedPath  = ResolvePath(requestedPath);

    bool isNet = IsNetworkUri(requestedPath) || IsNetworkUri(resolvedPath);

    if (isNet && resolvedPath.empty() && !requestedPath.empty() && requestedPath != "smb://" && requestedPath != "nfs://")
    {
        std::string u = input.Username.value_or("");
        std::string p = input.Password.value_or("");
        std::string d = input.Domain.value_or("");
        resolvedPath = AutoMountNetworkUri(requestedPath, u, p, d);
    }

    fs::path targetPath;
    if (resolvedPath.empty())
    {
        if (isNet)
        {
            auto gioItems = GioList(requestedPath);
            if (!gioItems.empty())
            {
                std::string baseUri = requestedPath;
                while (!baseUri.empty() && (baseUri.back() == '/' || baseUri.back() == '\\'))
                    baseUri.pop_back();

                for (const auto& name : gioItems)
                {
                    omnisphere::models::DirectoryItem item;
                    item.Name = name;
                    item.Path = baseUri + "/" + name;
                    item.IsDirectory = true;
                    items.push_back(item);
                }
            }
            return items;
        }
        targetPath = fs::path(GetUserHomeDirectory());
    }
    else
    {
        targetPath = fs::path(resolvedPath);
    }

    std::error_code ec;
    bool exists = fs::exists(targetPath, ec);

    if (exists && !ec)
    {
        if (!fs::is_directory(targetPath, ec)) targetPath = targetPath.parent_path();

        std::error_code dirEc;
        fs::directory_iterator iter(targetPath, fs::directory_options::skip_permission_denied, dirEc);
        if (!dirEc)
        {
            for (const auto& entry : iter)
            {
                std::string filename = entry.path().filename().string();
                if (filename.empty() || filename[0] == '.') continue;

                bool isDir = true;
                if (!isNet && targetPath.string().find("/gvfs") == std::string::npos)
                {
                    std::error_code statusEc;
                    isDir = entry.is_directory(statusEc);
                    if (statusEc)
                    {
                        statusEc.clear();
                        isDir = !fs::is_regular_file(entry.path(), statusEc);
                    }
                    if (!isDir) continue;
                }
                else
                {
                    std::error_code regEc;
                    if (fs::is_regular_file(entry.path(), regEc)) continue;
                }

                omnisphere::models::DirectoryItem item;

                if (targetPath.string().find("/gvfs") != std::string::npos && filename.rfind("smb-share:", 0) == 0)
                {
                    size_t sPos = filename.find("server=");
                    size_t shPos = filename.find("share=");
                    std::string serverName = (sPos != std::string::npos) ? filename.substr(sPos + 7, filename.find_first_of(",/", sPos) - (sPos + 7)) : "";
                    std::string shareName  = (shPos != std::string::npos) ? filename.substr(shPos + 6, filename.find_first_of(",/", shPos) - (shPos + 6)) : "";

                    if (requestedPath == "smb://" || requestedPath == "smb:" || requestedPath.empty())
                    {
                        item.Name = serverName;
                        item.Path = "smb://" + serverName;
                    }
                    else
                    {
                        item.Name = serverName + (shareName.empty() ? "" : " / " + shareName);
                        item.Path = "smb://" + serverName + (shareName.empty() ? "" : "/" + shareName);
                    }
                    item.IsDirectory = true;
                }
                else
                {
                    item.Name = filename;
                    item.IsDirectory = true;

                    if (isNet)
                    {
                        std::string baseUri = requestedPath;
                        while (!baseUri.empty() && (baseUri.back() == '/' || baseUri.back() == '\\'))
                            baseUri.pop_back();
                        item.Path = baseUri + "/" + filename;
                    }
                    else
                    {
                        item.Path = entry.path().string();
                    }
                }

                items.push_back(item);
            }
        }
    }

    if (isNet && items.empty() && !requestedPath.empty())
    {
        auto gioItems = GioList(requestedPath);
        if (!gioItems.empty())
        {
            std::string baseUri = requestedPath;
            while (!baseUri.empty() && (baseUri.back() == '/' || baseUri.back() == '\\'))
                baseUri.pop_back();

            for (const auto& name : gioItems)
            {
                omnisphere::models::DirectoryItem item;
                item.Name = name;
                item.Path = baseUri + "/" + name;
                item.IsDirectory = true;
                items.push_back(item);
            }
        }
    }

    return items;
}

// ---------------------------------------------------------------------------
// TestPermissions
// ---------------------------------------------------------------------------

omnisphere::models::DirectoryPermissions File::TestPermissions(const omnisphere::dtos::ValidateDirectoryPermissions& input) const
{
    omnisphere::models::DirectoryPermissions result;
    result.Path          = input.Path;
    result.HasReadAccess = false;
    result.HasWriteAccess = false;

    std::string resolved = ResolvePath(input.Path);
    if (resolved.empty()) return result;

    fs::path dirPath(resolved);
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) return result;

    // Test read: iterate at least one entry
    std::error_code ec;
    fs::directory_iterator it(dirPath, fs::directory_options::skip_permission_denied, ec);
    result.HasReadAccess = (!ec);

    // Test write: create and delete a 0B probe file
    fs::path probeFile = dirPath / ".omni_perm_check.tmp";
    try
    {
        {
            std::ofstream ofs(probeFile.string(), std::ios::trunc | std::ios::binary);
            result.HasWriteAccess = ofs.good();
        }
        if (result.HasWriteAccess)
            fs::remove(probeFile, ec);
    }
    catch (...)
    {
        result.HasWriteAccess = false;
    }

    return result;
}

// ---------------------------------------------------------------------------
// MountShare
// ---------------------------------------------------------------------------

static std::string UrlEncode(const std::string& str)
{
    std::string encoded;
    for (char c : str)
    {
        if (std::isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~')
            encoded += c;
        else
        {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            encoded += buf;
        }
    }
    return encoded;
}

std::string File::MountShare(const omnisphere::dtos::ConnectNetworkShare& input, std::string& outError) const
{
    std::string protocol = input.Protocol.empty() ? "smb" : ToLower(input.Protocol);
    std::string server   = input.Server;
    std::string share    = input.Share.value_or("");
    std::string username = input.Username.value_or("");
    std::string password = input.Password.value_or("");
    std::string domain   = input.Domain.value_or("");

    // Sanitize server prefix
    for (auto& pfx : {"smb://", "nfs://", "\\\\"})
        if (server.rfind(pfx, 0) == 0) { server = server.substr(std::strlen(pfx)); break; }
    std::replace(server.begin(), server.end(), '\\', '/');

    size_t sSlash = server.find('/');
    if (sSlash != std::string::npos)
    {
        if (share.empty()) share = server.substr(sSlash + 1);
        server = server.substr(0, sSlash);
    }
    while (!share.empty() && (share[0] == '/' || share[0] == '\\')) share = share.substr(1);
    while (!share.empty() && (share.back() == '/' || share.back() == '\\')) share.pop_back();



    std::string encUser = UrlEncode(username);
    std::string encPass = UrlEncode(password);
    std::string encDom  = UrlEncode(domain);

    std::string userPart = "";
    if (!encDom.empty() && !encUser.empty())
        userPart = encDom + ";" + encUser;
    else if (!encUser.empty())
        userPart = encUser;

    std::string authUriPart = "";
    if (!userPart.empty())
    {
        if (!encPass.empty())
            authUriPart = userPart + ":" + encPass + "@";
        else
            authUriPart = userPart + "@";
    }

    std::string gioTarget;
    if (protocol == "nfs")
        gioTarget = "nfs://" + server + (share.empty() ? "" : "/" + share);
    else
        gioTarget = "smb://" + authUriPart + server + (share.empty() ? "" : "/" + share);

    // Construct clean display URI (no password)
    std::string displayUri;
    if (protocol == "nfs")
        displayUri = gioTarget;
    else
        displayUri = "smb://" + (username.empty() ? "" : username + "@") + server + (share.empty() ? "" : "/" + share);

#ifndef _WIN32
    std::string cmd;
    if (protocol == "smb" && !password.empty())
    {
        std::string domVal = domain.empty() ? "WORKGROUP" : domain;
        cmd = "printf '%s\\n%s\\n' " + EscapeShellArg(domVal) + " " + EscapeShellArg(password) + " | timeout 10s gio mount " + EscapeShellArg(gioTarget) + " 2>&1";
    }
    else if (username.empty() && protocol == "smb")
    {
        cmd = "timeout 10s gio mount -a " + EscapeShellArg(gioTarget) + " 2>&1";
    }
    else
    {
        cmd = "timeout 10s gio mount " + EscapeShellArg(gioTarget) + " 2>&1";
    }

    std::string mountOutput;
    FILE* pipe = ::popen(cmd.c_str(), "r");
    int exitStatus = -1;
    if (pipe)
    {
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), pipe) != NULL)
            mountOutput += buffer;
        exitStatus = ::pclose(pipe);
    }

    std::string resolvedPath = ResolvePath(displayUri);

    bool alreadyMounted = (mountOutput.find("already mounted") != std::string::npos ||
                           mountOutput.find("ya está montada") != std::string::npos ||
                           mountOutput.find("Location is already mounted") != std::string::npos ||
                           mountOutput.find("Ubicación ya montada") != std::string::npos);

    if (WIFEXITED(exitStatus) && WEXITSTATUS(exitStatus) != 0 && !alreadyMounted && (resolvedPath.empty() || !fs::exists(resolvedPath)))
    {
        while (!mountOutput.empty() && (mountOutput.back() == '\r' || mountOutput.back() == '\n' || mountOutput.back() == ' '))
            mountOutput.pop_back();
        outError = mountOutput.empty() ? "Failed to authenticate or connect to network share." : mountOutput;
        return "";
    }
#else
    if (protocol == "smb")
    {
        std::string uncPath = "\\\\" + server + (share.empty() ? "" : "\\" + share);
        std::string winCmd = "net use \"" + uncPath + "\"";
        if (!username.empty())
        {
            std::string userDomain = domain.empty() ? username : (domain + "\\" + username);
            winCmd += " \"" + password + "\" /user:\"" + userDomain + "\"";
        }
        winCmd += " >nul 2>&1";
        ::system(winCmd.c_str());
    }
    std::string resolvedPath = ResolvePath(displayUri);
#endif

    return displayUri;
}

// ---------------------------------------------------------------------------
// MakeDir
// ---------------------------------------------------------------------------

std::string File::MakeDir(const omnisphere::dtos::CreateDirectory& input) const
{
    if (input.FolderName.empty())
        throw std::invalid_argument("Folder name cannot be empty");

    std::string resolved = ResolvePath(input.ParentPath);
    fs::path parentPath  = resolved.empty() ? fs::path(GetUserHomeDirectory()) : fs::path(resolved);

    if (!fs::exists(parentPath))
        throw std::runtime_error("Parent directory does not exist or network share is not mounted");

    fs::path newFolder = parentPath / input.FolderName;
    std::error_code ec;
    if (!fs::create_directory(newFolder, ec) && !fs::exists(newFolder))
        throw std::runtime_error(ec ? ec.message() : "Failed to create directory");

    return newFolder.string();
}

// ---------------------------------------------------------------------------
// ReadBytes / WriteBytes
// ---------------------------------------------------------------------------

std::vector<unsigned char> File::ReadBytes(const std::string& fullPath) const
{
    std::ifstream ifs(fullPath, std::ios::binary);
    if (!ifs.is_open())
        throw std::runtime_error("Cannot open file for reading: " + fullPath);

    return std::vector<unsigned char>((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

void File::WriteBytes(const std::string& fullPath, const std::vector<unsigned char>& data) const
{
    std::ofstream ofs(fullPath, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open())
        throw std::runtime_error("Cannot open file for writing: " + fullPath);

    ofs.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

} // namespace omnisphere::repositories
