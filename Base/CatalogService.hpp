#pragma once
#include "Base/CatalogRepository.hpp"
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace omnisphere::services
{
    /**
     * @brief Servicio agnóstico y genérico para gestión de categorías y catálogos de cualquier dominio.
     * Ofrece operaciones de lectura, escritura, actualización y eliminación de forma uniforme.
     */
    template <typename TModel, typename TCreateDTO, typename TUpdateDTO>
    class CatalogService
    {
    private:
        std::shared_ptr<omnisphere::repositories::CatalogRepository<TModel, TCreateDTO, TUpdateDTO>> m_repo;

    public:
        explicit CatalogService(
            std::shared_ptr<omnisphere::repositories::CatalogRepository<TModel, TCreateDTO, TUpdateDTO>> repo
        ) : m_repo(std::move(repo)) {}

        ~CatalogService() = default;

        std::shared_ptr<omnisphere::repositories::CatalogRepository<TModel, TCreateDTO, TUpdateDTO>> GetRepository() const
        {
            return m_repo;
        }

        std::optional<TModel> Create(
            TCreateDTO input,
            const std::vector<std::string>& mutationFields = {},
            const std::vector<std::string>& requestedFields = {}
        )
        {
            if (!m_repo) return std::nullopt;
            if (!m_repo->Create(input, mutationFields))
            {
                return std::nullopt;
            }
            return m_repo->GetByCode(input.code, requestedFields);
        }

        std::optional<TModel> GetByCode(
            const std::string& code,
            const std::vector<std::string>& requestedFields = {}
        )
        {
            if (!m_repo) return std::nullopt;
            return m_repo->GetByCode(code, requestedFields);
        }

        std::vector<TModel> GetItems(
            const std::optional<std::string>& scope = std::nullopt,
            const std::optional<bool>& onlyActive = std::nullopt,
            const std::vector<std::string>& requestedFields = {}
        )
        {
            if (!m_repo) return {};
            return m_repo->GetAll(scope, onlyActive, requestedFields);
        }

        std::optional<TModel> Update(
            const TUpdateDTO& input,
            const std::vector<std::string>& mutationFields = {},
            const std::vector<std::string>& requestedFields = {}
        )
        {
            if (!m_repo) return std::nullopt;
            if (!m_repo->Update(input, mutationFields))
            {
                return std::nullopt;
            }
            return m_repo->GetByCode(input.code, requestedFields);
        }

        bool DeleteByCode(const std::string& code)
        {
            if (!m_repo) return false;
            return m_repo->DeleteByCode(code);
        }
    };
} // namespace omnisphere::services
