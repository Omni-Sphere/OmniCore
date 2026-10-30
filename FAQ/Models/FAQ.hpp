#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct FAQ
    {
        int entry = 0;
        std::string code;
        std::string name;
        std::string categoryCode;
        std::string scope = "GLOBAL";
        std::string question;
        std::string answer;
        int sortOrder = 0;
        bool isActive = true;
        std::string createdBy = "SYSTEM";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(FAQ, (), (
        entry, code, name, categoryCode, scope, question, answer,
        sortOrder, isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))
} // namespace omnisphere::models
