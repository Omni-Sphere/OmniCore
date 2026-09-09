#pragma once
#include "Payment/Repositories/StripeRepository.hpp"
#include <OmniData/DatabasePool.hpp>
#include <OmniUtils/Http/Router.hpp>
#include <functional>
#include <memory>
#include <string>

namespace omnisphere::services
{
    using StripePaymentHandler = std::function<void(
        const omnisphere::net::Request& req,
        const std::string& stripeSessionId,
        const std::string& reservationCode,
        const std::string& paymentIntentId,
        double amount
    )>;

    class StripeWebhookRouter
    {
    public:
        static void RegisterEndpoints(
            std::shared_ptr<omnisphere::net::Router> router,
            std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
            StripePaymentHandler paymentCompletedHandler = nullptr
        );
    };
}
