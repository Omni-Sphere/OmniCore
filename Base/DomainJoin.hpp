#pragma once
#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
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

    /**
     * @brief Unir dos colecciones de objetos en memoria con rendimiento O(N+M) usando HashMap
     * 
     * @tparam TSource Tipo del objeto fuente
     * @tparam TTarget Tipo del objeto destino
     * @tparam TKeyGetter1 Lambda o función para obtener la clave del objeto fuente
     * @tparam TKeyGetter2 Lambda o función para obtener la clave del objeto destino
     * @tparam TAssigner Lambda para transferir/asignar campos cuando la clave coincide (=)
     */
    template <typename TSource, typename TTarget, typename TKeyGetter1, typename TKeyGetter2, typename TAssigner>
    inline void JoinList(const std::vector<TSource>& sourceList, std::vector<TTarget>& targetList, TKeyGetter1 getKey1, TKeyGetter2 getKey2, TAssigner assigner)
    {
        std::unordered_map<std::string, const TSource*> sourceMap;
        for (const auto& item : sourceList)
        {
            std::string k = getKey1(item);
            if (!k.empty()) sourceMap[k] = &item;
        }

        for (auto& target : targetList)
        {
            std::string k = getKey2(target);
            if (!k.empty() && sourceMap.count(k) > 0)
            {
                assigner(target, *sourceMap[k]);
            }
        }
    }
} // namespace omnisphere::utils
