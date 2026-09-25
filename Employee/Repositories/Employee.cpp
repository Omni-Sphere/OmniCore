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

    bool Employee::Create(const omnisphere::dtos::CreateEmployee& emp) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            std::string sql =
                "INSERT INTO \"Employees\" ("
                "\"Code\", \"Name\", \"FirstName\", \"SecondName\", \"LastName\", \"SecondLastName\", "
                "\"Email\", \"Phone\", \"Department\", \"Position\", \"DirectManagerCode\", "
                "\"DateOfBirth\", \"Comments\", \"IsActive\", \"CreateDate\", \"CreatedBy\""
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(emp.code),
                omnisphere::types::MakeSQLParam(emp.name),
                omnisphere::types::MakeSQLParam(emp.firstName),
                omnisphere::types::MakeSQLParam(emp.secondName),
                omnisphere::types::MakeSQLParam(emp.lastName),
                omnisphere::types::MakeSQLParam(emp.secondLastName),
                omnisphere::types::MakeSQLParam(emp.email),
                omnisphere::types::MakeSQLParam(emp.phone),
                omnisphere::types::MakeSQLParam(emp.department),
                omnisphere::types::MakeSQLParam(emp.position),
                omnisphere::types::MakeSQLParam(emp.directManagerCode),
                omnisphere::types::MakeSQLParam(emp.dateOfBirth),
                omnisphere::types::MakeSQLParam(emp.comments),
                omnisphere::types::MakeSQLParam(emp.isActive),
                omnisphere::types::MakeSQLParam(emp.createDate),
                omnisphere::types::MakeSQLParam(emp.createdBy)
            };

            if (!conn->RunPrepared(sql, params))
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

    bool Employee::Update(const omnisphere::dtos::UpdateEmployee& emp) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            std::string sql = "UPDATE \"Employees\" SET ";
            std::vector<omnisphere::types::SQLParam> params;

            if (emp.name.has_value()) {
                sql += "\"Name\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.name.value()));
            }
            if (emp.firstName.has_value()) {
                sql += "\"FirstName\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.firstName.value()));
            }
            if (emp.secondName.has_value()) {
                sql += "\"SecondName\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.secondName.value()));
            }
            if (emp.lastName.has_value()) {
                sql += "\"LastName\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.lastName.value()));
            }
            if (emp.secondLastName.has_value()) {
                sql += "\"SecondLastName\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.secondLastName.value()));
            }
            if (emp.email.has_value()) {
                sql += "\"Email\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.email.value()));
            }
            if (emp.phone.has_value()) {
                sql += "\"Phone\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.phone.value()));
            }
            if (emp.department.has_value()) {
                sql += "\"Department\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.department.value()));
            }
            if (emp.position.has_value()) {
                sql += "\"Position\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.position.value()));
            }
            if (emp.directManagerCode.has_value()) {
                sql += "\"DirectManagerCode\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.directManagerCode.value()));
            }
            if (emp.dateOfBirth.has_value()) {
                sql += "\"DateOfBirth\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.dateOfBirth.value()));
            }
            if (emp.comments.has_value()) {
                sql += "\"Comments\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.comments.value()));
            }
            if (emp.isActive.has_value()) {
                sql += "\"IsActive\" = ?, ";
                params.push_back(omnisphere::types::MakeSQLParam(emp.isActive.value()));
            }

            sql += "\"LastUpdatedBy\" = ?, \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"Code\" = ?";
            params.push_back(omnisphere::types::MakeSQLParam(emp.updatedBy));
            params.push_back(omnisphere::types::MakeSQLParam(emp.code));

            if (!conn->RunPrepared(sql, params))
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

    bool Employee::Delete(const std::string& code) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            std::string sql = "UPDATE \"Employees\" SET \"IsCanceled\" = true, \"IsActive\" = false, \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"Code\" = ?";
            bool ok = conn->RunPrepared(sql, { omnisphere::types::MakeSQLParam(code) });

            // Unlink in Users
            std::string unlink = "UPDATE \"Users\" SET \"EmployeeCode\" = NULL WHERE \"EmployeeCode\" = ?";
            conn->RunPrepared(unlink, { omnisphere::types::MakeSQLParam(code) });

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
