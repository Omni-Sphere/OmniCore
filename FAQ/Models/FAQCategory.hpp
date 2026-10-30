#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct FAQCategory
    {
        int entry = 0;
        std::string code;
        std::string name;
        std::string scope = "GLOBAL";
        std::string icon = "folder";
        int sortOrder = 0;
        bool isActive = true;
        std::string createdBy = "SYSTEM";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(FAQCategory, (), (
        entry, code, name, scope, icon, sortOrder, isActive,
        createdBy, createDate, lastUpdatedBy, updateDate
    ))
} // namespace omnisphere::models
