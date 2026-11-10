#pragma once
#include <boost/describe.hpp>
#include <string>
#include <optional>

namespace omnisphere::models
{
    struct AppNotification
    {
        int entry = 0;
        std::string code;
        std::string typeCode;
        std::string title;
        std::string message;
        std::optional<std::string> referenceId;
        std::optional<std::string> metadataJson;
        std::string createdBy = "SYSTEM";
        std::optional<std::string> createDate;

        // Propiedad calculada/leída en el contexto del usuario actual
        bool isRead = false;
        std::optional<std::string> readDate;
    };

    BOOST_DESCRIBE_STRUCT(AppNotification, (), (
        entry,
        code,
        typeCode,
        title,
        message,
        referenceId,
        metadataJson,
        createdBy,
        createDate,
        isRead,
        readDate
    ))
} // namespace omnisphere::models

