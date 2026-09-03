#pragma once
#include <string>
#include <optional>

namespace omnisphere::dtos
{
    struct WhatsAppConfig
    {
        std::string token;
        std::string phoneId;
        std::optional<std::string> appSecret;
        std::optional<std::string> webhookVerifyToken;
        std::string apiVersion = "v24.0";
    };
}
