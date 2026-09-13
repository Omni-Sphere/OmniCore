#include "Payment/Providers/PaymentProviderRegistry.hpp"
#include <algorithm>

namespace omnisphere::payment
{
    static std::string ToUpper(std::string str)
    {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
            return std::toupper(c);
        });
        return str;
    }

    void PaymentProviderRegistry::RegisterProvider(std::shared_ptr<IPaymentProvider> provider)
    {
        if (!provider) return;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_providers[ToUpper(provider->GetProviderCode())] = provider;
    }

    std::shared_ptr<IPaymentProvider> PaymentProviderRegistry::GetProvider(const std::string& providerCode) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_providers.find(ToUpper(providerCode));
        if (it != m_providers.end())
        {
            return it->second;
        }
        return nullptr;
    }

    std::vector<std::string> PaymentProviderRegistry::GetAvailableProviders() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::string> keys;
        keys.reserve(m_providers.size());
        for (const auto& [key, _] : m_providers)
        {
            keys.push_back(key);
        }
        return keys;
    }

    bool PaymentProviderRegistry::HasProvider(const std::string& providerCode) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_providers.find(ToUpper(providerCode)) != m_providers.end();
    }

    void PaymentProviderRegistry::ClearProviders()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_providers.clear();
    }
} // namespace omnisphere::payment
