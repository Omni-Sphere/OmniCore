#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>
#include "Payment/Models/PaymentMethodDetail.hpp"

namespace omnisphere::models
{
    struct PaymentMethod
    {
        int entry = 0;
        std::string code;
        std::string name;
        std::string type = "NOT_APPLICABLE";       // CASH, TRANSFER, CARD, OTHER
        bool usesCommission = false;
        double commissionRate = 0.0;
        bool usesIntegration = false;
        std::optional<std::string> integrationProvider; // STRIPE, OPENPAY, MERCADOPAGO
        bool isActive = true;
        std::string createdBy = "SYSTEM";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;

        std::optional<PaymentMethodDetail> details;
    };

    BOOST_DESCRIBE_STRUCT(PaymentMethod, (), (
        entry, code, name, type, usesCommission, commissionRate,
        usesIntegration, integrationProvider,
        isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))
} // namespace omnisphere::models
