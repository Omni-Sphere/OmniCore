#include "File/FileRouter.hpp"
#include "File/FileHandler.hpp"

namespace omnisphere::services
{
    void FileRouter::RegisterEndpoints(
        std::shared_ptr<omnisphere::net::Router> router,
        const std::string& uploadDir
    )
    {
        if (!router) return;

        // Instancia del controlador con RAII seguro
        auto handler = std::make_shared<FileHandler>(uploadDir);

        // 1. Endpoint POST Protegido por JWT para Carga de Archivos
        router->PostAuthorized("/api/v1/files/upload", [handler](const omnisphere::net::Request& req) {
            return handler->HandleUpload(req);
        });
        router->PostAuthorized("/uploads", [handler](const omnisphere::net::Request& req) {
            return handler->HandleUpload(req);
        });

        // 2. Handlers GET de Descarga/Servido Estático Seguro
        router->Get("/uploads/*", [handler](const omnisphere::net::Request& req) {
            return handler->HandleDownload(req, "/uploads/");
        });

        router->Get("/api/v1/files/*", [handler](const omnisphere::net::Request& req) {
            return handler->HandleDownload(req, "/api/v1/files/");
        });
    }
} // namespace omnisphere::services
