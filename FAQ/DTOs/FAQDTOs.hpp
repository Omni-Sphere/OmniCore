#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    struct CreateFAQInput
    {
        std::string code;
        std::string name;
        std::string scope = "GLOBAL";
        std::string category = "General";
        std::string question;
        std::string answer;
        int sortOrder = 0;
        bool isActive = true;
        std::string createdBy = "system";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(CreateFAQInput, (), (
        code, name, scope, category, question, answer,
        sortOrder, isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))

    struct UpdateFAQInput
    {
        std::string code;
        std::optional<std::string> name;
        std::optional<std::string> scope;
        std::optional<std::string> category;
        std::optional<std::string> question;
        std::optional<std::string> answer;
        std::optional<int> sortOrder;
        std::optional<bool> isActive;
        std::optional<std::string> lastUpdatedBy;
        std::string updateDate;
    };

    BOOST_DESCRIBE_STRUCT(UpdateFAQInput, (), (
        code, name, scope, category, question, answer,
        sortOrder, isActive, lastUpdatedBy, updateDate
    ))

    // Alias retrocompatibles
    using CreateFAQ = CreateFAQInput;
    using UpdateFAQ = UpdateFAQInput;
} // namespace omnisphere::dtos
