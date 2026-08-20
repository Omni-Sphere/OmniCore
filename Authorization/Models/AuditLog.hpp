#pragma once

#include <string>

namespace omnisphere::models
{
    struct AuditLogEntry
    {
        std::string userCode;
        std::string grantedByCode;
        std::string module;
        std::string permission;
        std::string resourceCode;
        std::string status; // "GRANTED" / "DENIED"
        std::string reason;
    };
} // namespace omnisphere::models
