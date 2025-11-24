#pragma once
#include <string>

namespace omnicore::model
{
    class SessionData
    {
    public:
        std::string AccessToken;
        std::string ExpiresAt;
        std::string IssuedAt;
        std::string SID;
    };
}