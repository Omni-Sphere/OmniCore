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
        int createdBy = 0;
        std::string createDate;
        std::optional<int> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(PaymentMethodDetail, (), (
        entry, code, bankName, clabe, accountHolder, paymentReference,
        isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))
} // namespace omnisphere::models
