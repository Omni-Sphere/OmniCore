#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct PaymentMethodDetail
    {
        int entry = 0;
        std::string code;
        std::string bankName;
        std::string clabe;
        std::string accountHolder;
        std::optional<std::string> paymentReference;
        bool isActive = true;
        std::string createdBy = "SYSTEM";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(PaymentMethodDetail, (), (
        entry, code, bankName, clabe, accountHolder, paymentReference,
        isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))
} // namespace omnisphere::models
