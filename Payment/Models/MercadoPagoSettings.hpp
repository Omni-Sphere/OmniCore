#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models
{
    struct MercadoPagoSettings
    {
        int entry = 0;
        std::string code = "DEFAULT";
        std::string name = "Mercado Pago Settings";
        std::string publicKey;
        std::string accessToken;
        std::string webhookSecretKey;
        std::string apiBaseUrl = "https://api.mercadopago.com";
        std::string webhookPath = "/api/v1/mercadopago/webhook";
        std::string currency = "mxn";
        bool isTestMode = true;
        bool isActive = true;
        std::string createdBy = "SYSTEM";
        std::optional<std::string> createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(MercadoPagoSettings, (), (
        entry,
        code,
        name,
        publicKey,
        accessToken,
        webhookSecretKey,
        apiBaseUrl,
        webhookPath,
        currency,
        isTestMode,
        isActive,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
} // namespace omnisphere::models
