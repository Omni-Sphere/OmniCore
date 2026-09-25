#include "Payment/Repositories/PaymentMethodRepository.hpp"
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include "Identity/Repositories/IdentityRepository.hpp"
#include <OmniUtils/Base64.hpp>
#include <iostream>
#include <stdexcept>

namespace omnisphere::repositories
{
    PaymentMethodRepository::PaymentMethodRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    bool PaymentMethodRepository::Create(const omnisphere::dtos::CreatePaymentMethodInput& input, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        if (input.UsesIntegration && input.Details.has_value())
        {
            throw std::runtime_error("No se permiten detalles de transferencia para formas de pago que usan integración");
        }
        try
        {
            auto conn = m_dbPool->Acquire();

            omnisphere::dtos::CreatePaymentMethodInput tempInput = input;
            IdentityRepository identityRepo(m_dbPool);
            tempInput.Code = identityRepo.GetNextCode("PaymentMethod", "PMT");
            const_cast<omnisphere::dtos::CreatePaymentMethodInput&>(input).Code = tempInput.Code;

            auto insertResult = omnisphere::types::BuildInsertQuery("\"PaymentMethods\"", 0, tempInput, mutationFields);
            bool ok = conn->RunPrepared(insertResult.Query, insertResult.Parameters);
            if (ok && !input.UsesIntegration && input.Details.has_value())
            {
                SaveDetail(tempInput.Code, *input.Details, input.CreatedBy);
            }
            return ok;
        }
        catch (const std::runtime_error&)
        {
            throw;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentMethodRepository::Create Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool PaymentMethodRepository::Update(const omnisphere::dtos::UpdatePaymentMethodInput& input, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool || input.Entry <= 0) return false;
        if (input.UsesIntegration.value_or(false) && input.Details.has_value())
        {
            throw std::runtime_error("No se permiten detalles de transferencia para formas de pago que usan integración");
        }
        try
        {
            auto conn = m_dbPool->Acquire();

            std::string code;
            bool currentUsesIntegration = false;
            std::string getCodeSql = "SELECT \"Code\", \"UsesIntegration\" FROM \"PaymentMethods\" WHERE \"Entry\" = ? LIMIT 1";
            std::vector<omnisphere::types::SQLParam> getParams = { omnisphere::types::MakeSQLParam(input.Entry) };
            auto curDt = conn->FetchPrepared(getCodeSql, getParams);
            if (curDt.RowsCount() > 0)
            {
                code = (std::string)curDt[0]["Code"];
                if (curDt[0].HasColumn("UsesIntegration") && !curDt[0]["UsesIntegration"].IsNull())
                {
                    currentUsesIntegration = (bool)curDt[0]["UsesIntegration"];
                }
            }

            bool effectivelyUsesIntegration = input.UsesIntegration.value_or(currentUsesIntegration);
            if (effectivelyUsesIntegration && input.Details.has_value())
            {
                throw std::runtime_error("No se permiten detalles de transferencia para formas de pago que usan integración");
            }

            auto updateCols = omnisphere::types::ExtractUpdateColumns(input, mutationFields);
            if (!updateCols.empty())
            {
                updateCols.push_back({"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(input.LastUpdatedBy)});

                auto updateResult = omnisphere::types::BuildUpdateQuery(
                    "\"PaymentMethods\"", updateCols, "\"Entry\"", omnisphere::types::MakeSQLParam(input.Entry)
                );
                if (!conn->RunPrepared(updateResult.Query, updateResult.Parameters))
                {
                    return false;
                }
            }

            if (!code.empty())
            {
                if (effectivelyUsesIntegration)
                {
                    DeactivateDetail(code, input.LastUpdatedBy);
                }
                else if (input.Details.has_value())
                {
                    SaveDetail(code, *input.Details, input.LastUpdatedBy);
                }
            }

            return true;
        }
        catch (const std::runtime_error&)
        {
            throw;
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
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"PaymentMethods\" ORDER BY \"Entry\" ASC";
            std::vector<omnisphere::types::SQLParam> params = {};
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
                {"", "\"Entry\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"PaymentMethods\" WHERE " + qp.WhereClause;
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(entry)
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
                {"", "\"Code\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"PaymentMethods\" WHERE " + qp.WhereClause;
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(code)
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
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::PaymentMethod>(fields);
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"IsActive\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"PaymentMethods\" WHERE " + qp.WhereClause + " ORDER BY \"Entry\" ASC";
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(true)
            };
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentMethodRepository::GetActiveMethods Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    std::optional<omnisphere::models::PaymentMethodDetail> PaymentMethodRepository::GetDetailByCode(const std::string& code) const
    {
        if (!m_dbPool || code.empty()) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = "SELECT \"Entry\", \"Code\", \"BankName\", \"CLABE\", \"AccountHolder\", \"PaymentReference\", \"IsActive\", \"CreatedBy\", \"CreateDate\", \"LastUpdatedBy\", \"UpdateDate\" "
                              "FROM \"PaymentMethodDetails\" WHERE \"Code\" = ? AND \"IsActive\" = true ORDER BY \"Entry\" DESC LIMIT 1";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(code) };
            auto dt = conn->FetchPrepared(sql, params);
            if (dt.RowsCount() == 0) return std::nullopt;

            auto& row = dt[0];
            omnisphere::models::PaymentMethodDetail detail;
            detail.entry = (int)row["Entry"];
            detail.code = (std::string)row["Code"];
            detail.bankName = (std::string)row["BankName"];

            // El único dato que se desencripta es la CLABE
            auto safeDecode = [](const std::string& val) -> std::string {
                if (val.empty()) return "";
                try { return omnisphere::utils::Base64::Decode(val); } catch (...) { return val; }
            };
            detail.clabe = safeDecode((std::string)row["CLABE"]);

            detail.accountHolder = (std::string)row["AccountHolder"];
            if (row.HasColumn("PaymentReference") && !row["PaymentReference"].IsNull())
            {
                std::string ref = (std::string)row["PaymentReference"];
                detail.paymentReference = ref.empty() ? std::nullopt : std::make_optional(ref);
            }
            detail.isActive = (bool)row["IsActive"];
            if (row.HasColumn("CreatedBy") && !row["CreatedBy"].IsNull()) detail.createdBy = (std::string)row["CreatedBy"];
            if (row.HasColumn("CreateDate") && !row["CreateDate"].IsNull()) detail.createDate = (std::string)row["CreateDate"];
            if (row.HasColumn("LastUpdatedBy") && !row["LastUpdatedBy"].IsNull()) detail.lastUpdatedBy = (std::string)row["LastUpdatedBy"];
            if (row.HasColumn("UpdateDate") && !row["UpdateDate"].IsNull()) detail.updateDate = (std::string)row["UpdateDate"];

            return detail;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentMethodRepository::GetDetailByCode Exception] " << ex.what() << std::endl;
            return std::nullopt;
        }
    }

    bool PaymentMethodRepository::SaveDetail(const std::string& code, const omnisphere::dtos::PaymentMethodDetailInput& detailInput, const std::string& userId) const
    {
        if (!m_dbPool || code.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();

            // El único dato que se encripta es la CLABE
            std::string encClabe = omnisphere::utils::Base64::Encode(detailInput.Clabe);
            std::string rawBankName = detailInput.BankName;
            std::string rawAccountHolder = detailInput.AccountHolder;
            std::optional<std::string> rawPaymentRef = detailInput.PaymentReference;

            std::string checkSql = "SELECT \"Entry\" FROM \"PaymentMethodDetails\" WHERE \"Code\" = ? LIMIT 1";
            std::vector<omnisphere::types::SQLParam> checkParams = { omnisphere::types::MakeSQLParam(code) };
            auto checkDt = conn->FetchPrepared(checkSql, checkParams);

            if (checkDt.RowsCount() > 0)
            {
                std::string updateSql = "UPDATE \"PaymentMethodDetails\" SET "
                                        "\"BankName\" = ?, \"CLABE\" = ?, \"AccountHolder\" = ?, \"PaymentReference\" = ?, "
                                        "\"IsActive\" = true, \"LastUpdatedBy\" = ?, \"UpdateDate\" = CURRENT_TIMESTAMP "
                                        "WHERE \"Code\" = ?";
                std::vector<omnisphere::types::SQLParam> updateParams = {
                    omnisphere::types::MakeSQLParam(rawBankName),
                    omnisphere::types::MakeSQLParam(encClabe),
                    omnisphere::types::MakeSQLParam(rawAccountHolder),
                    omnisphere::types::MakeSQLParam(rawPaymentRef),
                    omnisphere::types::MakeSQLParam(userId),
                    omnisphere::types::MakeSQLParam(code)
                };
                return conn->RunPrepared(updateSql, updateParams);
            }
            else
            {
                std::string insertSql = "INSERT INTO \"PaymentMethodDetails\" "
                                        "(\"Code\", \"BankName\", \"CLABE\", \"AccountHolder\", \"PaymentReference\", \"IsActive\", \"CreatedBy\") "
                                        "VALUES (?, ?, ?, ?, ?, true, ?)";
                std::vector<omnisphere::types::SQLParam> insertParams = {
                    omnisphere::types::MakeSQLParam(code),
                    omnisphere::types::MakeSQLParam(rawBankName),
                    omnisphere::types::MakeSQLParam(encClabe),
                    omnisphere::types::MakeSQLParam(rawAccountHolder),
                    omnisphere::types::MakeSQLParam(rawPaymentRef),
                    omnisphere::types::MakeSQLParam(userId)
                };
                return conn->RunPrepared(insertSql, insertParams);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentMethodRepository::SaveDetail Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool PaymentMethodRepository::DeactivateDetail(const std::string& code, const std::string& userId) const
    {
        if (!m_dbPool || code.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = "UPDATE \"PaymentMethodDetails\" SET \"IsActive\" = false, \"LastUpdatedBy\" = ?, \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"Code\" = ?";
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(userId),
                omnisphere::types::MakeSQLParam(code)
            };
            return conn->RunPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentMethodRepository::DeactivateDetail Exception] " << ex.what() << std::endl;
            return false;
        }
    }
} // namespace omnisphere::repositories
