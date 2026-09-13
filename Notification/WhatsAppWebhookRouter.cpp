#include "Notification/WhatsAppWebhookRouter.hpp"
#include "Notification/WhatsAppWebhookHandler.hpp"

namespace omnisphere::services
{
    void WhatsAppWebhookRouter::RegisterEndpoints(
        std::shared_ptr<omnisphere::net::Router> router,
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
        const std::string& verifyToken,
        const std::string& path,
        InboundMessageHandler messageHandler
    )
    {
        if (!router || !dbPool) return;

        // Instancia controladora con gestión RAII segura
        auto handler = std::make_shared<WhatsAppWebhookHandler>(dbPool, verifyToken, messageHandler);

        // Alias paths para matching flexible
        std::vector<std::string> pathsToRegister = { path };
        if (path != "/webhook") pathsToRegister.push_back("/webhook");
        if (path != "/whatsapp/webhook") pathsToRegister.push_back("/whatsapp/webhook");
        if (path != "/api/v1/whatsapp/webhook") pathsToRegister.push_back("/api/v1/whatsapp/webhook");

        for (const auto& p : pathsToRegister)
        {
            // 1. GET: Verificación de suscripción de Meta
            router->Get(p, [handler](const omnisphere::net::Request& req) -> omnisphere::net::Response {
                return handler->HandleVerification(req);
            });

            // 2. POST: Recepción de eventos de mensajes y cambios de estatus
            router->Post(p, [handler](const omnisphere::net::Request& req) -> omnisphere::net::Response {
                return handler->HandleInboundEvent(req);
            });
        }
    }
} // namespace omnisphere::services
