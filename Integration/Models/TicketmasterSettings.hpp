#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models
{
    struct TicketmasterSettings
    {
        int entry = 0;
        std::string code = "DEFAULT";
        std::string name = "Ticketmaster Discovery Settings";
        std::string apiKey;
        std::string apiSecret;
        std::string apiBaseUrl = "https://app.ticketmaster.com";
        std::string apiVersion = "v2";
        std::string defaultCountryCode = "MX";
        std::optional<std::string> defaultCity;
        std::optional<std::string> defaultClassification = "Music";
        bool autoCreateRoutes = false;
        std::string defaultOriginPointCode = "DEP1";
        int syncIntervalMinutes = 1440;
        bool isActive = true;
        std::optional<std::string> lastSyncDate;
        std::string createdBy = "SYSTEM";
        std::optional<std::string> createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(TicketmasterSettings, (), (
        entry,
        code,
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
        isActive,
        lastSyncDate,
        createdBy,
        createDate,
        lastUpdatedBy,
        updateDate
    ))
}
