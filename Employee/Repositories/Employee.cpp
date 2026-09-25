#include "Employee/Repositories/Employee.hpp"
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <iostream>

namespace omnisphere::repositories
{
    Employee::Employee(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    omnisphere::models::Employee Employee::MapRow(omnisphere::types::DataTable::Row& row) const
    {
        return omnisphere::types::FromDataRow<omnisphere::models::Employee>(row);
    }

    bool Employee::Create(const omnisphere::dtos::CreateEmployee& emp, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            auto insertData = omnisphere::types::BuildInsertQuery("\"Employees\"", 0, emp, mutationFields);

            if (!conn->RunPrepared(insertData.Query, insertData.Parameters))
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
            std::cerr << "[Employee::Create Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool Employee::Update(const omnisphere::dtos::UpdateEmployee& emp, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            auto cols = omnisphere::types::ExtractUpdateColumns(emp, mutationFields);

            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&in_time_t));

            bool hasLastUpdatedBy = false;
            for (const auto& c : cols) {
                if (c.Column == "\"LastUpdatedBy\"") { hasLastUpdatedBy = true; break; }
            }
            if (!hasLastUpdatedBy) {
                cols.push_back({"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(emp.lastUpdatedBy.value_or("SYSTEM"))});
            }
            bool hasUpdateDate = false;
            for (const auto& c : cols) {
                if (c.Column == "\"UpdateDate\"") { hasUpdateDate = true; break; }
            }
            if (!hasUpdateDate) {
                cols.push_back({"\"UpdateDate\"", omnisphere::types::MakeSQLParam(std::string(timeBuf))});
            }

            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"Employees\"", cols, "\"Code\"", omnisphere::types::MakeSQLParam(emp.code));

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
            std::cerr << "[Employee::Update Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool Employee::Delete(const std::string& code, const std::vector<std::string>& /*mutationFields*/) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            char timeBuf[32];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&in_time_t));

            std::vector<omnisphere::types::ColumnValue> updateCols = {
                {"\"IsCanceled\"", omnisphere::types::MakeSQLParam(true)},
                {"\"IsActive\"", omnisphere::types::MakeSQLParam(false)},
                {"\"UpdateDate\"", omnisphere::types::MakeSQLParam(std::string(timeBuf))}
            };

            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"Employees\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(code));
            bool ok = conn->RunPrepared(updateResult.Query, updateResult.Parameters);

            // Unlink in Users
            std::vector<omnisphere::types::ColumnValue> unlinkCols = {
                {"\"EmployeeCode\"", omnisphere::types::MakeSQLParam(std::optional<std::string>())}
            };
            auto unlinkResult = omnisphere::types::BuildUpdateQuery(
                "\"Users\"", unlinkCols, "\"EmployeeCode\"", omnisphere::types::MakeSQLParam(code));
            conn->RunPrepared(unlinkResult.Query, unlinkResult.Parameters);

            conn->CommitTransaction();
            return ok;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[Employee::Delete Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    std::optional<omnisphere::models::Employee> Employee::GetByCode(const std::string& code, const std::vector<std::string>& fields) const
    {
        if (!m_dbPool) return std::nullopt;
        auto conn = m_dbPool->Acquire();
        try
        {
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::Employee>(fields);
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"Code\"", "=", "?"},
                {"", "\"IsCanceled\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"Employees\" WHERE " + qp.WhereClause + " LIMIT 1";

            auto dt = conn->FetchPrepared(sql, {
                omnisphere::types::MakeSQLParam(code),
                omnisphere::types::MakeSQLParam(false)
            });
            if (dt.RowsCount() > 0)
            {
                return MapRow(dt[0]);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[Employee::GetByCode Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }

    std::optional<omnisphere::models::Employee> Employee::GetByEntry(int entry, const std::vector<std::string>& fields) const
    {
        if (!m_dbPool) return std::nullopt;
        auto conn = m_dbPool->Acquire();
        try
        {
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::Employee>(fields);
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"Entry\"", "=", "?"},
                {"", "\"IsCanceled\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"Employees\" WHERE " + qp.WhereClause + " LIMIT 1";

            auto dt = conn->FetchPrepared(sql, {
                omnisphere::types::MakeSQLParam(entry),
                omnisphere::types::MakeSQLParam(false)
            });
            if (dt.RowsCount() > 0)
            {
                return MapRow(dt[0]);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[Employee::GetByEntry Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }

    std::vector<omnisphere::models::Employee> Employee::GetAll(const std::vector<std::string>& fields) const
    {
        std::vector<omnisphere::models::Employee> result;
        if (!m_dbPool) return result;
        auto conn = m_dbPool->Acquire();
        try
        {
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::Employee>(fields);
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"IsCanceled\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"Employees\" WHERE " + qp.WhereClause + " ORDER BY \"Entry\" ASC";

            auto dt = conn->FetchPrepared(sql, { omnisphere::types::MakeSQLParam(false) });
            result.reserve(dt.RowsCount());
            for (size_t i = 0; i < dt.RowsCount(); ++i)
            {
                result.push_back(MapRow(dt[i]));
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[Employee::GetAll Exception] " << ex.what() << std::endl;
        }
        return result;
    }

    EmployeeCursorPage Employee::GetPage(std::optional<int> afterEntry, int limit, const std::vector<std::string>& fields) const
    {
        EmployeeCursorPage page;
        if (!m_dbPool) return page;
        auto conn = m_dbPool->Acquire();
        try
        {
            std::string countSql = "SELECT COALESCE(COUNT(*), 0) AS Total FROM \"Employees\" WHERE \"IsCanceled\" = false";
            auto countDt = conn->FetchResults(countSql);
            if (countDt.RowsCount() > 0)
            {
                page.totalCount = countDt[0]["Total"];
            }

            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::Employee>(fields);
            // Ensure Entry is always present for cursor pagination
            bool hasEntry = false;
            for (const auto& f : selectFields) {
                if (f == "\"Entry\"" || f == "Entry") { hasEntry = true; break; }
            }
            if (!hasEntry) {
                selectFields.insert(selectFields.begin(), "\"Entry\"");
            }

            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"IsCanceled\"", "=", "?"}
            };
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(false)
            };

            if (afterEntry.has_value())
            {
                conditions.push_back({"", "\"Entry\"", ">", "?"});
                params.push_back(omnisphere::types::MakeSQLParam(afterEntry.value()));
            }

            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"Employees\" WHERE " + qp.WhereClause + " ORDER BY \"Entry\" ASC LIMIT ?";
            params.push_back(omnisphere::types::MakeSQLParam(limit + 1));

            auto dt = conn->FetchPrepared(sql, params);
            page.hasPreviousPage = afterEntry.has_value();
            size_t rowLimit = std::min<size_t>(dt.RowsCount(), static_cast<size_t>(limit));
            page.employees.reserve(rowLimit);

            for (size_t i = 0; i < rowLimit; ++i)
            {
                page.employees.push_back(MapRow(dt[i]));
            }

            if (dt.RowsCount() > static_cast<size_t>(limit))
            {
                page.nextCursor = page.employees.back().entry;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[Employee::GetPage Exception] " << ex.what() << std::endl;
        }
        return page;
    }
} // namespace omnisphere::repositories
