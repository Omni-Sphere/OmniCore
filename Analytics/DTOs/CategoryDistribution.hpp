#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct CategoryDistribution
    {
        std::string Code;
        std::string Label;
        int Count = 0;
        std::optional<double> TotalAmount;
        double Percentage = 0.0;
        std::optional<std::string> SubLabelLeft;
        std::optional<std::string> SubLabelRight;
        std::optional<std::string> Icon;
        std::optional<std::string> Color;
    };

    BOOST_DESCRIBE_STRUCT(CategoryDistribution, (), (
        Code,
        Label,
        Count,
        TotalAmount,
        Percentage,
        SubLabelLeft,
        SubLabelRight,
        Icon,
        Color
    ))
} // namespace omnisphere::dtos

