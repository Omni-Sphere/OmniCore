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

    bool FAQRepository::Create(omnisphere::dtos::CreateFAQ& faq) const
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
            faq.createDate = std::string(timeBuf);
            faq.updateDate = std::string(timeBuf);

            auto insertData = omnisphere::types::BuildInsertQuery("\"core_faqs\"", 0, faq);

            if (!conn->RunPrepared(insertData.Query, insertData.Parameters))
            {
                conn->RollbackTransaction();
                std::cerr << "[FAQRepository::Create Error] Failed to insert into core_faqs: " << insertData.Query << std::endl;
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

    std::optional<omnisphere::models::FAQ> FAQRepository::GetByCode(const std::string& code) const
    {
        if (!m_dbPool) return std::nullopt;
        auto conn = m_dbPool->Acquire();
        try
        {
            std::string sql = "SELECT * FROM \"core_faqs\" WHERE \"Code\" = ? LIMIT 1";
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
        const std::optional<std::string>& category,
        const std::optional<bool>& onlyActive
    ) const
    {
        if (!m_dbPool) return {};
        auto conn = m_dbPool->Acquire();
        try
        {
            std::string sql = "SELECT * FROM \"core_faqs\" WHERE 1=1";
            std::vector<omnisphere::types::SQLParam> params;

            if (scope.has_value() && !scope->empty()) {
                sql += " AND (\"Scope\" = ? OR \"Scope\" = 'GLOBAL')";
                params.push_back(omnisphere::types::MakeSQLParam(scope.value()));
            }

            if (category.has_value() && !category->empty() && category.value() != "ALL") {
                sql += " AND \"Category\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(category.value()));
            }

            if (onlyActive.has_value() && onlyActive.value()) {
                sql += " AND \"IsActive\" = true";
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

    std::vector<std::string> FAQRepository::GetCategories(const std::optional<std::string>& scope) const
    {
        if (!m_dbPool) return {};
        auto conn = m_dbPool->Acquire();
        try
        {
            std::string sql = "SELECT DISTINCT \"Category\" FROM \"core_faqs\" WHERE \"IsActive\" = true";
            std::vector<omnisphere::types::SQLParam> params;

            if (scope.has_value() && !scope->empty()) {
                sql += " AND (\"Scope\" = ? OR \"Scope\" = 'GLOBAL')";
                params.push_back(omnisphere::types::MakeSQLParam(scope.value()));
            }

            sql += " ORDER BY \"Category\" ASC";

            auto dt = conn->FetchPrepared(sql, params);
            std::vector<std::string> categories;
            for (const auto& row : dt) {
                try {
                    categories.push_back(row["Category"].operator std::string());
                } catch (...) {}
            }
            return categories;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FAQRepository::GetCategories Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    bool FAQRepository::Update(
        const std::string& code,
        const omnisphere::dtos::UpdateFAQ& input,
        const std::string& updatedBy
    ) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&in_time_t));
            std::string nowStr(timeBuf);

            std::string sql = "UPDATE \"core_faqs\" SET \"LastUpdatedBy\" = ?, \"UpdateDate\" = ?";
            std::vector<omnisphere::types::SQLParam> params;
            params.push_back(omnisphere::types::MakeSQLParam(updatedBy));
            params.push_back(omnisphere::types::MakeSQLParam(nowStr));

            if (input.name.has_value()) {
                sql += ", \"Name\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(input.name.value()));
            }
            if (input.scope.has_value()) {
                sql += ", \"Scope\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(input.scope.value()));
            }
            if (input.category.has_value()) {
                sql += ", \"Category\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(input.category.value()));
            }
            if (input.question.has_value()) {
                sql += ", \"Question\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(input.question.value()));
            }
            if (input.answer.has_value()) {
                sql += ", \"Answer\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(input.answer.value()));
            }
            if (input.sortOrder.has_value()) {
                sql += ", \"SortOrder\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(input.sortOrder.value()));
            }
            if (input.isActive.has_value()) {
                sql += ", \"IsActive\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(input.isActive.value()));
            }

            sql += " WHERE \"Code\" = ?";
            params.push_back(omnisphere::types::MakeSQLParam(code));

            return conn->RunPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FAQRepository::Update Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool FAQRepository::DeleteByCode(const std::string& code) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            std::string sql = "DELETE FROM \"core_faqs\" WHERE \"Code\" = ?";
            return conn->RunPrepared(sql, { omnisphere::types::MakeSQLParam(code) });
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FAQRepository::DeleteByCode Exception] " << ex.what() << std::endl;
            return false;
        }
    }
} // namespace omnisphere::repositories
