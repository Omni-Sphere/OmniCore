#pragma once
#include <string>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct IntegrationInfo
    {
        std::string code;
        std::string name;
        std::string displayName;
        std::string category;      // "PAYMENT", "MESSAGING"
        bool isActive = false;
        bool isConfigured = false;
        bool isTestMode = true;
        std::string message;
    };

    BOOST_DESCRIBE_STRUCT(IntegrationInfo, (), (
        code, name, displayName, category, isActive, isConfigured, isTestMode, message
    ))
} // namespace omnisphere::models
