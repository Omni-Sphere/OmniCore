#pragma once
#include "Payment/Repositories/StripeRepository.hpp"
#include "Authorization/Models/SecurityContext.hpp"
#include <OmniData/DatabasePool.hpp>
#include <memory>
#include <string>
#include <optional>

namespace omnisphere::services
{
    struct StripeCheckoutResult
    {
        bool success = false;
        std::string checkoutUrl;
        std::string sessionId;
        std::string errorMessage;
    };

    struct StripePaymentIntentResult
    {
        bool success = false;
        std::string clientSecret;
        std::string publishableKey;
        std::string paymentIntentId;
        std::string errorMessage;
    };

    struct StripeTestIntegrationResult
    {
        bool isConfigured = false;
        bool isFunctional = false;
        std::string message;
    };

    struct StripeBankTransferResult
    {
        bool success = false;
        std::string paymentIntentId;
        std::string clabe;
        std::string bankName;
        std::string hostedInstructionsUrl;
        double amount = 0.0;
        std::string currency = "mxn";
        std::string errorMessage;
    };

    class StripeService
    {
    public:
        explicit StripeService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        explicit StripeService(std::shared_ptr<omnisphere::repositories::StripeRepository> repository);

        std::optional<omnisphere::models::StripeSettings> GetSettings(bool decryptKeys = false) const;
        bool SaveSettings(const omnisphere::models::StripeSettings& settings) const;

        StripeCheckoutResult CreateCheckoutSession(
            const omnisphere::models::SecurityContext& ctx,
            const std::string& reservationCode,
            double amount,
            int seats = 1,
            const std::string& successUrl = "",
            const std::string& cancelUrl = ""
        ) const;

        StripePaymentIntentResult CreatePaymentIntent(
            const omnisphere::models::SecurityContext& ctx,
            const std::string& reservationCode,
            double amount,
            const std::string& currency = "mxn"
        ) const;

        StripeBankTransferResult CreateBankTransferPaymentIntent(
            const omnisphere::models::SecurityContext& ctx,
            const std::string& reservationCode,
            double amount,
            const std::string& customerName = "",
            const std::string& customerEmail = ""
        ) const;

        StripeTestIntegrationResult TestIntegration(
            const omnisphere::models::SecurityContext& ctx
        ) const;

    private:
        std::shared_ptr<omnisphere::repositories::StripeRepository> m_repository;
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
    };
}
