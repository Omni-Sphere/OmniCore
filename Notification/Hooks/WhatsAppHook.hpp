#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <boost/describe.hpp>
#include <OmniData/DatabasePool.hpp>

namespace omnisphere::notification
{
    struct InboundMessageEvent
    {
        std::string fromPhone;
        std::string customerName;
        std::string messageText;
        std::string buttonPayload;
        std::string buttonTitle;
        std::string messageType; // "text", "button", "interactive", "quick_reply", etc.
        std::string wamid;
        std::string traceContext;
        std::string rawPayload;
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool;
    };

    BOOST_DESCRIBE_STRUCT(InboundMessageEvent, (), (
        fromPhone,
        customerName,
        messageText,
        buttonPayload,
        buttonTitle,
        messageType,
        wamid,
        traceContext,
        rawPayload
    ))

    class IWhatsAppHook
    {
    public:
        virtual ~IWhatsAppHook() = default;

        // Domain identifier, e.g. "ROUTE", "ECOMMERCE", "SUPPORT" or "*" for all
        virtual std::string GetDomain() const = 0;

        // Predicate to check if this hook can handle the incoming message/interaction
        virtual bool CanHandle(const InboundMessageEvent& event) const = 0;

        // Process inbound message or button action. Return true if handled.
        virtual bool HandleInboundMessage(const InboundMessageEvent& event) = 0;
    };

    class WhatsAppHookRegistry
    {
    public:
        static WhatsAppHookRegistry& Instance()
        {
            static WhatsAppHookRegistry instance;
            return instance;
        }

        void RegisterHook(std::shared_ptr<IWhatsAppHook> hook)
        {
            if (!hook) return;
            std::lock_guard<std::mutex> lock(m_mutex);
            m_hooks.push_back(hook);
        }

        void ClearHooks()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_hooks.clear();
        }

        bool DispatchInboundMessage(const InboundMessageEvent& event)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            bool handled = false;
            for (const auto& hook : m_hooks)
            {
                if (hook && hook->CanHandle(event))
                {
                    try
                    {
                        if (hook->HandleInboundMessage(event))
                        {
                            handled = true;
                        }
                    }
                    catch (const std::exception& ex)
                    {
                        // Safely catch hook exceptions to prevent webhook crash
                    }
                }
            }
            return handled;
        }

    private:
        WhatsAppHookRegistry() = default;
        ~WhatsAppHookRegistry() = default;
        WhatsAppHookRegistry(const WhatsAppHookRegistry&) = delete;
        WhatsAppHookRegistry& operator=(const WhatsAppHookRegistry&) = delete;

        std::mutex m_mutex;
        std::vector<std::shared_ptr<IWhatsAppHook>> m_hooks;
    };
} // namespace omnisphere::notification
