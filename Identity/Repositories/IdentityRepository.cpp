#include "Identity/Repositories/IdentityRepository.hpp"
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace omnisphere::repositories
{
    IdentityRepository::IdentityRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    bool IdentityRepository::Create(const omnisphere::dtos::CreateIdentityInput& input) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = "INSERT INTO \"Identities\" (\"Domain\", \"Prefix1\", \"Prefix2\", \"Prefix3\", \"CurrentSequence\", \"IsActive\", \"CreatedBy\", \"CreateDate\") "
                              "VALUES (?, ?, ?, ?, ?, true, ?, NOW())";

            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(input.domain),
                omnisphere::types::MakeSQLParam(input.prefix1),
                input.prefix2.has_value() ? omnisphere::types::MakeSQLParam(input.prefix2.value()) : omnisphere::types::MakeSQLParam(std::string{}),
                input.prefix3.has_value() ? omnisphere::types::MakeSQLParam(input.prefix3.value()) : omnisphere::types::MakeSQLParam(std::string{}),
                omnisphere::types::MakeSQLParam(input.initialSequence),
                omnisphere::types::MakeSQLParam(input.createdBy)
            };
            return conn->RunPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[IdentityRepository::Create Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    template <typename T, typename TRow>
    static T GetVal(const TRow& row, const std::string& col, T defaultVal = T{})
    {
        if (row.HasColumn(col)) {
            auto val = row[col];
            if (val.has_value()) {
                if (auto p = std::get_if<T>(&(*val))) {
                    return *p;
                }
            }
        }
        return defaultVal;
    }

    std::string IdentityRepository::GetNextCode(const std::string& domain, const std::string& defaultPrefix, int prefixIndex) const
    {
        if (!m_dbPool) return "";
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sqlUpdate = "UPDATE \"Identities\" SET \"CurrentSequence\" = \"CurrentSequence\" + 1, \"UpdateDate\" = NOW() "
                                    "WHERE \"Domain\" = ? RETURNING \"Prefix1\", \"Prefix2\", \"Prefix3\", \"CurrentSequence\"";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(domain) };

            auto dt = conn->FetchPrepared(sqlUpdate, params);
            if (dt.RowsCount() > 0)
            {
                std::string p1 = GetVal<std::string>(dt[0], "Prefix1");
                std::string p2 = GetVal<std::string>(dt[0], "Prefix2");
                std::string p3 = GetVal<std::string>(dt[0], "Prefix3");
                int seq = GetVal<int>(dt[0], "CurrentSequence");

                std::string selectedPrefix = p1;
                if (prefixIndex == 2 && !p2.empty()) selectedPrefix = p2;
                else if (prefixIndex == 3 && !p3.empty()) selectedPrefix = p3;

                selectedPrefix.erase(std::remove_if(selectedPrefix.begin(), selectedPrefix.end(),
                    [](unsigned char c) { return !std::isalnum(c); }), selectedPrefix.end());

                char buf[64];
                snprintf(buf, sizeof(buf), "%s%03d", selectedPrefix.c_str(), seq);
                return std::string(buf);
            }

            // Si el dominio no existía en la tabla, insertarlo con el prefijo por defecto
            std::string pref = !defaultPrefix.empty() ? defaultPrefix : "COD";
            pref.erase(std::remove_if(pref.begin(), pref.end(),
                [](unsigned char c) { return !std::isalnum(c); }), pref.end());

            std::string sqlInsert = "INSERT INTO \"Identities\" (\"Domain\", \"Prefix1\", \"CurrentSequence\", \"CreatedBy\", \"CreateDate\") "
                                    "VALUES (?, ?, 1, 0, NOW()) RETURNING \"Prefix1\", \"CurrentSequence\"";
            std::vector<omnisphere::types::SQLParam> insertParams = {
                omnisphere::types::MakeSQLParam(domain),
                omnisphere::types::MakeSQLParam(pref)
            };

            auto insertDt = conn->FetchPrepared(sqlInsert, insertParams);
            if (insertDt.RowsCount() > 0)
            {
                std::string p1 = GetVal<std::string>(insertDt[0], "Prefix1");
                p1.erase(std::remove_if(p1.begin(), p1.end(),
                    [](unsigned char c) { return !std::isalnum(c); }), p1.end());
                int seq = GetVal<int>(insertDt[0], "CurrentSequence");
                char buf[64];
                snprintf(buf, sizeof(buf), "%s%03d", p1.c_str(), seq);
                return std::string(buf);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[IdentityRepository::GetNextCode Exception] " << ex.what() << std::endl;
        }
        return "";
    }

    omnisphere::types::DataTable IdentityRepository::ReadAll(const std::vector<std::string>& fields) const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::Identity>(fields);
            std::vector<omnisphere::types::Condition> conditions = {{"", "\"IsActive\"", "=", "?"}};
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"Identities\" WHERE " + qp.WhereClause + " ORDER BY \"Domain\" ASC";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(true) };
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[IdentityRepository::ReadAll Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    omnisphere::types::DataTable IdentityRepository::GetByDomain(const std::string& domain, const std::vector<std::string>& fields) const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::Identity>(fields);
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"Domain\"", "=", "?"},
                {"", "\"IsActive\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"Identities\" WHERE " + qp.WhereClause;
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(domain),
                omnisphere::types::MakeSQLParam(true)
            };
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[IdentityRepository::GetByDomain Exception] " << ex.what() << std::endl;
            return {};
        }
    }
} // namespace omnisphere::repositories
