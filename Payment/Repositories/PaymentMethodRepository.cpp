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
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();

            omnisphere::dtos::CreatePaymentMethodInput tempInput = input;
            IdentityRepository identityRepo(m_dbPool);
            tempInput.Code = identityRepo.GetNextCode(conn, "PaymentMethod", "PMT");
            const_cast<omnisphere::dtos::CreatePaymentMethodInput&>(input).Code = tempInput.Code;

            auto insertResult = omnisphere::types::BuildInsertQuery("\"PaymentMethods\"", 0, tempInput, mutationFields);
            bool ok = conn->RunPrepared(insertResult.Query, insertResult.Parameters);
            if (!ok)
            {
                conn->RollbackTransaction();
                return false;
            }

            if (!input.UsesIntegration && input.Details.has_value())
            {
                if (!SaveDetail(tempInput.Code, *input.Details, input.CreatedBy))
                {
                    conn->RollbackTransaction();
                    return false;
                }
            }

            conn->CommitTransaction();
            return true;
        }
        catch (const std::runtime_error&)
        {
            conn->RollbackTransaction();
            throw;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[PaymentMethodRepository::Create Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool PaymentMethodRepository::Update(const omnisphere::dtos::UpdatePaymentMethodInput& input, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        std::string targetCode = input.Code;
        if (targetCode.empty() && input.Entry > 0)
        {
            // Fallback de retrocompatibilidad solo si Code viene vacío
            auto curDt = GetByEntry(input.Entry, {"\"Code\""});
            if (!curDt.IsEmpty())
            {
                targetCode = curDt[0]["Code"].GetOptional<std::string>().value_or("");
            }
        }
        if (targetCode.empty()) return false;

        if (input.UsesIntegration.value_or(false) && input.Details.has_value())
        {
            throw std::runtime_error("No se permiten detalles de transferencia para formas de pago que usan integración");
        }
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();

            std::string code = targetCode;
            bool currentUsesIntegration = false;

            std::string getCodeSql = "SELECT \"Code\", \"UsesIntegration\" FROM \"PaymentMethods\" WHERE \"Code\" = ? LIMIT 1";
            std::vector<omnisphere::types::SQLParam> getParams = { omnisphere::types::MakeSQLParam(targetCode) };

            auto curDt = conn->FetchPrepared(getCodeSql, getParams);
            if (!curDt.IsEmpty())
            {
                const auto& curRow = curDt[0];
                code = curRow["Code"].GetOptional<std::string>().value_or(targetCode);
                currentUsesIntegration = curRow["UsesIntegration"].GetOptional<bool>().value_or(false);
            }
            else
            {
                conn->RollbackTransaction();
                return false;
            }

            bool effectivelyUsesIntegration = input.UsesIntegration.value_or(currentUsesIntegration);
            if (effectivelyUsesIntegration && input.Details.has_value())
            {
                conn->RollbackTransaction();
                throw std::runtime_error("No se permiten detalles de transferencia para formas de pago que usan integración");
            }

            auto updateCols = omnisphere::types::ExtractUpdateColumns(input, mutationFields);
            if (!updateCols.empty())
            {
                updateCols.push_back({"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(input.LastUpdatedBy)});

                auto updateResult = omnisphere::types::BuildUpdateQuery(
                    "\"PaymentMethods\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(code)
                );
                if (!conn->RunPrepared(updateResult.Query, updateResult.Parameters))
                {
                    conn->RollbackTransaction();
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

            conn->CommitTransaction();
            return true;
        }
        catch (const std::runtime_error&)
        {
            conn->RollbackTransaction();
            throw;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[PaymentMethodRepository::Update Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool PaymentMethodRepository::Delete(int entry) const
    {
        if (!m_dbPool || entry <= 0) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            std::vector<omnisphere::types::ColumnValue> updateCols = {
                {"\"IsActive\"", omnisphere::types::MakeSQLParam(false)}
            };
            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"PaymentMethods\"", updateCols, "\"Entry\"", omnisphere::types::MakeSQLParam(entry)
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
            std::cerr << "[PaymentMethodRepository::Delete Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool PaymentMethodRepository::DeleteByCode(const std::string& code) const
    {
        if (!m_dbPool || code.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            std::vector<omnisphere::types::ColumnValue> updateCols = {
                {"\"IsActive\"", omnisphere::types::MakeSQLParam(false)}
            };
            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"PaymentMethods\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(code)
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
            std::cerr << "[PaymentMethodRepository::DeleteByCode Exception] " << ex.what() << std::endl;
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
            if (dt.IsEmpty()) return std::nullopt;

            auto detail = omnisphere::types::FromDataRow<omnisphere::models::PaymentMethodDetail>(dt[0]);
            if (!detail.clabe.empty())
            {
                try { detail.clabe = omnisphere::utils::Base64::Decode(detail.clabe); } catch (...) {}
            }
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

            if (!checkDt.IsEmpty())
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
