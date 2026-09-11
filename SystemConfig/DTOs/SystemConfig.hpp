#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct GetSystemConfigFilter
    {
        std::optional<int> Entry;
        std::optional<std::string> Code;
    };

    BOOST_DESCRIBE_STRUCT(GetSystemConfigFilter, (), (
        Entry, Code
    ))

    struct UpdateSystemConfigInput
    {
        int Entry = 1;
        std::optional<std::string> Code;
        std::optional<std::string> FeeHandlingStrategy;
        std::optional<double> TaxRatePercent;
        std::optional<std::string> DefaultCurrency;
        std::optional<std::string> CompanyName;
        std::optional<bool> EnableEmailNotifications;
        std::optional<bool> EnableWhatsappNotifications;
        std::optional<bool> AllowPartialPayments;
        std::optional<bool> IsActive;
        int LastUpdatedBy = 1;
    };

    BOOST_DESCRIBE_STRUCT(UpdateSystemConfigInput, (), (
        Entry, Code, FeeHandlingStrategy, TaxRatePercent, DefaultCurrency, CompanyName,
        EnableEmailNotifications, EnableWhatsappNotifications, AllowPartialPayments,
        IsActive, LastUpdatedBy
    ))
} // namespace omnisphere::dtos
