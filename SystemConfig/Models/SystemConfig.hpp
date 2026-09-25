#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct SystemConfig
    {
        int entry = 1;
        std::string code = "DEFAULT";
        std::string feeHandlingStrategy = "SURCHARGE";
        double taxRatePercent = 16.0;
        std::string defaultCurrency = "MXN";
        std::string companyName = "OmniRoute Express";
        bool enableEmailNotifications = true;
        bool enableWhatsappNotifications = true;
        bool allowPartialPayments = false;
        bool isActive = true;
        std::string createdBy = "SYSTEM";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(SystemConfig, (), (
        entry, code, feeHandlingStrategy, taxRatePercent,
        defaultCurrency, companyName, enableEmailNotifications,
        enableWhatsappNotifications, allowPartialPayments,
        isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))
} // namespace omnisphere::models
