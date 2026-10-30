#pragma once
#include "Identity/Repositories/IdentityRepository.hpp"
#include <OmniData/DatabasePool.hpp>
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <OmniData/DataMapper.hpp>
#include <OmniData/DataTable.hpp>
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>

namespace omnisphere::repositories
{
    /**
     * @brief Repositorio genérico y agnóstico para entidades de catálogo y categorías.
     * Permite operar sobre cualquier tabla de categorías por dominio (ej. "FAQCategories", "EventCategories")
     * de forma completamente transparente y dinámica con dynamic fields y dynamic updates.
     */
    template <typename TModel, typename TCreateDTO, typename TUpdateDTO>
    class CatalogRepository
    {
    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
        std::string m_tableName;
        std::string m_identityName;
        std::string m_prefix;

        static std::string CurrentTimestamp()
        {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&in_time_t));
            return std::string(timeBuf);
        }

    public:
        CatalogRepository(
            std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
            std::string tableName,
            std::string identityName,
            std::string prefix
        ) : m_dbPool(std::move(dbPool)),
            m_tableName(std::move(tableName)),
            m_identityName(std::move(identityName)),
            m_prefix(std::move(prefix)) {}

        ~CatalogRepository() = default;

        const std::string& GetTableName() const { return m_tableName; }
        const std::string& GetIdentityName() const { return m_identityName; }
        const std::string& GetPrefix() const { return m_prefix; }

        /**
         * @brief Inserta un nuevo elemento de catálogo asignando automáticamente el Code secuencial.
         */
        bool Create(TCreateDTO& input, const std::vector<std::string>& mutationFields = {}) const
        {
            if (!m_dbPool) return false;
            auto conn = m_dbPool->Acquire();
            try
            {
                conn->BeginTransaction();

                IdentityRepository identityRepo(m_dbPool);
                input.code = identityRepo.GetNextCode(conn, m_identityName, m_prefix);

                std::string nowStr = CurrentTimestamp();
                if (input.createDate.empty()) {
                    input.createDate = nowStr;
                }
                if (!input.updateDate.has_value() || input.updateDate->empty()) {
                    input.updateDate = nowStr;
                }

                auto insertData = omnisphere::types::BuildInsertQuery(m_tableName, 0, input, mutationFields);

                if (!conn->RunPrepared(insertData.Query, insertData.Parameters))
                {
                    conn->RollbackTransaction();
                    std::cerr << "[CatalogRepository::Create Error] Table: " << m_tableName << " Query: " << insertData.Query << std::endl;
                    return false;
                }

                conn->CommitTransaction();
                return true;
            }
            catch (const std::exception& ex)
            {
                conn->RollbackTransaction();
                std::cerr << "[CatalogRepository::Create Exception] Table: " << m_tableName << " Ex: " << ex.what() << std::endl;
                return false;
            }
        }

        /**
         * @brief Obtiene un elemento por su Code con selección dinámica de campos.
         */
        std::optional<TModel> GetByCode(const std::string& code, const std::vector<std::string>& fields = {}) const
        {
            if (!m_dbPool || code.empty()) return std::nullopt;
            auto conn = m_dbPool->Acquire();
            try
            {
                auto selectFields = omnisphere::types::FilterModelFields<TModel>(fields);
                std::vector<omnisphere::types::Condition> conditions = {
                    {"", "\"Code\"", "=", "?"}
                };
                auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);

                std::string query = "SELECT " + qp.Fields + " FROM " + m_tableName + " WHERE " + qp.Where;
                std::vector<omnisphere::data::DbParamValue> params = { code };

                auto dt = conn->RunPreparedSelect(query, params);
                if (!dt || dt->GetRowCount() == 0) return std::nullopt;

                auto models = omnisphere::types::DataTableToModels<TModel>(dt);
                if (models.empty()) return std::nullopt;
                return models.front();
            }
            catch (const std::exception& ex)
            {
                std::cerr << "[CatalogRepository::GetByCode Exception] Table: " << m_tableName << " Ex: " << ex.what() << std::endl;
                return std::nullopt;
            }
        }

        /**
         * @brief Lista elementos con filtrado opcional por Scope y OnlyActive.
         */
        std::vector<TModel> GetAll(
            const std::optional<std::string>& scope = std::nullopt,
            const std::optional<bool>& onlyActive = std::nullopt,
            const std::vector<std::string>& fields = {}
        ) const
        {
            if (!m_dbPool) return {};
            auto conn = m_dbPool->Acquire();
            try
            {
                auto selectFields = omnisphere::types::FilterModelFields<TModel>(fields);
                std::vector<omnisphere::types::Condition> conditions;
                std::vector<omnisphere::data::DbParamValue> params;

                if (scope.has_value() && !scope->empty())
                {
                    conditions.push_back({"", "\"Scope\"", "=", "?"});
                    params.emplace_back(*scope);
                }

                if (onlyActive.has_value())
                {
                    conditions.push_back({conditions.empty() ? "" : "AND", "\"IsActive\"", "=", "?"});
                    params.emplace_back(*onlyActive);
                }

                auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
                std::string query = "SELECT " + qp.Fields + " FROM " + m_tableName;
                if (!qp.Where.empty())
                {
                    query += " WHERE " + qp.Where;
                }
                query += " ORDER BY \"SortOrder\" ASC, \"Name\" ASC";

                auto dt = conn->RunPreparedSelect(query, params);
                if (!dt || dt->GetRowCount() == 0) return {};

                return omnisphere::types::DataTableToModels<TModel>(dt);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "[CatalogRepository::GetAll Exception] Table: " << m_tableName << " Ex: " << ex.what() << std::endl;
                return {};
            }
        }

        /**
         * @brief Actualiza dinámicamente solo los campos mutados de la entidad.
         */
        bool Update(const TUpdateDTO& input, const std::vector<std::string>& mutationFields = {}) const
        {
            if (!m_dbPool || input.code.empty()) return false;
            auto conn = m_dbPool->Acquire();
            try
            {
                conn->BeginTransaction();

                TUpdateDTO mutableInput = input;
                mutableInput.updateDate = CurrentTimestamp();

                auto [columns, values] = omnisphere::types::ExtractUpdateColumns(mutableInput, mutationFields);
                if (columns.empty())
                {
                    conn->RollbackTransaction();
                    return true;
                }

                auto uq = omnisphere::types::BuildUpdateQuery(m_tableName, columns);
                values.push_back(input.code);

                if (!conn->RunPrepared(uq.Query, values))
                {
                    conn->RollbackTransaction();
                    std::cerr << "[CatalogRepository::Update Error] Table: " << m_tableName << " Query: " << uq.Query << std::endl;
                    return false;
                }

                conn->CommitTransaction();
                return true;
            }
            catch (const std::exception& ex)
            {
                conn->RollbackTransaction();
                std::cerr << "[CatalogRepository::Update Exception] Table: " << m_tableName << " Ex: " << ex.what() << std::endl;
                return false;
            }
        }

        /**
         * @brief Elimina físicamente un elemento por su código.
         */
        bool DeleteByCode(const std::string& code) const
        {
            if (!m_dbPool || code.empty()) return false;
            auto conn = m_dbPool->Acquire();
            try
            {
                std::string query = "DELETE FROM " + m_tableName + " WHERE \"Code\" = $1";
                std::vector<omnisphere::data::DbParamValue> params = { code };
                return conn->RunPrepared(query, params);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "[CatalogRepository::DeleteByCode Exception] Table: " << m_tableName << " Ex: " << ex.what() << std::endl;
                return false;
            }
        }
    };
} // namespace omnisphere::repositories
