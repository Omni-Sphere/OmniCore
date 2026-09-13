#include "Payment/UniversalPaymentWebhookRouter.hpp"
#include "Payment/UniversalPaymentWebhookHandler.hpp"

namespace omnisphere::payment
{
    void UniversalPaymentWebhookRouter::RegisterEndpoints(
        std::shared_ptr<omnisphere::net::Router> router,
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool
    )
    {
        if (!router || !dbPool) return;

        // Instancia del controlador con RAII seguro
        auto handler = std::make_shared<UniversalPaymentWebhookHandler>(dbPool);

        // 1. Stripe Endpoints
        router->Post("/api/v1/stripe/webhook", [handler](const omnisphere::net::Request& req) {
            return handler->HandleWebhook("STRIPE", req);
        });

        // 2. OpenPay Endpoints
        router->Post("/api/v1/openpay/webhook", [handler](const omnisphere::net::Request& req) {
            return handler->HandleWebhook("OPENPAY", req);
        });

        // 3. Mercado Pago Endpoints
        router->Post("/api/v1/mercadopago/webhook", [handler](const omnisphere::net::Request& req) {
            return handler->HandleWebhook("MERCADOPAGO", req);
        });

        // 4. Universal Provider Endpoint
        router->Post("/api/v1/payments/webhook", [handler](const omnisphere::net::Request& req) {
            return handler->HandleUniversalWebhook(req);
        });
    }
} // namespace omnisphere::payment
