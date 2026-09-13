#pragma once
#include "Payment/Providers/IPaymentProvider.hpp"
#include <OmniData/DatabasePool.hpp>
#include <memory>

namespace omnisphere::payment
{
    class MercadoPagoPaymentProvider : public IPaymentProvider
    {
    public:
        explicit MercadoPagoPaymentProvider(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);

        std::string GetProviderCode() const override { return "MERCADOPAGO"; }
        std::string GetDisplayName() const override { return "Mercado Pago / Mercado Libre"; }

        ProviderPaymentIntentResult CreatePaymentIntent(const omnisphere::models::PayableEntity& entity) override;
        ProviderBankTransferResult CreateBankTransfer(const omnisphere::models::PayableEntity& entity) override;
        ProviderCheckoutResult CreateCheckoutSession(const omnisphere::models::PayableEntity& entity) override;

        ProviderDiagnosticResult TestIntegration() override;

        bool VerifyWebhookSignature(const omnisphere::net::Request& req) const override;
        std::optional<PaymentEvent> ParseWebhookEvent(const omnisphere::net::Request& req) const override;

    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
    };
} // namespace omnisphere::payment
