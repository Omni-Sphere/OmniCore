#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct MetricSummary
    {
        std::string Code;
        std::string Title;
        double Value = 0.0;
        std::string FormattedValue;
        std::optional<double> PreviousValue;
        std::optional<double> ChangePercent;
        std::string Trend = "NEUTRAL"; // UP, DOWN, NEUTRAL
        std::optional<std::string> HelperText;
        std::optional<std::string> Icon;
        std::optional<std::string> Color;
    };

    BOOST_DESCRIBE_STRUCT(MetricSummary, (), (
        Code,
        Title,
        Value,
        FormattedValue,
        PreviousValue,
        ChangePercent,
        Trend,
        HelperText,
        Icon,
        Color
    ))
} // namespace omnisphere::dtos

