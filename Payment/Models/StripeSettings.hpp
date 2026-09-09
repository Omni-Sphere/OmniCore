#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models
{
    struct StripeSettings
    {
        int entry = 0;
        std::string code = "DEFAULT";
        std::string name = "Stripe Payment Settings";
        std::string publishableKey;
        std::string secretKey;
        std::string webhookSecretKey;
        std::string apiBaseUrl = "https://api.stripe.com/v1";
        std::string checkoutEndpoint = "/checkout/sessions";
        std::string webhookPath = "/api/v1/stripe/webhook";
        std::string currency = "mxn";
        bool isTestMode = true;
        bool isActive = true;
        int createdBy = 1;
        std::optional<std::string> createDate;
        std::optional<int> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(StripeSettings, (), (
        entry,
        code,
        name,
        publishableKey,
        secretKey,
        webhookSecretKey,
        apiBaseUrl,
        checkoutEndpoint,
        webhookPath,
        currency,
        isTestMode,
        isActive,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
}
