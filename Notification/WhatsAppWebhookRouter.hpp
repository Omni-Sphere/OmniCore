#pragma once
#include "Notification/Repositories/WhatsAppRepository.hpp"
#include <OmniData/DatabasePool.hpp>
#include <OmniUtils/Http/Router.hpp>
#include <memory>
#include <string>

namespace omnisphere::services
{
    class WhatsAppWebhookRouter
    {
    public:
        static void RegisterEndpoints(
            std::shared_ptr<omnisphere::net::Router> router,
            std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
            const std::string& verifyToken = "OMNI_WHATSAPP_VERIFY_TOKEN",
            const std::string& path = "/api/v1/whatsapp/webhook"
        );
    };
}
