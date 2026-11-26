#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct TimeSeriesPoint
    {
        std::string Label;
        std::optional<std::string> Timestamp;
        double PrimaryValue = 0.0;
        std::optional<double> SecondaryValue;
        std::optional<std::string> DisplayValue;
        std::optional<std::string> SecondaryLabel;
        std::optional<std::string> Tooltip;
        bool Highlight = false;
        std::optional<std::string> Color;
    };

    BOOST_DESCRIBE_STRUCT(TimeSeriesPoint, (), (
        Label,
        Timestamp,
        PrimaryValue,
        SecondaryValue,
        DisplayValue,
        SecondaryLabel,
        Tooltip,
        Highlight,
        Color
    ))
} // namespace omnisphere::dtos

