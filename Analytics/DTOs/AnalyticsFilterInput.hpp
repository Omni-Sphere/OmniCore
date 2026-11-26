#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct AnalyticsFilterInput
    {
        std::optional<std::string> DateFrom;
        std::optional<std::string> DateTo;
        std::optional<std::string> Granularity; // HOUR, DAY, WEEK, MONTH
        std::optional<std::string> CompanyCode;
    };

    BOOST_DESCRIBE_STRUCT(AnalyticsFilterInput, (), (
        DateFrom,
        DateTo,
        Granularity,
        CompanyCode
    ))
} // namespace omnisphere::dtos

