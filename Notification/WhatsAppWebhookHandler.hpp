#pragma once
#include <OmniData/DatabasePool.hpp>
#include <OmniUtils/Http/Request.hpp>
#include <OmniUtils/Http/Response.hpp>
#include <OmniUtils/DomainTaskDispatcher.hpp>
#include "Notification/Repositories/WhatsAppRepository.hpp"
#include <boost/json.hpp>
#include <functional>
#include <memory>
#include <string>

namespace omnisphere::services
{
    using InboundMessageHandler = std::function<void(
        const omnisphere::net::Request& req,
        const std::string& fromPhone,
        const std::string& customerName,
        const std::string& messageText
    )>;

    class WhatsAppWebhookHandler
    {
    public:
        explicit WhatsAppWebhookHandler(
            std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
            const std::string& verifyToken = "OMNI_WHATSAPP_VERIFY_TOKEN",
            InboundMessageHandler messageHandler = nullptr
        );
        ~WhatsAppWebhookHandler() = default;

        // 1. GET Endpoint: Meta Subscription Verification (hub.mode, hub.challenge, hub.verify_token)
        omnisphere::net::Response HandleVerification(const omnisphere::net::Request& req) const;

        // 2. POST Endpoint: Meta Webhook Event Delivery (Status Updates & Inbound Messages)
        omnisphere::net::Response HandleInboundEvent(const omnisphere::net::Request& req) const;

    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
        std::shared_ptr<omnisphere::repositories::WhatsAppRepository> m_repo;
        std::string m_verifyToken;
        InboundMessageHandler m_messageHandler;
        mutable omnisphere::utils::DomainTaskDispatcher m_metaWorker{"MetaWorkerThread"};

        bool IsTokenValid(const std::string& token) const;
        void ProcessStatuses(const boost::json::array& statuses, const std::string& traceCtx, const std::string& rawBody) const;
        void ProcessMessages(const boost::json::array& messages, const std::string& customerName, const omnisphere::net::Request& req) const;
        void ProcessTemplateStatusUpdate(const boost::json::object& changeObj, const boost::json::object& valueObj, const std::string& traceCtx) const;
        void ProcessEventAsync(omnisphere::net::Request req) const;
    };
} // namespace omnisphere::services
