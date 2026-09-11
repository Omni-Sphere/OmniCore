#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models
{
    struct StripeTransaction
    {
        int entry = 0;
        std::string code;
        std::string reservationCode;
        std::string stripePaymentIntentId;
        std::optional<std::string> stripeChargeId;
        double amount = 0.0;
        std::string currency = "mxn";
        std::string status = "succeeded";
        std::optional<std::string> cardBrand;
        std::optional<std::string> cardLast4;
        std::optional<std::string> cardExpMonthYear;
        std::optional<std::string> cardFunding;
        std::optional<std::string> authorizationCode;
        std::optional<std::string> cardFingerprint;
        std::optional<std::string> cvcCheck;
        std::optional<std::string> receiptUrl;
        std::optional<std::string> clientIp;
        std::optional<std::string> errorCode;
        std::optional<std::string> errorMessage;
        bool isActive = true;
        int createdBy = 1;
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(StripeTransaction, (), (
        entry,
        code,
        reservationCode,
        stripePaymentIntentId,
        stripeChargeId,
        amount,
        currency,
        status,
        cardBrand,
        cardLast4,
        cardExpMonthYear,
        cardFunding,
        authorizationCode,
        cardFingerprint,
        cvcCheck,
        receiptUrl,
        clientIp,
        errorCode,
        errorMessage,
        isActive,
        createdBy,
        createDate
    ))
}
