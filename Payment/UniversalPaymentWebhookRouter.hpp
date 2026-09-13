#pragma once
#include <OmniData/DatabasePool.hpp>
#include <OmniUtils/Http/Router.hpp>
#include <memory>

namespace omnisphere::payment
{
    class UniversalPaymentWebhookRouter
    {
    public:
        static void RegisterEndpoints(
            std::shared_ptr<omnisphere::net::Router> router,
            std::shared_ptr<omnisphere::data::DatabasePool> dbPool
        );
    };
} // namespace omnisphere::payment
