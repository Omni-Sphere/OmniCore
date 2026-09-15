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

        // 1. Validar firma del webhook síncronamente
        if (!provider->VerifyWebhookSignature(req))
        {
            omnisphere::utils::Logger::LogError("UniversalPaymentWebhookHandler",
                req.TraceContext() + " Signature verification failed for provider: [" + providerCode + "]");
            return omnisphere::net::Response(400, "application/json", R"({"error":"Invalid webhook signature"})");
        }

        omnisphere::utils::Logger::LogInfo("UniversalPaymentWebhookHandler",
            req.TraceContext() + " Enqueueing Universal Webhook Event to [UniversalPaymentWorkerThread]...");

        // 2. Despachar asíncronamente al hilo dedicado de Pagos Universales
        m_universalWorker.Enqueue([this, providerCode, req]() {
            ProcessWebhookAsync(providerCode, req);
        });

        boost::json::object resObj;
        resObj["received"] = true;
        resObj["provider"] = providerCode;
        resObj["status"] = "DISPATCHED";

        return omnisphere::net::Response::Json(resObj);
    }

    void UniversalPaymentWebhookHandler::ProcessWebhookAsync(std::string providerCode, omnisphere::net::Request req) const
    {
        try
        {
            auto provider = PaymentProviderRegistry::Instance().GetProvider(providerCode);
            if (!provider) return;

            auto eventOpt = provider->ParseWebhookEvent(req);
            if (!eventOpt.has_value()) return;

            const auto& event = eventOpt.value();
            omnisphere::utils::Logger::LogInfo("UniversalPaymentWebhookHandler",
                req.TraceContext() + " [UniversalPaymentWorkerThread] Payment event [" + event.eventType + "] received for EntityType: [" + event.entityType +
                "], EntityCode: [" + event.entityCode + "], Provider: [" + providerCode + "], Amount: $" + std::to_string(event.amount));

            if (event.eventType == "PAYMENT_COMPLETED")
            {
                PaymentHookRegistry::Instance().DispatchCompleted(event);
            }
            else if (event.eventType == "PAYMENT_FAILED")
            {
                PaymentHookRegistry::Instance().DispatchFailed(event);
            }
            else if (event.eventType == "PAYMENT_EXPIRED")
            {
                PaymentHookRegistry::Instance().DispatchExpired(event);
            }
        }
        catch (const std::exception& ex)
        {
            omnisphere::utils::Logger::LogError("UniversalPaymentWebhookHandler",
                req.TraceContext() + " [UniversalPaymentWorkerThread] Exception processing webhook: " + ex.what());
            std::cerr << "[UniversalPaymentWebhookHandler Error - UniversalPaymentWorkerThread] " << ex.what() << std::endl;
        }
    }

    omnisphere::net::Response UniversalPaymentWebhookHandler::HandleUniversalWebhook(const omnisphere::net::Request& req) const
    {
        std::string provider = req.QueryParam("provider");
        if (provider.empty()) provider = "STRIPE";
        return HandleWebhook(provider, req);
    }
} // namespace omnisphere::payment
