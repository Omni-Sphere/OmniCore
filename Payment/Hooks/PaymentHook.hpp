#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <boost/json.hpp>
#include <boost/describe.hpp>

namespace omnisphere::payment
{
    struct PaymentEvent
    {
        std::string eventId;
        std::string eventType;          // "PAYMENT_COMPLETED", "PAYMENT_FAILED", "PAYMENT_EXPIRED", "TRANSFER_PENDING"
        std::string entityType;         // e.g. "ROUTE_RESERVATION", "CAFE_ORDER", "ERP_INVOICE"
        std::string entityCode;         // e.g. "RSV027", "ORD-101"
        std::string paymentIntentId;
        std::string sessionId;
        std::string provider;           // "STRIPE", "OPENPAY", "MERCADOPAGO"
        std::string paymentMethod;      // "CARD", "TRANSFER", "SPEI", "CASH"
        double amount = 0.0;
        std::string currency = "mxn";
        std::string customerPhone;
        std::string customerName;
        std::string customerEmail;
        boost::json::object metadata;
    };

    BOOST_DESCRIBE_STRUCT(PaymentEvent, (), (
        eventId,
        eventType,
        entityType,
        entityCode,
        paymentIntentId,
        sessionId,
        provider,
        paymentMethod,
        amount,
        currency,
        customerPhone,
        customerName,
        customerEmail
    ))

    class IPaymentHook
    {
    public:
        virtual ~IPaymentHook() = default;

        // Domain identifier, e.g. "ROUTE_RESERVATION", "CAFE_ORDER" or "*" for all
        virtual std::string GetTargetEntityType() const = 0;

        virtual bool OnPaymentCompleted(const PaymentEvent& event) = 0;
        virtual bool OnPaymentFailed(const PaymentEvent& event) = 0;
        virtual bool OnPaymentExpired(const PaymentEvent& event) = 0;
    };

    class PaymentHookRegistry
    {
    public:
        static PaymentHookRegistry& Instance()
        {
            static PaymentHookRegistry instance;
            return instance;
        }

        void RegisterHook(std::shared_ptr<IPaymentHook> hook)
        {
            if (!hook) return;
            std::lock_guard<std::mutex> lock(m_mutex);
            m_hooks[hook->GetTargetEntityType()].push_back(hook);
        }

        void ClearHooks()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_hooks.clear();
        }

        void DispatchCompleted(const PaymentEvent& event)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            DispatchToMatching(event.entityType, [&event](IPaymentHook* hook) {
                hook->OnPaymentCompleted(event);
            });
        }

        void DispatchFailed(const PaymentEvent& event)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            DispatchToMatching(event.entityType, [&event](IPaymentHook* hook) {
                hook->OnPaymentFailed(event);
            });
        }

        void DispatchExpired(const PaymentEvent& event)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            DispatchToMatching(event.entityType, [&event](IPaymentHook* hook) {
                hook->OnPaymentExpired(event);
            });
        }

    private:
        PaymentHookRegistry() = default;
        ~PaymentHookRegistry() = default;
        PaymentHookRegistry(const PaymentHookRegistry&) = delete;
        PaymentHookRegistry& operator=(const PaymentHookRegistry&) = delete;

        template<typename Func>
        void DispatchToMatching(const std::string& entityType, Func func)
        {
            // 1. Dispatch to specific entityType handlers
            auto it = m_hooks.find(entityType);
            if (it != m_hooks.end())
            {
                for (const auto& hook : it->second)
                {
                    if (hook) func(hook.get());
                }
            }

            // 2. Dispatch to wildcard "*" handlers
            auto itWild = m_hooks.find("*");
            if (itWild != m_hooks.end())
            {
                for (const auto& hook : itWild->second)
                {
                    if (hook) func(hook.get());
                }
            }
        }

        std::mutex m_mutex;
        std::unordered_map<std::string, std::vector<std::shared_ptr<IPaymentHook>>> m_hooks;
    };
} // namespace omnisphere::payment
