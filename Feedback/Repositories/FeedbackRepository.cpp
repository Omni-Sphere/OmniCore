#include "Feedback/Repositories/FeedbackRepository.hpp"
#include "Identity/Repositories/IdentityRepository.hpp"
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <OmniData/DataMapper.hpp>
#include <iostream>
#include <chrono>
#include <ctime>

namespace omnisphere::repositories
{
    FeedbackRepository::FeedbackRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    bool FeedbackRepository::Create(omnisphere::dtos::CreateFeedback& feedback) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();

            IdentityRepository identityRepo(m_dbPool);
            feedback.code = identityRepo.GetNextCode(conn, "Feedback", "FDK");

            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&in_time_t));
            feedback.createDate = std::string(timeBuf);
            feedback.updateDate = std::string(timeBuf);

            auto insertData = omnisphere::types::BuildInsertQuery("\"core_feedback\"", 0, feedback);

            if (!conn->RunPrepared(insertData.Query, insertData.Parameters))
            {
                conn->RollbackTransaction();
                std::cerr << "[FeedbackRepository::Create Error] Failed to insert into core_feedback: " << insertData.Query << std::endl;
                return false;
            }

            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[FeedbackRepository::Create Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    std::optional<omnisphere::models::FeedbackItem> FeedbackRepository::GetByCode(const std::string& code) const
    {
        if (!m_dbPool) return std::nullopt;
        auto conn = m_dbPool->Acquire();
        try
        {
            std::string sql = "SELECT * FROM \"core_feedback\" WHERE \"Code\" = ? LIMIT 1";
            auto dt = conn->FetchPrepared(sql, { omnisphere::types::MakeSQLParam(code) });
            if (!dt.IsEmpty())
            {
                return omnisphere::types::FromDataRow<omnisphere::models::FeedbackItem>(dt[0]);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FeedbackRepository::GetByCode Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }

    std::vector<omnisphere::models::FeedbackItem> FeedbackRepository::GetFeedbacks(
        const std::optional<std::string>& scope,
        const std::optional<std::string>& type,
        const std::optional<bool>& onlyUnreviewed,
        int limit,
        int offset
    ) const
    {
        std::vector<omnisphere::models::FeedbackItem> result;
        if (!m_dbPool) return result;
        auto conn = m_dbPool->Acquire();
        try
        {
            std::string sql = "SELECT * FROM \"core_feedback\" WHERE 1=1";
            std::vector<omnisphere::types::SQLParam> params;

            if (scope && !scope->empty())
            {
                sql += " AND \"Scope\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(*scope));
            }
            if (type && !type->empty())
            {
                sql += " AND \"Type\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(*type));
            }
            if (onlyUnreviewed.has_value() && *onlyUnreviewed)
            {
                sql += " AND \"IsReviewed\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(false));
            }

            sql += " ORDER BY \"CreateDate\" DESC";

            if (limit <= 0) limit = 50;
            if (offset < 0) offset = 0;
            sql += " LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

            auto dt = conn->FetchPrepared(sql, params);
            if (!dt.IsEmpty())
            {
                return omnisphere::types::DataTableToModels<omnisphere::models::FeedbackItem>(dt);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FeedbackRepository::GetFeedbacks Exception] " << ex.what() << std::endl;
        }
        return result;
    }

    bool FeedbackRepository::ToggleReviewedByCode(
        const std::string& code,
        bool isReviewed,
        const std::optional<std::string>& adminNotes,
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

            std::string sql = "UPDATE \"core_feedback\" SET \"IsReviewed\" = ?, \"LastUpdatedBy\" = ?, \"UpdateDate\" = ?";
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(isReviewed),
                omnisphere::types::MakeSQLParam(updatedBy),
                omnisphere::types::MakeSQLParam(std::string(timeBuf))
            };

            if (adminNotes.has_value())
            {
                sql += ", \"AdminNotes\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(*adminNotes));
            }

            sql += " WHERE \"Code\" = ?";
            params.push_back(omnisphere::types::MakeSQLParam(code));

            return conn->RunPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FeedbackRepository::ToggleReviewedByCode Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool FeedbackRepository::DeleteByCode(const std::string& code) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            std::string sql = "DELETE FROM \"core_feedback\" WHERE \"Code\" = ?";
            return conn->RunPrepared(sql, { omnisphere::types::MakeSQLParam(code) });
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[FeedbackRepository::DeleteByCode Exception] " << ex.what() << std::endl;
            return false;
        }
    }
} // namespace omnisphere::repositories
