#include "Payment/UniversalPaymentWebhookRouter.hpp"
#include "Payment/Providers/PaymentProviderRegistry.hpp"
#include "Payment/Hooks/PaymentHook.hpp"
#include <OmniUtils/Logger.hpp>
#include <iostream>

namespace omnisphere::payment
{
    void UniversalPaymentWebhookRouter::RegisterEndpoints(
        std::shared_ptr<omnisphere::net::Router> router,
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool
    )
    {
        if (!router || !dbPool) return;

        auto handleWebhookForProvider = [](const std::string& providerCode, const omnisphere::net::Request& req) -> omnisphere::net::Response {
            omnisphere::utils::Logger::LogHttpRequest(req);

            auto provider = PaymentProviderRegistry::Instance().GetProvider(providerCode);
            if (!provider)
            {
                omnisphere::utils::Logger::LogError("UniversalPaymentWebhook", "No payment provider registered for code: [" + providerCode + "]");
                return omnisphere::net::Response(404, "application/json", R"({"error":"Payment provider not found"})");
            }

            // 1. Validar firma del webhook
            if (!provider->VerifyWebhookSignature(req))
            {
                omnisphere::utils::Logger::LogError("UniversalPaymentWebhook", "Signature verification failed for provider: [" + providerCode + "]");
                return omnisphere::net::Response(400, "application/json", R"({"error":"Invalid webhook signature"})");
            }

            // 2. Parsear evento
            auto eventOpt = provider->ParseWebhookEvent(req);
            if (!eventOpt.has_value())
            {
                // Evento no procesable o informativo (ej. payment_intent.created)
                return omnisphere::net::Response::Json(boost::json::object{{"received", true}, {"processed", false}});
            }

            const auto& event = eventOpt.value();
            omnisphere::utils::Logger::LogInfo("UniversalPaymentWebhook",
                "Payment event [" + event.eventType + "] received for EntityType: [" + event.entityType +
                "], EntityCode: [" + event.entityCode + "], Provider: [" + providerCode + "], Amount: $" + std::to_string(event.amount));

            // 3. Despachar a los Hooks registrados en Core
            PaymentHookRegistry::Instance().DispatchCompleted(event);

            boost::json::object resObj;
            resObj["received"] = true;
            resObj["entityType"] = event.entityType;
            resObj["entityCode"] = event.entityCode;
            resObj["status"] = "DISPATCHED";

            return omnisphere::net::Response::Json(resObj);
        };

        // 1. Stripe Endpoints
        router->Post("/api/v1/stripe/webhook", [handleWebhookForProvider](const omnisphere::net::Request& req) {
            return handleWebhookForProvider("STRIPE", req);
        });

        // 2. OpenPay Endpoints
        router->Post("/api/v1/openpay/webhook", [handleWebhookForProvider](const omnisphere::net::Request& req) {
            return handleWebhookForProvider("OPENPAY", req);
        });

        // 3. Mercado Pago Endpoints
        router->Post("/api/v1/mercadopago/webhook", [handleWebhookForProvider](const omnisphere::net::Request& req) {
            return handleWebhookForProvider("MERCADOPAGO", req);
        });

        // 4. Universal Provider Endpoint
        router->Post("/api/v1/payments/webhook", [handleWebhookForProvider](const omnisphere::net::Request& req) {
            std::string provider = req.QueryParam("provider");
            if (provider.empty()) provider = "STRIPE";
            return handleWebhookForProvider(provider, req);
        });
    }
} // namespace omnisphere::payment
