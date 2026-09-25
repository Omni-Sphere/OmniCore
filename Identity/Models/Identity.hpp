#pragma once
#include <string>
#include <optional>
#include <boost/describe.hpp>

namespace omnisphere::models
{
    struct Identity
    {
        int entry = 0;
        std::string domain;
        std::string prefix1;
        std::optional<std::string> prefix2;
        std::optional<std::string> prefix3;
        int currentSequence = 0;
        bool isActive = true;
        std::string createdBy = "SYSTEM";
        std::string createDate;
        std::optional<std::string> lastUpdatedBy;
        std::optional<std::string> updateDate;
    };

    BOOST_DESCRIBE_STRUCT(Identity, (), (
        entry, domain, prefix1, prefix2, prefix3, currentSequence,
        isActive, createdBy, createDate, lastUpdatedBy, updateDate
    ))
} // namespace omnisphere::models
