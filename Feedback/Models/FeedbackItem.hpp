#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct FeedbackItem
    {
        int entry = 0;
        std::string code;
        std::string name;
        std::string scope;
        std::string type;
        std::string description;
        std::optional<std::string> metadata;
        std::optional<std::string> userName;
        std::optional<std::string> userEmail;
        std::optional<std::string> userPhone;
        bool isReviewed = false;
        std::optional<std::string> adminNotes;
        std::string createdBy = "system";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(FeedbackItem, (), (
        entry, code, name, scope, type, description, metadata,
        userName, userEmail, userPhone, isReviewed, adminNotes,
        createdBy, createDate, lastUpdatedBy, updateDate
    ))
} // namespace omnisphere::models
