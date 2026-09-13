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
        std::string paymentIntentId;
        std::optional<std::string> chargeId;
        double amount = 0.0;
        std::string currency = "mxn";
        std::string status = "succeeded";
        std::optional<std::string> paymentMethodType;
        std::optional<std::string> clabe;
        std::optional<std::string> bankName;
        std::optional<std::string> cardBrand;
        std::optional<std::string> cardLast4;
        std::optional<std::string> cardType;
        std::optional<std::string> authorizationCode;
        std::optional<std::string> cardFingerprint;
        std::optional<std::string> receiptUrl;
        std::optional<std::string> hostedInstructionsUrl;
        std::optional<std::string> clientIp;
        bool isActive = true;
        int createdBy = 1;
        std::optional<std::string> createDate;
        std::optional<int> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(StripeTransaction, (), (
        entry,
        code,
        reservationCode,
        paymentIntentId,
        chargeId,
        amount,
        currency,
        status,
        paymentMethodType,
        clabe,
        bankName,
        cardBrand,
        cardLast4,
        cardType,
        authorizationCode,
        cardFingerprint,
        receiptUrl,
        hostedInstructionsUrl,
        clientIp,
        isActive,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
}
