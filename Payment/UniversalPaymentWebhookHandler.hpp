#pragma once
#include <OmniData/DatabasePool.hpp>
#include <OmniUtils/Http/Request.hpp>
#include <OmniUtils/Http/Response.hpp>
#include <memory>
#include <string>

namespace omnisphere::payment
{
    class UniversalPaymentWebhookHandler
    {
    public:
        explicit UniversalPaymentWebhookHandler(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~UniversalPaymentWebhookHandler() = default;

        // Procesa el webhook para un proveedor específico (STRIPE, OPENPAY, MERCADOPAGO)
        omnisphere::net::Response HandleWebhook(const std::string& providerCode, const omnisphere::net::Request& req) const;

        // Procesa el webhook universal extrayendo el proveedor de query param o body
        omnisphere::net::Response HandleUniversalWebhook(const omnisphere::net::Request& req) const;

    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
    };
} // namespace omnisphere::payment
