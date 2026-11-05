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
    // Límite máximo de tamaño de carga: 25 MB (soporte para imágenes, PDFs, audio y video de WhatsApp)
    constexpr static std::size_t MAX_UPLOAD_SIZE = 25 * 1024 * 1024;

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
        return (
            lowerExt == ".webp" || lowerExt == ".png" || lowerExt == ".jpg" || lowerExt == ".jpeg" ||
            lowerExt == ".gif"  || lowerExt == ".svg" || lowerExt == ".pdf" || lowerExt == ".doc"  ||
            lowerExt == ".docx" || lowerExt == ".xls" || lowerExt == ".xlsx"|| lowerExt == ".mp4"  ||
            lowerExt == ".mov"  || lowerExt == ".mp3" || lowerExt == ".ogg" || lowerExt == ".aac"  ||
            lowerExt == ".m4a"  || lowerExt == ".amr"
        );
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
        if (ext == ".pdf")  return "application/pdf";
        if (ext == ".mp4")  return "video/mp4";
        if (ext == ".mov")  return "video/quicktime";
        if (ext == ".mp3")  return "audio/mpeg";
        if (ext == ".ogg")  return "audio/ogg";
        if (ext == ".aac")  return "audio/aac";
        if (ext == ".m4a")  return "audio/mp4";
        if (ext == ".amr")  return "audio/amr";
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

        std::string lowerExt = extension;
        std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::string prefix = "file_";
        if (lowerExt == ".jpg" || lowerExt == ".jpeg" || lowerExt == ".png" || lowerExt == ".webp" || lowerExt == ".gif" || lowerExt == ".svg")
            prefix = "img_";
        else if (lowerExt == ".pdf" || lowerExt == ".doc" || lowerExt == ".docx")
            prefix = "doc_";
        else if (lowerExt == ".mp4" || lowerExt == ".mov")
            prefix = "vid_";
        else if (lowerExt == ".mp3" || lowerExt == ".ogg" || lowerExt == ".aac" || lowerExt == ".m4a")
            prefix = "aud_";

        return prefix + std::to_string(now) + "_" + std::string(hexBuf) + lowerExt;
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
            return omnisphere::net::Response(413, "application/json", R"({"error":"Payload too large: File exceeds maximum allowed size of 25 MB."})");
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
            else if (contentType.find("application/pdf") != std::string::npos) ext = ".pdf";
            else if (contentType.find("video/mp4") != std::string::npos) ext = ".mp4";
            else if (contentType.find("audio/mpeg") != std::string::npos) ext = ".mp3";
            else if (contentType.find("audio/ogg") != std::string::npos) ext = ".ogg";
            else ext = ".webp";
        }

        if (!IsAllowedImageExtension(ext))
        {
            omnisphere::utils::Logger::LogWarning("FileHandler Security", req.TraceContext() + " Rejected file extension: " + ext);
            return omnisphere::net::Response::BadRequest(R"({"error":"Formato de archivo no permitido. Se permiten imágenes (.jpg, .png, .webp), documentos (.pdf), videos (.mp4) y audio (.mp3, .ogg)."})");
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

        if (filename.empty() || filename.find("..") != std::string::npos || filename.find('\\') != std::string::npos)
        {
            omnisphere::utils::Logger::LogWarning("FileHandler Security", "Path traversal attack blocked: " + target);
            return omnisphere::net::Response::BadRequest(R"({"error":"Invalid file path"})");
        }

        // Eliminar slash inicial si existe
        if (!filename.empty() && filename.front() == '/')
        {
            filename.erase(0, 1);
        }

        fs::path baseUploadDir = fs::absolute(m_uploadDir);
        fs::path candidatePath = fs::weakly_canonical(baseUploadDir / filename);

        // Validar que la ruta canónica permanezca dentro del directorio base de subidas
        std::string baseStr = baseUploadDir.string();
        std::string candStr = candidatePath.string();
        if (candStr.rfind(baseStr, 0) != 0)
        {
            omnisphere::utils::Logger::LogWarning("FileHandler Security", "Path traversal attack blocked: " + target);
            return omnisphere::net::Response::BadRequest(R"({"error":"Invalid file path"})");
        }

        std::ifstream file(candStr, std::ios::binary);
        if (!file.is_open())
        {
            return omnisphere::net::Response::NotFound(R"({"error":"File not found"})");
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::string mime = DetectMimeType(filename);

        return omnisphere::net::Response(200, mime, content);
    }
} // namespace omnisphere::services
