#include "File/FileHandler.hpp"
#include <OmniUtils/Logger.hpp>
#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <random>
#include <cerrno>
#include <cstring>

namespace fs = std::filesystem;

namespace omnisphere::services
{
    // Límite máximo de tamaño de carga: 5 MB
    constexpr static std::size_t MAX_UPLOAD_SIZE = 5 * 1024 * 1024;

    FileHandler::FileHandler(std::string uploadDir)
        : m_uploadDir(std::move(uploadDir))
    {
        std::error_code dirEc;
        if (!fs::exists(m_uploadDir, dirEc))
        {
            fs::create_directories(m_uploadDir, dirEc);
            if (dirEc)
            {
                omnisphere::utils::Logger::LogError("FileHandler", "Failed to create upload directory '" + m_uploadDir + "': " + dirEc.message());
            }
            else
            {
                omnisphere::utils::Logger::LogInfo("FileHandler", "Upload directory initialized: " + fs::absolute(m_uploadDir, dirEc).string());
            }
        }
    }

    bool FileHandler::IsAllowedImageExtension(const std::string& ext)
    {
        std::string lowerExt = ext;
        std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return (lowerExt == ".webp" || lowerExt == ".png" || lowerExt == ".jpg" || lowerExt == ".jpeg" || lowerExt == ".gif" || lowerExt == ".svg");
    }

    std::string FileHandler::DetectMimeType(const std::string& fileName)
    {
        std::string ext = fs::path(fileName).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
        if (ext == ".png")  return "image/png";
        if (ext == ".gif")  return "image/gif";
        if (ext == ".webp") return "image/webp";
        if (ext == ".svg")  return "image/svg+xml";
        return "application/octet-stream";
    }

    std::string FileHandler::GenerateSecureFilename(const std::string& extension)
    {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis;

        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        uint64_t randVal = dis(gen);
        char hexBuf[17];
        std::snprintf(hexBuf, sizeof(hexBuf), "%016lx", randVal);

        return "img_" + std::to_string(now) + "_" + std::string(hexBuf) + extension;
    }

    omnisphere::net::Response FileHandler::HandleUpload(const omnisphere::net::Request& req) const
    {
        // A. Verificación de Autenticación de Usuario (JWT Bearer Token)
        if (!req.IsAuthenticated())
        {
            omnisphere::utils::Logger::LogWarning("FileHandler Auth", req.TraceContext() + " Unauthorized upload attempt.");
            return omnisphere::net::Response(401, "application/json", R"({"error":"Unauthorized: Valid JWT Bearer token required for file uploads."})");
        }

        // B. Validación de Tamaño Máximo de Carga (Max File Size)
        std::string body = req.Body();
        if (body.empty())
        {
            return omnisphere::net::Response::BadRequest(R"({"error":"Empty file payload"})");
        }
        if (body.size() > MAX_UPLOAD_SIZE)
        {
            omnisphere::utils::Logger::LogWarning("FileHandler Security", req.TraceContext() + " File upload exceeded limit: " + std::to_string(body.size()) + " bytes");
            return omnisphere::net::Response(413, "application/json", R"({"error":"Payload too large: File exceeds maximum allowed size of 5 MB."})");
        }

        // C. Detección y Validación Estricta de Extensión (Whitelist)
        std::string providedName = req.Header("X-File-Name");
        if (providedName.empty()) providedName = req.QueryParam("fileName");

        std::string ext = fs::path(providedName).extension().string();
        if (ext.empty())
        {
            std::string contentType = req.Header("Content-Type");
            if (contentType.find("image/png") != std::string::npos) ext = ".png";
            else if (contentType.find("image/jpeg") != std::string::npos || contentType.find("image/jpg") != std::string::npos) ext = ".jpg";
            else if (contentType.find("image/gif") != std::string::npos) ext = ".gif";
            else ext = ".webp";
        }

        if (!IsAllowedImageExtension(ext))
        {
            omnisphere::utils::Logger::LogWarning("FileHandler Security", req.TraceContext() + " Rejected file extension: " + ext);
            return omnisphere::net::Response::BadRequest(R"({"error":"Forbidden file extension. Only .webp, .png, .jpg, .jpeg, .gif, .svg image files are allowed."})");
        }

        // D. Generación Segura e Impredecible del Nombre de Archivo
        std::string secureName = GenerateSecureFilename(ext);
        fs::path destinationPath = fs::path(m_uploadDir) / secureName;

        std::error_code writeEc;
        fs::create_directories(destinationPath.parent_path(), writeEc);
        if (writeEc)
        {
            omnisphere::utils::Logger::LogError("FileHandler", "create_directories failed for '" + destinationPath.parent_path().string() + "': " + writeEc.message());
        }

        int lastErrno = 0;
        std::ofstream outFile(destinationPath.string(), std::ios::binary);
        if (!outFile.is_open())
        {
            lastErrno = errno;
            std::string absPath = "";
            try { absPath = fs::absolute(destinationPath).string(); } catch (...) { absPath = destinationPath.string(); }
            std::string errStr = (lastErrno != 0) ? std::string(std::strerror(lastErrno)) : "Unknown I/O error";
            omnisphere::utils::Logger::LogError("FileHandler", "Failed to write file to: " + destinationPath.string() + " (absolute: " + absPath + ", reason: " + errStr + ")");
            return omnisphere::net::Response::InternalError(R"({"error":"Failed to save uploaded file"})");
        }

        outFile.write(body.data(), body.size());
        outFile.close();

        omnisphere::utils::Logger::LogInfo("FileHandler", req.TraceContext() + " Secure file uploaded: " + destinationPath.string() + " (" + std::to_string(body.size()) + " bytes)");

        boost::json::object resObj;
        resObj["success"] = true;
        resObj["fileName"] = secureName;
        resObj["url"] = "/uploads/" + secureName;
        return omnisphere::net::Response::Json(resObj);
    }

    omnisphere::net::Response FileHandler::HandleDownload(const omnisphere::net::Request& req, const std::string& prefix) const
    {
        std::string target = req.Target();
        size_t queryPos = target.find('?');
        if (queryPos != std::string::npos)
        {
            target = target.substr(0, queryPos);
        }

        std::string filename;
        if (target.rfind(prefix, 0) == 0)
        {
            filename = target.substr(prefix.length());
        }
        else
        {
            filename = target;
        }

        if (filename.empty() || filename.find("..") != std::string::npos || filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos)
        {
            omnisphere::utils::Logger::LogWarning("FileHandler Security", "Path traversal attack blocked: " + target);
            return omnisphere::net::Response::BadRequest(R"({"error":"Invalid file path"})");
        }

        std::string candidatePath = (fs::path(m_uploadDir) / filename).string();
        std::ifstream file(candidatePath, std::ios::binary);
        if (!file.is_open())
        {
            return omnisphere::net::Response::NotFound(R"({"error":"File not found"})");
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::string mime = DetectMimeType(filename);

        return omnisphere::net::Response(200, mime, content);
    }
} // namespace omnisphere::services
