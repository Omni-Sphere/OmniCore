#include "Payment/UniversalPaymentWebhookHandler.hpp"
#include "Payment/Providers/PaymentProviderRegistry.hpp"
#include "Payment/Hooks/PaymentHook.hpp"
#include <OmniUtils/Logger.hpp>
#include <boost/json.hpp>
#include <iostream>

namespace omnisphere::payment
{
    UniversalPaymentWebhookHandler::UniversalPaymentWebhookHandler(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool))
    {
    }

    omnisphere::net::Response UniversalPaymentWebhookHandler::HandleWebhook(
        const std::string& providerCode,
        const omnisphere::net::Request& req
    ) const
    {
        omnisphere::utils::Logger::LogHttpRequest(req);

        auto provider = PaymentProviderRegistry::Instance().GetProvider(providerCode);
        if (!provider)
        {
            omnisphere::utils::Logger::LogError("UniversalPaymentWebhookHandler",
                req.TraceContext() + " No payment provider registered for code: [" + providerCode + "]");
            return omnisphere::net::Response(404, "application/json", R"({"error":"Payment provider not found"})");
        }

        // 1. Validar firma del webhook
        if (!provider->VerifyWebhookSignature(req))
        {
            omnisphere::utils::Logger::LogError("UniversalPaymentWebhookHandler",
                req.TraceContext() + " Signature verification failed for provider: [" + providerCode + "]");
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
        omnisphere::utils::Logger::LogInfo("UniversalPaymentWebhookHandler",
            req.TraceContext() + " Payment event [" + event.eventType + "] received for EntityType: [" + event.entityType +
            "], EntityCode: [" + event.entityCode + "], Provider: [" + providerCode + "], Amount: $" + std::to_string(event.amount));

        // 3. Despachar a los Hooks registrados en Core
        PaymentHookRegistry::Instance().DispatchCompleted(event);

        boost::json::object resObj;
        resObj["received"] = true;
        resObj["entityType"] = event.entityType;
        resObj["entityCode"] = event.entityCode;
        resObj["status"] = "DISPATCHED";

        return omnisphere::net::Response::Json(resObj);
    }

    omnisphere::net::Response UniversalPaymentWebhookHandler::HandleUniversalWebhook(const omnisphere::net::Request& req) const
    {
        std::string provider = req.QueryParam("provider");
        if (provider.empty()) provider = "STRIPE";
        return HandleWebhook(provider, req);
    }
} // namespace omnisphere::payment
