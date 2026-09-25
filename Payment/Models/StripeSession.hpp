#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models
{
    struct StripeSession
    {
        int entry = 0;
        std::string code;
        std::string reservationCode;
        std::string stripeSessionId;
        std::optional<std::string> paymentIntentId;
        std::string checkoutUrl;
        double amount = 0.0;
        std::string currency = "mxn";
        std::string status = "open";
        bool isActive = true;
        std::string createdBy = "SYSTEM";
        std::optional<std::string> createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(StripeSession, (), (
        entry,
        code,
        reservationCode,
        stripeSessionId,
        paymentIntentId,
        checkoutUrl,
        amount,
        currency,
        status,
        isActive,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
}
