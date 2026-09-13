#pragma once
#include <OmniUtils/Http/Request.hpp>
#include <OmniUtils/Http/Response.hpp>
#include <string>

namespace omnisphere::services
{
    class FileHandler
    {
    public:
        explicit FileHandler(std::string uploadDir = "./uploads");
        ~FileHandler() = default;

        // Procesa la carga segura de archivos con autenticación JWT, límite de 5MB y whitelist de extensiones
        omnisphere::net::Response HandleUpload(const omnisphere::net::Request& req) const;

        // Procesa la descarga/servido estático seguro previniendo Path Traversal (LFI)
        omnisphere::net::Response HandleDownload(const omnisphere::net::Request& req, const std::string& prefix) const;

    private:
        std::string m_uploadDir;

        static bool IsAllowedImageExtension(const std::string& ext);
        static std::string DetectMimeType(const std::string& fileName);
        static std::string GenerateSecureFilename(const std::string& extension);
    };
} // namespace omnisphere::services
