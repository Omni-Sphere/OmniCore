#include "Payment/UniversalPaymentWebhookRouter.hpp"
#include "Payment/UniversalPaymentWebhookHandler.hpp"
#include "Payment/Repositories/StripeRepository.hpp"
#include "Payment/Repositories/MercadoPagoRepository.hpp"
#include <unordered_set>

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

        // 1. Stripe Endpoints (Ruta configurable desde StripeSettings)
        std::string stripeConfiguredPath = "/api/v1/stripe/webhook";
        try
        {
            auto stripeRepo = std::make_shared<omnisphere::repositories::StripeRepository>(dbPool);
            auto dt = stripeRepo->GetSettings({"WebhookPath"});
            if (!dt.IsEmpty())
            {
                std::string p = dt[0]["WebhookPath"].GetOptional<std::string>().value_or("");
                if (!p.empty()) stripeConfiguredPath = p;
            }
        }
        catch (...) {}

        std::unordered_set<std::string> stripePaths = { stripeConfiguredPath, "/api/v1/stripe/webhook", "/stripe/webhook" };
        for (const auto& path : stripePaths)
        {
            router->Post(path, [handler](const omnisphere::net::Request& req) {
                return handler->HandleWebhook("STRIPE", req);
            });
        }

        // 2. Mercado Pago Endpoints (Ruta configurable desde MercadoPagoSettings)
        std::string mpConfiguredPath = "/api/v1/mercadopago/webhook";
        try
        {
            auto mpRepo = std::make_shared<omnisphere::repositories::MercadoPagoRepository>(dbPool);
            auto dt = mpRepo->GetSettings({"WebhookPath"});
            if (!dt.IsEmpty())
            {
                std::string p = dt[0]["WebhookPath"].GetOptional<std::string>().value_or("");
                if (!p.empty()) mpConfiguredPath = p;
            }
        }
        catch (...) {}

        std::unordered_set<std::string> mpPaths = { mpConfiguredPath, "/api/v1/mercadopago/webhook", "/mercadopago/webhook" };
        for (const auto& path : mpPaths)
        {
            router->Post(path, [handler](const omnisphere::net::Request& req) {
                return handler->HandleWebhook("MERCADOPAGO", req);
            });
        }

        // 3. OpenPay Endpoints
        router->Post("/api/v1/openpay/webhook", [handler](const omnisphere::net::Request& req) {
            return handler->HandleWebhook("OPENPAY", req);
        });

        // 4. Universal Provider Endpoint
        router->Post("/api/v1/payments/webhook", [handler](const omnisphere::net::Request& req) {
            return handler->HandleUniversalWebhook(req);
        });
    }
} // namespace omnisphere::payment
