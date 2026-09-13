#include "Payment/Repositories/PaymentMethodRepository.hpp"
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include "Identity/Repositories/IdentityRepository.hpp"
#include <iostream>

namespace omnisphere::repositories
{
    PaymentMethodRepository::PaymentMethodRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    bool PaymentMethodRepository::Create(const omnisphere::dtos::CreatePaymentMethodInput& input) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();

            omnisphere::dtos::CreatePaymentMethodInput tempInput = input;
            IdentityRepository identityRepo(m_dbPool);
            tempInput.Code = identityRepo.GetNextCode("PaymentMethod", "PMT");
            const_cast<omnisphere::dtos::CreatePaymentMethodInput&>(input).Code = tempInput.Code;

            auto insertResult = omnisphere::types::BuildInsertQuery("\"PaymentMethods\"", 0, tempInput);
            return conn->RunPrepared(insertResult.Query, insertResult.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentMethodRepository::Create Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool PaymentMethodRepository::Update(const omnisphere::dtos::UpdatePaymentMethodInput& input) const
    {
        if (!m_dbPool || input.Entry <= 0) return false;
        try
        {
            auto conn = m_dbPool->Acquire();

            auto updateCols = omnisphere::types::ExtractUpdateColumns(input);
            if (updateCols.empty()) return true;

            updateCols.push_back({"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(input.LastUpdatedBy)});

            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"PaymentMethods\"", updateCols, "\"Entry\"", omnisphere::types::MakeSQLParam(input.Entry)
            );
            return conn->RunPrepared(updateResult.Query, updateResult.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentMethodRepository::Update Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool PaymentMethodRepository::Delete(int entry) const
    {
        if (!m_dbPool || entry <= 0) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<omnisphere::types::ColumnValue> updateCols = {
                {"\"IsActive\"", omnisphere::types::MakeSQLParam(false)}
            };
            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"PaymentMethods\"", updateCols, "\"Entry\"", omnisphere::types::MakeSQLParam(entry)
            );
            return conn->RunPrepared(updateResult.Query, updateResult.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentMethodRepository::Delete Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    omnisphere::types::DataTable PaymentMethodRepository::ReadAll(const std::vector<std::string>& fields) const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::PaymentMethod>(fields);
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"PaymentMethods\" WHERE \"IsActive\" = ? ORDER BY \"Entry\" ASC";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(true) };
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentMethodRepository::ReadAll Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    omnisphere::types::DataTable PaymentMethodRepository::GetByEntry(int entry, const std::vector<std::string>& fields) const
    {
        if (!m_dbPool || entry <= 0) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::PaymentMethod>(fields);
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"Entry\"", "=", "?"},
                {"", "\"IsActive\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"PaymentMethods\" WHERE " + qp.WhereClause;
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(entry),
                omnisphere::types::MakeSQLParam(true)
            };
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentMethodRepository::GetByEntry Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    omnisphere::types::DataTable PaymentMethodRepository::GetByCode(const std::string& code, const std::vector<std::string>& fields) const
    {
        if (!m_dbPool || code.empty()) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::PaymentMethod>(fields);
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"Code\"", "=", "?"},
                {"", "\"IsActive\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"PaymentMethods\" WHERE " + qp.WhereClause;
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(code),
                omnisphere::types::MakeSQLParam(true)
            };
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentMethodRepository::GetByCode Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    omnisphere::types::DataTable PaymentMethodRepository::GetActiveMethods(const std::vector<std::string>& fields) const
    {
        return ReadAll(fields);
    }
} // namespace omnisphere::repositories
