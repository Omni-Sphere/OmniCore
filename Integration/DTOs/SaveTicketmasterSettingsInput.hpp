#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::dtos
{
    struct SaveTicketmasterSettingsInput
    {
        std::optional<std::string> name;
        std::optional<std::string> apiKey;
        std::optional<std::string> apiSecret;
        std::optional<std::string> apiBaseUrl;
        std::optional<std::string> apiVersion;
        std::optional<std::string> defaultCountryCode;
        std::optional<std::string> defaultCity;
        std::optional<std::string> defaultClassification;
        std::optional<bool> autoCreateRoutes;
        std::optional<std::string> defaultOriginPointCode;
        std::optional<int> syncIntervalMinutes;
        std::optional<bool> isActive;
    };

    BOOST_DESCRIBE_STRUCT(SaveTicketmasterSettingsInput, (), (
        name,
        apiKey,
        apiSecret,
        apiBaseUrl,
        apiVersion,
        defaultCountryCode,
        defaultCity,
        defaultClassification,
        autoCreateRoutes,
        defaultOriginPointCode,
        syncIntervalMinutes,
        isActive
    ))
}
