#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::dtos
{
    // =========================================================================
    // FAQ Category DTOs
    // =========================================================================
    struct CreateFAQCategoryInput
    {
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

    BOOST_DESCRIBE_STRUCT(CreateFAQCategoryInput, (), (
        code, name, scope, icon, sortOrder, isActive,
        createdBy, createDate, lastUpdatedBy, updateDate
    ))

    struct UpdateFAQCategoryInput
    {
        std::string code;
        std::optional<std::string> name;
        std::optional<std::string> scope;
        std::optional<std::string> icon;
        std::optional<int> sortOrder;
        std::optional<bool> isActive;
        std::optional<std::string> lastUpdatedBy;
        std::string updateDate;
    };

    BOOST_DESCRIBE_STRUCT(UpdateFAQCategoryInput, (), (
        code, name, scope, icon, sortOrder, isActive,
        lastUpdatedBy, updateDate
    ))

    // =========================================================================
    // FAQ DTOs
    // =========================================================================
    struct CreateFAQInput
    {
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

    BOOST_DESCRIBE_STRUCT(CreateFAQInput, (), (
        code, name, categoryCode, scope, question, answer,
        sortOrder, isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))

    struct UpdateFAQInput
    {
        std::string code;
        std::optional<std::string> name;
        std::optional<std::string> categoryCode;
        std::optional<std::string> scope;
        std::optional<std::string> question;
        std::optional<std::string> answer;
        std::optional<int> sortOrder;
        std::optional<bool> isActive;
        std::optional<std::string> lastUpdatedBy;
        std::string updateDate;
    };

    BOOST_DESCRIBE_STRUCT(UpdateFAQInput, (), (
        code, name, categoryCode, scope, question, answer,
        sortOrder, isActive, lastUpdatedBy, updateDate
    ))

    using CreateFAQ = CreateFAQInput;
    using UpdateFAQ = UpdateFAQInput;
    using CreateFAQCategory = CreateFAQCategoryInput;
    using UpdateFAQCategory = UpdateFAQCategoryInput;
} // namespace omnisphere::dtos
