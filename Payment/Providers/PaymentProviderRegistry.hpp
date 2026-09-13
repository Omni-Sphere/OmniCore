#pragma once
#include "Payment/Providers/IPaymentProvider.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace omnisphere::payment
{
    class PaymentProviderRegistry
    {
    public:
        static PaymentProviderRegistry& Instance()
        {
            static PaymentProviderRegistry instance;
            return instance;
        }

        void RegisterProvider(std::shared_ptr<IPaymentProvider> provider);
        std::shared_ptr<IPaymentProvider> GetProvider(const std::string& providerCode) const;
        std::vector<std::string> GetAvailableProviders() const;
        bool HasProvider(const std::string& providerCode) const;
        void ClearProviders();

    private:
        PaymentProviderRegistry() = default;
        ~PaymentProviderRegistry() = default;
        PaymentProviderRegistry(const PaymentProviderRegistry&) = delete;
        PaymentProviderRegistry& operator=(const PaymentProviderRegistry&) = delete;

        mutable std::mutex m_mutex;
        std::unordered_map<std::string, std::shared_ptr<IPaymentProvider>> m_providers;
    };
} // namespace omnisphere::payment
