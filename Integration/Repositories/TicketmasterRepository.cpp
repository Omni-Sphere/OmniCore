#include "Integration/Repositories/TicketmasterRepository.hpp"
#include <OmniData/DataMapper.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <iostream>

namespace omnisphere::repositories
{
    TicketmasterRepository::TicketmasterRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    omnisphere::types::DataTable TicketmasterRepository::GetSettings(const std::vector<std::string>& fields) const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<std::string> requestedFields = fields;
            if (requestedFields.empty())
            {
                requestedFields = {
                    "Entry", "Code", "Name", "ApiKey", "ApiSecret", "ApiBaseUrl", "ApiVersion",
                    "DefaultCountryCode", "DefaultCity", "DefaultClassification", "AutoCreateRoutes",
                    "DefaultOriginPointCode", "SyncIntervalMinutes", "IsActive", "LastSyncDate",
                    "CreatedBy", "CreateDate", "LastUpdatedBy", "UpdateDate"
                };
            }
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::TicketmasterSettings>(requestedFields);
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"TicketmasterSettings\" LIMIT 1";
            std::vector<omnisphere::types::SQLParam> params;
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[TicketmasterRepository::GetSettings Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    bool TicketmasterRepository::Create(const omnisphere::models::TicketmasterSettings& settings, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            conn->BeginTransaction();
            auto insertResult = omnisphere::types::BuildInsertQuery("\"TicketmasterSettings\"", 0, settings, mutationFields);
            if (!conn->RunPrepared(insertResult.Query, insertResult.Parameters))
            {
                conn->RollbackTransaction();
                return false;
            }
            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[TicketmasterRepository::Create Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool TicketmasterRepository::Update(const omnisphere::models::TicketmasterSettings& settings, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            conn->BeginTransaction();
            auto updateCols = omnisphere::types::ExtractUpdateColumns(settings, mutationFields);
            if (updateCols.empty())
            {
                conn->CommitTransaction();
                return true;
            }
            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"TicketmasterSettings\"",
                updateCols,
                "\"Code\"",
                omnisphere::types::MakeSQLParam(settings.code.empty() ? "DEFAULT" : settings.code)
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
            std::cerr << "[TicketmasterRepository::Update Exception] " << ex.what() << std::endl;
            return false;
        }
    }
}
