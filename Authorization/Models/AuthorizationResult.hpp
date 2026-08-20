#pragma once

#include <string>

namespace omnisphere::models
{
    struct AuthorizationResult
    {
        bool success = false;
        std::string message;
        std::string userCode;
        std::string permission;
    };
} // namespace omnisphere::models
