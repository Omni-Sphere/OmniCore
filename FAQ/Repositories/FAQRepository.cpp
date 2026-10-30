#include "FAQ/Repositories/FAQRepository.hpp"
#include "Identity/Repositories/IdentityRepository.hpp"
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <OmniData/DataMapper.hpp>
#include <iostream>
#include <chrono>
#include <ctime>

namespace omnisphere::repositories
{
    FAQRepository::FAQRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    // =========================================================================
    // Category Methods
    // =========================================================================
    bool FAQRepository::CreateCategory(omnisphere::dtos::CreateFAQCategoryInput& category, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();

            IdentityRepository identityRepo(m_dbPool);
            category.code = identityRepo.GetNextCode(conn, "FAQCategory", "FQC");

            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&in_time_t));
            if (category.createDate.empty()) {
                category.createDate = std::string(timeBuf);
            }
            if (!category.updateDate.has_value() || category.updateDate->empty()) {
                category.updateDate = std::string(timeBuf);
            }

            auto insertData = omnisphere::types::BuildInsertQuery("\"FAQCategories\"", 0, category, mutationFields);

            if (!conn->RunPrepared(insertData.Query, insertData.Parameters))
            {
                conn->RollbackTransaction();
                std::cerr << "[FAQRepository::CreateCategory Error] Failed to insert: " << insertData.Query << std::endl;
                return false;
            }

            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[FAQRepository::CreateCategory Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    std::optional<omnisphere::models::FAQCategory> FAQRepository::GetCategoryByCode(const std::string& code, const std::vector<std::string>& fields) const
    {
        if (!m_dbPool || code.empty()) return std::nullopt;
        auto conn = m_dbPool->Acquire();
        try
        {
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::FAQCategory>(fields);
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"Code\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"FAQCategories\" WHERE " + qp.WhereClause + " LIMIT 1";

            auto dt = conn->FetchPrepared(sql, { omnisphere::types::MakeSQLParam(code) });
            if (!dt.IsEmpty())
            {
                return omnisphere::types::FromDataRow<omnisphere::models::FAQCategory>(dt[0]);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FAQRepository::GetCategoryByCode Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }

    std::vector<omnisphere::models::FAQCategory> FAQRepository::GetCategories(
        const std::optional<std::string>& scope,
        const std::optional<bool>& onlyActive,
        const std::vector<std::string>& fields
    ) const
    {
        if (!m_dbPool) return {};
        auto conn = m_dbPool->Acquire();
        try
        {
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::FAQCategory>(fields);
            std::vector<omnisphere::types::Condition> conditions;
            std::vector<omnisphere::types::SQLParam> params;

            if (scope.has_value() && !scope->empty()) {
                conditions.push_back({conditions.empty() ? "" : "AND", "(\"Scope\" = ? OR \"Scope\" = 'GLOBAL')", "", ""});
                params.push_back(omnisphere::types::MakeSQLParam(scope.value()));
            }

            if (onlyActive.has_value() && onlyActive.value()) {
                conditions.push_back({conditions.empty() ? "" : "AND", "\"IsActive\"", "=", "?"});
                params.push_back(omnisphere::types::MakeSQLParam(true));
            }

            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"FAQCategories\"";
            if (!qp.WhereClause.empty()) {
                sql += " WHERE " + qp.WhereClause;
            }
            sql += " ORDER BY \"SortOrder\" ASC, \"Entry\" ASC";

            auto dt = conn->FetchPrepared(sql, params);
            return omnisphere::types::DataTableToModels<omnisphere::models::FAQCategory>(dt);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FAQRepository::GetCategories Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    bool FAQRepository::UpdateCategory(
        const omnisphere::dtos::UpdateFAQCategoryInput& input,
        const std::vector<std::string>& mutationFields
    ) const
    {
        if (!m_dbPool || input.code.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            omnisphere::dtos::UpdateFAQCategoryInput mutableInput = input;
            auto updateCols = omnisphere::types::ExtractUpdateColumns(mutableInput, mutationFields);
            if (updateCols.empty())
            {
                conn->CommitTransaction();
                return true;
            }

            bool hasLastUpdatedBy = false;
            bool hasUpdateDate = false;
            for (const auto& c : updateCols) {
                if (c.Column == "\"LastUpdatedBy\"") hasLastUpdatedBy = true;
                if (c.Column == "\"UpdateDate\"") hasUpdateDate = true;
            }
            if (!hasLastUpdatedBy && mutableInput.lastUpdatedBy.has_value()) {
                updateCols.push_back({"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(mutableInput.lastUpdatedBy.value())});
            }
            if (!hasUpdateDate) {
                auto now = std::chrono::system_clock::now();
                auto in_time_t = std::chrono::system_clock::to_time_t(now);
                char buf[32];
                std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&in_time_t));
                updateCols.push_back({"\"UpdateDate\"", omnisphere::types::MakeSQLParam(mutableInput.updateDate.empty() ? std::string(buf) : mutableInput.updateDate)});
            }

            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"FAQCategories\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(mutableInput.code)
            );
            if (!conn->RunPrepared(updateResult.Query, updateResult.Parameters))
            {
                conn->RollbackTransaction();
                return false;
            }
            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[FAQRepository::UpdateCategory Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool FAQRepository::DeleteCategoryByCode(const std::string& code) const
    {
        if (!m_dbPool || code.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            std::string sql = "DELETE FROM \"FAQCategories\" WHERE \"Code\" = ?";
            return conn->RunPrepared(sql, { omnisphere::types::MakeSQLParam(code) });
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FAQRepository::DeleteCategoryByCode Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    // =========================================================================
    // FAQ Methods
    // =========================================================================
    bool FAQRepository::Create(omnisphere::dtos::CreateFAQInput& faq, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();

            IdentityRepository identityRepo(m_dbPool);
            faq.code = identityRepo.GetNextCode(conn, "FAQ", "FAQ");

            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&in_time_t));
            if (faq.createDate.empty()) {
                faq.createDate = std::string(timeBuf);
            }
            if (!faq.updateDate.has_value() || faq.updateDate->empty()) {
                faq.updateDate = std::string(timeBuf);
            }

            auto insertData = omnisphere::types::BuildInsertQuery("\"FAQs\"", 0, faq, mutationFields);

            if (!conn->RunPrepared(insertData.Query, insertData.Parameters))
            {
                conn->RollbackTransaction();
                std::cerr << "[FAQRepository::Create Error] Failed to insert into FAQs: " << insertData.Query << std::endl;
                return false;
            }

            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[FAQRepository::Create Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    std::optional<omnisphere::models::FAQ> FAQRepository::GetByCode(const std::string& code, const std::vector<std::string>& fields) const
    {
        if (!m_dbPool || code.empty()) return std::nullopt;
        auto conn = m_dbPool->Acquire();
        try
        {
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::FAQ>(fields);
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"Code\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"FAQs\" WHERE " + qp.WhereClause + " LIMIT 1";

            auto dt = conn->FetchPrepared(sql, { omnisphere::types::MakeSQLParam(code) });
            if (!dt.IsEmpty())
            {
                return omnisphere::types::FromDataRow<omnisphere::models::FAQ>(dt[0]);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FAQRepository::GetByCode Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }

    std::vector<omnisphere::models::FAQ> FAQRepository::GetFAQs(
        const std::optional<std::string>& scope,
        const std::optional<std::string>& categoryCode,
        const std::optional<bool>& onlyActive,
        const std::vector<std::string>& fields
    ) const
    {
        if (!m_dbPool) return {};
        auto conn = m_dbPool->Acquire();
        try
        {
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::FAQ>(fields);
            std::vector<omnisphere::types::Condition> conditions;
            std::vector<omnisphere::types::SQLParam> params;

            if (scope.has_value() && !scope->empty()) {
                conditions.push_back({conditions.empty() ? "" : "AND", "(\"Scope\" = ? OR \"Scope\" = 'GLOBAL')", "", ""});
                params.push_back(omnisphere::types::MakeSQLParam(scope.value()));
            }

            if (categoryCode.has_value() && !categoryCode->empty() && categoryCode.value() != "ALL") {
                conditions.push_back({conditions.empty() ? "" : "AND", "\"CategoryCode\"", "=", "?"});
                params.push_back(omnisphere::types::MakeSQLParam(categoryCode.value()));
            }

            if (onlyActive.has_value() && onlyActive.value()) {
                conditions.push_back({conditions.empty() ? "" : "AND", "\"IsActive\"", "=", "?"});
                params.push_back(omnisphere::types::MakeSQLParam(true));
            }

            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"FAQs\"";
            if (!qp.WhereClause.empty()) {
                sql += " WHERE " + qp.WhereClause;
            }
            sql += " ORDER BY \"SortOrder\" ASC, \"Entry\" ASC";

            auto dt = conn->FetchPrepared(sql, params);
            return omnisphere::types::DataTableToModels<omnisphere::models::FAQ>(dt);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FAQRepository::GetFAQs Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    bool FAQRepository::Update(
        const omnisphere::dtos::UpdateFAQInput& input,
        const std::vector<std::string>& mutationFields
    ) const
    {
        if (!m_dbPool || input.code.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            omnisphere::dtos::UpdateFAQInput mutableInput = input;
            auto updateCols = omnisphere::types::ExtractUpdateColumns(mutableInput, mutationFields);
            if (updateCols.empty())
            {
                conn->CommitTransaction();
                return true;
            }

            bool hasLastUpdatedBy = false;
            bool hasUpdateDate = false;
            for (const auto& c : updateCols) {
                if (c.Column == "\"LastUpdatedBy\"") hasLastUpdatedBy = true;
                if (c.Column == "\"UpdateDate\"") hasUpdateDate = true;
            }
            if (!hasLastUpdatedBy && mutableInput.lastUpdatedBy.has_value()) {
                updateCols.push_back({"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(mutableInput.lastUpdatedBy.value())});
            }
            if (!hasUpdateDate) {
                auto now = std::chrono::system_clock::now();
                auto in_time_t = std::chrono::system_clock::to_time_t(now);
                char buf[32];
                std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&in_time_t));
                updateCols.push_back({"\"UpdateDate\"", omnisphere::types::MakeSQLParam(mutableInput.updateDate.empty() ? std::string(buf) : mutableInput.updateDate)});
            }

            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"FAQs\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(mutableInput.code)
            );
            if (!conn->RunPrepared(updateResult.Query, updateResult.Parameters))
            {
                conn->RollbackTransaction();
                return false;
            }
            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[FAQRepository::Update Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool FAQRepository::DeleteByCode(const std::string& code) const
    {
        if (!m_dbPool || code.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            std::string sql = "DELETE FROM \"FAQs\" WHERE \"Code\" = ?";
            return conn->RunPrepared(sql, { omnisphere::types::MakeSQLParam(code) });
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FAQRepository::DeleteByCode Exception] " << ex.what() << std::endl;
            return false;
        }
    }
} // namespace omnisphere::repositories
