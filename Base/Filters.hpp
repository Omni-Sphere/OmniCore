#pragma once
#include <string>
#include <optional>

namespace omnisphere::dtos
{
    struct FloatFilter {
        std::optional<double> eq;
        std::optional<double> gt;
        std::optional<double> lt;
        std::optional<double> gte;
        std::optional<double> lte;
    };

    struct IntFilter {
        std::optional<int> eq;
        std::optional<int> gt;
        std::optional<int> lt;
        std::optional<int> gte;
        std::optional<int> lte;
    };

    struct StringFilter {
        std::optional<std::string> eq;
        std::optional<std::string> contains;
    };

    struct BooleanFilter {
        std::optional<bool> eq;
    };

} // namespace omnisphere::dtos
