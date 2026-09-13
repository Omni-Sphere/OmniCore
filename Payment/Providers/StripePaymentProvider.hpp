#pragma once
#include "Payment/Providers/IPaymentProvider.hpp"
#include "Payment/StripeService.hpp"
#include <OmniData/DatabasePool.hpp>
#include <memory>

namespace omnisphere::payment
{
    class StripePaymentProvider : public IPaymentProvider
    {
    public:
        explicit StripePaymentProvider(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        explicit StripePaymentProvider(std::shared_ptr<omnisphere::services::StripeService> stripeService, std::shared_ptr<omnisphere::data::DatabasePool> dbPool);

        std::string GetProviderCode() const override { return "STRIPE"; }
        std::string GetDisplayName() const override { return "Stripe Payments (Tarjeta / SPEI)"; }

        ProviderPaymentIntentResult CreatePaymentIntent(const omnisphere::models::PayableEntity& entity) override;
        ProviderBankTransferResult CreateBankTransfer(const omnisphere::models::PayableEntity& entity) override;
        ProviderCheckoutResult CreateCheckoutSession(const omnisphere::models::PayableEntity& entity) override;

        ProviderDiagnosticResult TestIntegration() override;

        bool CancelPayment(const std::string& transactionOrReferenceId, const std::string& reason = "abandoned") override;

        bool VerifyWebhookSignature(const omnisphere::net::Request& req) const override;
        std::optional<PaymentEvent> ParseWebhookEvent(const omnisphere::net::Request& req) const override;

    private:
        std::shared_ptr<omnisphere::services::StripeService> m_stripeService;
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
    };
} // namespace omnisphere::payment
