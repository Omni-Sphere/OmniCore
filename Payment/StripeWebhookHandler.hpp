#pragma once
#include <OmniData/DatabasePool.hpp>
#include <OmniUtils/Http/Request.hpp>
#include <OmniUtils/Http/Response.hpp>
#include "Payment/Repositories/StripeRepository.hpp"
#include <functional>
#include <memory>
#include <string>

namespace omnisphere::services
{
    using StripePaymentHandler = std::function<void(
        const omnisphere::net::Request& req,
        const std::string& stripeSessionId,
        const std::string& reservationCode,
        const std::string& paymentIntentId,
        double amount
    )>;

    class StripeWebhookHandler
    {
    public:
        explicit StripeWebhookHandler(
            std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
            StripePaymentHandler paymentCompletedHandler = nullptr
        );
        ~StripeWebhookHandler() = default;

        // POST Endpoint: Procesamiento seguro de eventos de Stripe con validación criptográfica
        omnisphere::net::Response HandleWebhook(const omnisphere::net::Request& req) const;

    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
        std::shared_ptr<omnisphere::repositories::StripeRepository> m_repo;
        StripePaymentHandler m_paymentCompletedHandler;

        bool VerifySignature(const omnisphere::net::Request& req, const std::string& webhookSecret) const;
        std::string RetrieveWebhookSecret() const;
    };
} // namespace omnisphere::services
