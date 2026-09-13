#pragma once
#include "Payment/Models/PayableEntity.hpp"
#include "Payment/Hooks/PaymentHook.hpp"
#include <OmniUtils/Http/Request.hpp>
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::payment
{
    struct ProviderCheckoutResult
    {
        bool success = false;
        std::string checkoutUrl;
        std::string sessionId;
        std::string errorMessage;
    };

    BOOST_DESCRIBE_STRUCT(ProviderCheckoutResult, (), (
        success, checkoutUrl, sessionId, errorMessage
    ))

    struct ProviderPaymentIntentResult
    {
        bool success = false;
        std::string clientSecret;
        std::string publishableKey;
        std::string paymentIntentId;
        std::string errorMessage;
    };

    BOOST_DESCRIBE_STRUCT(ProviderPaymentIntentResult, (), (
        success, clientSecret, publishableKey, paymentIntentId, errorMessage
    ))

    struct ProviderBankTransferResult
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

    BOOST_DESCRIBE_STRUCT(ProviderBankTransferResult, (), (
        success, paymentIntentId, clabe, bankName, hostedInstructionsUrl, amount, currency, errorMessage
    ))

    struct ProviderDiagnosticResult
    {
        bool isConfigured = false;
        bool isFunctional = false;
        std::string message;
    };

    BOOST_DESCRIBE_STRUCT(ProviderDiagnosticResult, (), (
        isConfigured, isFunctional, message
    ))

    class IPaymentProvider
    {
    public:
        virtual ~IPaymentProvider() = default;

        // Unique identifier code: "STRIPE", "OPENPAY", "MERCADOPAGO", etc.
        virtual std::string GetProviderCode() const = 0;
        virtual std::string GetDisplayName() const = 0;

        virtual ProviderPaymentIntentResult CreatePaymentIntent(const omnisphere::models::PayableEntity& entity) = 0;
        virtual ProviderBankTransferResult CreateBankTransfer(const omnisphere::models::PayableEntity& entity) = 0;
        virtual ProviderCheckoutResult CreateCheckoutSession(const omnisphere::models::PayableEntity& entity) = 0;

        virtual ProviderDiagnosticResult TestIntegration() = 0;

        virtual bool VerifyWebhookSignature(const omnisphere::net::Request& req) const = 0;
        virtual std::optional<PaymentEvent> ParseWebhookEvent(const omnisphere::net::Request& req) const = 0;
    };
} // namespace omnisphere::payment
