#pragma once
#include <boost/describe.hpp>
#include <optional>
#include <string>

namespace omnisphere::models
{
    struct CustomMessageParameter
    {
        int entry = 0;
        std::string messageCode;
        std::string paramKey;
        std::string paramName;
        std::string dataType = "STRING"; // "STRING", "INTEGER", "CURRENCY", "TIME", "DATE", "MASKED_CARD"
        std::optional<std::string> defaultValue;
        bool isRequired = true;
        std::optional<std::string> description;
        int sortOrder = 1;
        int createdBy = 1;
        std::optional<std::string> createDate;
    };

    BOOST_DESCRIBE_STRUCT(CustomMessageParameter, (), (
        entry,
        messageCode,
        paramKey,
        paramName,
        dataType,
        defaultValue,
        isRequired,
        description,
        sortOrder,
        createdBy,
        createDate
    ))
}
