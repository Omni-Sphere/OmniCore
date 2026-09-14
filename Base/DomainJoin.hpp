#pragma once
#include <string>
#include <optional>
#include <type_traits>
#include <cstdint>

namespace omnisphere::utils
{
    /**
     * @brief Helper genérico para unir/relacionar propiedades entre entidades por igualdad (=)
     * 
     * Compara sourceKey con targetKey. Si coinciden por igualdad (=) y no son nulos/vacíos,
     * asigna sourceVal a targetVal y retorna true.
     */
    template <typename TKey, typename TValue>
    inline bool Join(const TKey& sourceKey, const TKey& targetKey, const TValue& sourceVal, TValue& targetVal)
    {
        if constexpr (std::is_same_v<TKey, std::string>)
        {
            if (!sourceKey.empty() && sourceKey == targetKey)
            {
                targetVal = sourceVal;
                return true;
            }
        }
        else if constexpr (std::is_same_v<TKey, int> || std::is_same_v<TKey, int64_t>)
        {
            if (sourceKey > 0 && sourceKey == targetKey)
            {
                targetVal = sourceVal;
                return true;
            }
        }
        else
        {
            if (sourceKey == targetKey)
            {
                targetVal = sourceVal;
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Sobrecargas de Join para manejo transparente de std::optional
     */
    template <typename TKey, typename TValue>
    inline bool Join(const std::optional<TKey>& sourceKey, const TKey& targetKey, const TValue& sourceVal, TValue& targetVal)
    {
        if (sourceKey.has_value())
        {
            return Join(sourceKey.value(), targetKey, sourceVal, targetVal);
        }
        return false;
    }

    template <typename TKey, typename TValue>
    inline bool Join(const TKey& sourceKey, const std::optional<TKey>& targetKey, const TValue& sourceVal, TValue& targetVal)
    {
        if (targetKey.has_value())
        {
            return Join(sourceKey, targetKey.value(), sourceVal, targetVal);
        }
        return false;
    }

    template <typename TKey, typename TValue>
    inline bool Join(const TKey& sourceKey, const TKey& targetKey, const std::optional<TValue>& sourceVal, TValue& targetVal)
    {
        if (sourceVal.has_value())
        {
            return Join(sourceKey, targetKey, sourceVal.value(), targetVal);
        }
        return false;
    }
} // namespace omnisphere::utils
