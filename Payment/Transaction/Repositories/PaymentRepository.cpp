#include "Payment/Transaction/Repositories/PaymentRepository.hpp"
#include "Identity/Repositories/IdentityRepository.hpp"
#include <OmniData/DataMapper.hpp>
#include <OmniUtils/Logger.hpp>
#include <iostream>
#include <algorithm>
#include <unordered_set>

namespace omnisphere::repositories
{
    PaymentRepository::PaymentRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    std::optional<omnisphere::models::Payment> PaymentRepository::Create(const omnisphere::dtos::CreatePaymentInput& input) const
    {
        if (!m_dbPool) return std::nullopt;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            IdentityRepository identityRepo(m_dbPool);

            std::string upperType = input.Type;
            std::transform(upperType.begin(), upperType.end(), upperType.begin(), ::toupper);

            std::string targetTable;
            std::string targetDomain;
            std::string prefix;
            if (upperType == "TRANSFER" || upperType == "SPEI")
            {
                targetTable = "\"TransferTransactions\"";
                targetDomain = "TransferTransactions";
                prefix = "TRF";
            }
            else if (upperType == "CASH")
            {
                targetTable = "\"CashTransactions\"";
                targetDomain = "CashTransactions";
                prefix = "CSH";
            }
            else
            {
                targetTable = "\"CardTransactions\"";
                targetDomain = "CardTransactions";
                prefix = "CRD";
            }

            std::string code = input.Code ? *input.Code : identityRepo.GetNextCode(conn, targetDomain, prefix);

            std::vector<omnisphere::types::ColumnValue> cols = {
                {"\"Code\"", omnisphere::types::MakeSQLParam(code)},
                {"\"PaymentCode\"", omnisphere::types::MakeSQLParam(input.PaymentCode)},
                {"\"EntityType\"", omnisphere::types::MakeSQLParam(input.EntityType)},
                {"\"EntityCode\"", omnisphere::types::MakeSQLParam(input.EntityCode)},
                {"\"Amount\"", omnisphere::types::MakeSQLParam(input.Amount)},
                {"\"Currency\"", omnisphere::types::MakeSQLParam(input.Currency)},
                {"\"Status\"", omnisphere::types::MakeSQLParam(input.Status)},
                {"\"IsActive\"", omnisphere::types::MakeSQLParam(input.IsActive)},
                {"\"CreatedBy\"", omnisphere::types::MakeSQLParam(input.CreatedBy.empty() ? "system" : input.CreatedBy)}
            };

            if (input.ExpiresAt) cols.push_back({"\"ExpiresAt\"", omnisphere::types::MakeSQLParam(*input.ExpiresAt)});

            if (targetTable == "\"TransferTransactions\"")
            {
                if (input.BankName) cols.push_back({"\"BankName\"", omnisphere::types::MakeSQLParam(*input.BankName)});
                if (input.Clabe) cols.push_back({"\"Clabe\"", omnisphere::types::MakeSQLParam(*input.Clabe)});
                if (input.AccountHolder) cols.push_back({"\"AccountHolder\"", omnisphere::types::MakeSQLParam(*input.AccountHolder)});
                if (input.PaymentReference) cols.push_back({"\"PaymentReference\"", omnisphere::types::MakeSQLParam(*input.PaymentReference)});
                if (input.ReceiptUrl) cols.push_back({"\"ReceiptUrl\"", omnisphere::types::MakeSQLParam(*input.ReceiptUrl)});
            }
            else if (targetTable == "\"CashTransactions\"")
            {
                if (input.CashReceived) cols.push_back({"\"CashReceived\"", omnisphere::types::MakeSQLParam(*input.CashReceived)});
                if (input.CashChange) cols.push_back({"\"CashChange\"", omnisphere::types::MakeSQLParam(*input.CashChange)});
                if (input.ReceivedBy) cols.push_back({"\"ReceivedBy\"", omnisphere::types::MakeSQLParam(*input.ReceivedBy)});
                if (input.Location) cols.push_back({"\"Location\"", omnisphere::types::MakeSQLParam(*input.Location)});
                if (input.ReceiptNumber) cols.push_back({"\"ReceiptNumber\"", omnisphere::types::MakeSQLParam(*input.ReceiptNumber)});
                if (input.Notes) cols.push_back({"\"Notes\"", omnisphere::types::MakeSQLParam(*input.Notes)});
            }
            else // CardTransactions
            {
                std::string prov = input.Provider.value_or("STRIPE");
                cols.push_back({"\"Provider\"", omnisphere::types::MakeSQLParam(prov)});
                if (input.PaymentIntentId) cols.push_back({"\"PaymentIntentId\"", omnisphere::types::MakeSQLParam(*input.PaymentIntentId)});
                if (input.SessionId) cols.push_back({"\"SessionId\"", omnisphere::types::MakeSQLParam(*input.SessionId)});
                if (input.CardLast4) cols.push_back({"\"CardLast4\"", omnisphere::types::MakeSQLParam(*input.CardLast4)});
                if (input.CardBrand) cols.push_back({"\"CardBrand\"", omnisphere::types::MakeSQLParam(*input.CardBrand)});
                if (input.CardHolderName) cols.push_back({"\"CardHolderName\"", omnisphere::types::MakeSQLParam(*input.CardHolderName)});
                if (input.AuthorizationCode) cols.push_back({"\"AuthorizationCode\"", omnisphere::types::MakeSQLParam(*input.AuthorizationCode)});
                if (input.ReceiptUrl) cols.push_back({"\"ReceiptUrl\"", omnisphere::types::MakeSQLParam(*input.ReceiptUrl)});
                if (input.HostedUrl) cols.push_back({"\"HostedUrl\"", omnisphere::types::MakeSQLParam(*input.HostedUrl)});
                if (input.RawPayload) cols.push_back({"\"RawPayload\"", omnisphere::types::MakeSQLParam(*input.RawPayload)});
            }

            std::vector<std::string> colNames;
            std::vector<omnisphere::types::SQLParam> params;
            colNames.reserve(cols.size());
            params.reserve(cols.size());
            for (const auto& c : cols)
            {
                colNames.push_back(c.Column);
                params.push_back(c.Value);
            }
            std::string insertSql = omnisphere::types::BuildInsertQuery(targetTable, colNames);
            if (!conn->RunPrepared(insertSql, params))
            {
                conn->RollbackTransaction();
                return std::nullopt;
            }

            conn->CommitTransaction();
            return GetByCode(code);
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[PaymentRepository::Create Exception] " << ex.what() << std::endl;
            return std::nullopt;
        }
    }

    bool PaymentRepository::Update(const omnisphere::dtos::UpdatePaymentInput& input, const std::string& type) const
    {
        if (!m_dbPool || input.Code.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();

            std::string upperType = type;
            std::transform(upperType.begin(), upperType.end(), upperType.begin(), ::toupper);

            std::string targetTable;
            if (upperType == "TRANSFER" || upperType == "SPEI") targetTable = "\"TransferTransactions\"";
            else if (upperType == "CASH") targetTable = "\"CashTransactions\"";
            else targetTable = "\"CardTransactions\"";

            std::vector<omnisphere::types::ColumnValue> updateCols;

            if (input.PaymentCode) updateCols.push_back({"\"PaymentCode\"", omnisphere::types::MakeSQLParam(*input.PaymentCode)});
            if (input.Status) updateCols.push_back({"\"Status\"", omnisphere::types::MakeSQLParam(*input.Status)});
            if (input.IsActive) updateCols.push_back({"\"IsActive\"", omnisphere::types::MakeSQLParam(*input.IsActive)});
            if (input.ExpiresAt) updateCols.push_back({"\"ExpiresAt\"", omnisphere::types::MakeSQLParam(*input.ExpiresAt)});

            if (targetTable == "\"TransferTransactions\"")
            {
                if (input.BankName) updateCols.push_back({"\"BankName\"", omnisphere::types::MakeSQLParam(*input.BankName)});
                if (input.Clabe) updateCols.push_back({"\"Clabe\"", omnisphere::types::MakeSQLParam(*input.Clabe)});
                if (input.AccountHolder) updateCols.push_back({"\"AccountHolder\"", omnisphere::types::MakeSQLParam(*input.AccountHolder)});
                if (input.PaymentReference) updateCols.push_back({"\"PaymentReference\"", omnisphere::types::MakeSQLParam(*input.PaymentReference)});
                if (input.ReceiptUrl) updateCols.push_back({"\"ReceiptUrl\"", omnisphere::types::MakeSQLParam(*input.ReceiptUrl)});
            }
            else if (targetTable == "\"CashTransactions\"")
            {
                if (input.CashReceived) updateCols.push_back({"\"CashReceived\"", omnisphere::types::MakeSQLParam(*input.CashReceived)});
                if (input.CashChange) updateCols.push_back({"\"CashChange\"", omnisphere::types::MakeSQLParam(*input.CashChange)});
                if (input.ReceivedBy) updateCols.push_back({"\"ReceivedBy\"", omnisphere::types::MakeSQLParam(*input.ReceivedBy)});
                if (input.Location) updateCols.push_back({"\"Location\"", omnisphere::types::MakeSQLParam(*input.Location)});
                if (input.ReceiptNumber) updateCols.push_back({"\"ReceiptNumber\"", omnisphere::types::MakeSQLParam(*input.ReceiptNumber)});
                if (input.Notes) updateCols.push_back({"\"Notes\"", omnisphere::types::MakeSQLParam(*input.Notes)});
            }
            else // CardTransactions
            {
                if (input.Provider) updateCols.push_back({"\"Provider\"", omnisphere::types::MakeSQLParam(*input.Provider)});
                if (input.PaymentIntentId) updateCols.push_back({"\"PaymentIntentId\"", omnisphere::types::MakeSQLParam(*input.PaymentIntentId)});
                if (input.SessionId) updateCols.push_back({"\"SessionId\"", omnisphere::types::MakeSQLParam(*input.SessionId)});
                if (input.CardLast4) updateCols.push_back({"\"CardLast4\"", omnisphere::types::MakeSQLParam(*input.CardLast4)});
                if (input.CardBrand) updateCols.push_back({"\"CardBrand\"", omnisphere::types::MakeSQLParam(*input.CardBrand)});
                if (input.CardHolderName) updateCols.push_back({"\"CardHolderName\"", omnisphere::types::MakeSQLParam(*input.CardHolderName)});
                if (input.AuthorizationCode) updateCols.push_back({"\"AuthorizationCode\"", omnisphere::types::MakeSQLParam(*input.AuthorizationCode)});
                if (input.ReceiptUrl) updateCols.push_back({"\"ReceiptUrl\"", omnisphere::types::MakeSQLParam(*input.ReceiptUrl)});
                if (input.HostedUrl) updateCols.push_back({"\"HostedUrl\"", omnisphere::types::MakeSQLParam(*input.HostedUrl)});
                if (input.RawPayload) updateCols.push_back({"\"RawPayload\"", omnisphere::types::MakeSQLParam(*input.RawPayload)});
            }

            updateCols.push_back({"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(input.LastUpdatedBy.value_or("system"))});
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::gmtime(&in_time_t));
            updateCols.push_back({"\"UpdateDate\"", omnisphere::types::MakeSQLParam(input.UpdateDate.value_or(std::string(buf)))});

            auto updateResult = omnisphere::types::BuildUpdateQuery(
                targetTable, updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(input.Code)
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
            std::cerr << "[PaymentRepository::Update Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool PaymentRepository::Delete(const std::string& code, const std::string& type) const
    {
        if (!m_dbPool || code.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            std::string upperType = type;
            std::transform(upperType.begin(), upperType.end(), upperType.begin(), ::toupper);

            std::string targetTable;
            if (upperType == "TRANSFER" || upperType == "SPEI") targetTable = "\"TransferTransactions\"";
            else if (upperType == "CASH") targetTable = "\"CashTransactions\"";
            else targetTable = "\"CardTransactions\"";

            std::vector<omnisphere::types::ColumnValue> updateCols = {
                {"\"IsActive\"", omnisphere::types::MakeSQLParam(false)}
            };
            auto updateResult = omnisphere::types::BuildUpdateQuery(
                targetTable, updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(code)
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
            std::cerr << "[PaymentRepository::Delete Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    std::optional<omnisphere::models::Payment> PaymentRepository::GetByCode(const std::string& code, const std::vector<std::string>& requestedFields) const
    {
        if (!m_dbPool || code.empty()) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<omnisphere::types::Condition> conds = {
                {"", "\"Code\"", "=", "?"},
                {"", "\"IsActive\"", "=", "?"}
            };
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(code),
                omnisphere::types::MakeSQLParam(true)
            };

            auto tFields = omnisphere::types::FilterModelFields<omnisphere::models::TransferTransaction>(requestedFields);
            auto qpT = omnisphere::types::BuildQueryParts(tFields, conds);
            auto dt = conn->FetchPrepared("SELECT " + qpT.SelectClause + " FROM \"TransferTransactions\" WHERE " + qpT.WhereClause + " LIMIT 1", params);
            if (!dt.IsEmpty())
            {
                auto p = omnisphere::types::FromDataRow<omnisphere::models::Payment>(dt[0]);
                p.type = "TRANSFER";
                return p;
            }

            auto cFields = omnisphere::types::FilterModelFields<omnisphere::models::CashTransaction>(requestedFields);
            auto qpC = omnisphere::types::BuildQueryParts(cFields, conds);
            dt = conn->FetchPrepared("SELECT " + qpC.SelectClause + " FROM \"CashTransactions\" WHERE " + qpC.WhereClause + " LIMIT 1", params);
            if (!dt.IsEmpty())
            {
                auto p = omnisphere::types::FromDataRow<omnisphere::models::Payment>(dt[0]);
                p.type = "CASH";
                return p;
            }

            auto cdFields = omnisphere::types::FilterModelFields<omnisphere::models::CardTransaction>(requestedFields);
            auto qpCd = omnisphere::types::BuildQueryParts(cdFields, conds);
            dt = conn->FetchPrepared("SELECT " + qpCd.SelectClause + " FROM \"CardTransactions\" WHERE " + qpCd.WhereClause + " LIMIT 1", params);
            if (!dt.IsEmpty())
            {
                auto p = omnisphere::types::FromDataRow<omnisphere::models::Payment>(dt[0]);
                p.type = "CARD";
                return p;
            }

            return std::nullopt;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentRepository::GetByCode Exception] " << ex.what() << std::endl;
            return std::nullopt;
        }
    }

    std::optional<omnisphere::models::Payment> PaymentRepository::GetByEntity(
        const std::string& entityType,
        const std::string& entityCode,
        const std::optional<std::string>& paymentTypeHint,
        const std::vector<std::string>& requestedFields
    ) const
    {
        if (!m_dbPool || entityCode.empty()) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();

            std::string whereClause;
            std::vector<omnisphere::types::SQLParam> params;

            if (entityType == "ROUTE_RESERVATION" || entityType == "RESERVATION")
            {
                whereClause = "\"EntityType\" IN ('ROUTE_RESERVATION', 'RESERVATION') AND \"EntityCode\" = ? AND \"IsActive\" = ?";
                params = {
                    omnisphere::types::MakeSQLParam(entityCode),
                    omnisphere::types::MakeSQLParam(true)
                };
            }
            else if (!entityType.empty())
            {
                whereClause = "\"EntityType\" = ? AND \"EntityCode\" = ? AND \"IsActive\" = ?";
                params = {
                    omnisphere::types::MakeSQLParam(entityType),
                    omnisphere::types::MakeSQLParam(entityCode),
                    omnisphere::types::MakeSQLParam(true)
                };
            }
            else
            {
                whereClause = "\"EntityCode\" = ? AND \"IsActive\" = ?";
                params = {
                    omnisphere::types::MakeSQLParam(entityCode),
                    omnisphere::types::MakeSQLParam(true)
                };
            }

            std::string normType = paymentTypeHint.value_or("");
            std::transform(normType.begin(), normType.end(), normType.begin(), ::toupper);

            if (normType == "NOT_APPLICABLE") return std::nullopt;

            bool checkTransfer = normType.empty() || normType == "TRANSFER" || normType == "SPEI" || normType == "PMT2";
            bool checkCash     = normType.empty() || normType == "CASH" || normType == "PMT1";
            bool checkCard     = normType.empty() || normType == "CARD" || normType == "STRIPE" || normType == "PMT3";

            if (!checkTransfer && !checkCash && !checkCard)
            {
                checkTransfer = true;
                checkCash = true;
                checkCard = true;
            }

            std::vector<omnisphere::types::Condition> dummyConds;

            if (checkTransfer)
            {
                auto tFields = omnisphere::types::FilterModelFields<omnisphere::models::TransferTransaction>(requestedFields);
                auto qpT = omnisphere::types::BuildQueryParts(tFields, dummyConds);
                auto dt = conn->FetchPrepared("SELECT " + qpT.SelectClause + " FROM \"TransferTransactions\" WHERE " + whereClause + " ORDER BY \"Entry\" DESC LIMIT 1", params);
                if (!dt.IsEmpty())
                {
                    auto p = omnisphere::types::FromDataRow<omnisphere::models::Payment>(dt[0]);
                    p.type = "TRANSFER";
                    return p;
                }
            }

            if (checkCash)
            {
                auto cFields = omnisphere::types::FilterModelFields<omnisphere::models::CashTransaction>(requestedFields);
                auto qpC = omnisphere::types::BuildQueryParts(cFields, dummyConds);
                auto dt = conn->FetchPrepared("SELECT " + qpC.SelectClause + " FROM \"CashTransactions\" WHERE " + whereClause + " ORDER BY \"Entry\" DESC LIMIT 1", params);
                if (!dt.IsEmpty())
                {
                    auto p = omnisphere::types::FromDataRow<omnisphere::models::Payment>(dt[0]);
                    p.type = "CASH";
                    return p;
                }
            }

            if (checkCard)
            {
                auto cdFields = omnisphere::types::FilterModelFields<omnisphere::models::CardTransaction>(requestedFields);
                auto qpCd = omnisphere::types::BuildQueryParts(cdFields, dummyConds);
                auto dt = conn->FetchPrepared("SELECT " + qpCd.SelectClause + " FROM \"CardTransactions\" WHERE " + whereClause + " ORDER BY \"Entry\" DESC LIMIT 1", params);
                if (!dt.IsEmpty())
                {
                    auto p = omnisphere::types::FromDataRow<omnisphere::models::Payment>(dt[0]);
                    p.type = "CARD";
                    return p;
                }
            }

            return std::nullopt;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentRepository::GetByEntity Exception] " << ex.what() << std::endl;
            return std::nullopt;
        }
    }

    std::vector<omnisphere::models::Payment> PaymentRepository::GetAll(const std::optional<std::string>& entityType, const std::optional<std::string>& entityCode, const std::vector<std::string>& requestedFields) const
    {
        std::vector<omnisphere::models::Payment> results;
        if (!m_dbPool) return results;
        try
        {
            auto conn = m_dbPool->Acquire();

            std::string where = " WHERE \"IsActive\" = ?";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(true) };

            if (entityType && !entityType->empty())
            {
                if (*entityType == "ROUTE_RESERVATION" || *entityType == "RESERVATION")
                {
                    where += " AND \"EntityType\" IN ('ROUTE_RESERVATION', 'RESERVATION')";
                }
                else
                {
                    where += " AND \"EntityType\" = ?";
                    params.push_back(omnisphere::types::MakeSQLParam(*entityType));
                }
            }
            if (entityCode && !entityCode->empty())
            {
                where += " AND \"EntityCode\" = ?";
                params.push_back(omnisphere::types::MakeSQLParam(*entityCode));
            }

            std::vector<omnisphere::types::Condition> dummyConds;

            auto tFields = omnisphere::types::FilterModelFields<omnisphere::models::TransferTransaction>(requestedFields);
            auto qpT = omnisphere::types::BuildQueryParts(tFields, dummyConds);
            auto dt1 = conn->FetchPrepared("SELECT " + qpT.SelectClause + " FROM \"TransferTransactions\"" + where + " ORDER BY \"Entry\" DESC", params);
            for (const auto& row : dt1)
            {
                auto p = omnisphere::types::FromDataRow<omnisphere::models::Payment>(row);
                p.type = "TRANSFER";
                results.push_back(std::move(p));
            }

            auto cFields = omnisphere::types::FilterModelFields<omnisphere::models::CashTransaction>(requestedFields);
            auto qpC = omnisphere::types::BuildQueryParts(cFields, dummyConds);
            auto dt2 = conn->FetchPrepared("SELECT " + qpC.SelectClause + " FROM \"CashTransactions\"" + where + " ORDER BY \"Entry\" DESC", params);
            for (const auto& row : dt2)
            {
                auto p = omnisphere::types::FromDataRow<omnisphere::models::Payment>(row);
                p.type = "CASH";
                results.push_back(std::move(p));
            }

            auto cdFields = omnisphere::types::FilterModelFields<omnisphere::models::CardTransaction>(requestedFields);
            auto qpCd = omnisphere::types::BuildQueryParts(cdFields, dummyConds);
            auto dt3 = conn->FetchPrepared("SELECT " + qpCd.SelectClause + " FROM \"CardTransactions\"" + where + " ORDER BY \"Entry\" DESC", params);
            for (const auto& row : dt3)
            {
                auto p = omnisphere::types::FromDataRow<omnisphere::models::Payment>(row);
                p.type = "CARD";
                results.push_back(std::move(p));
            }

            return results;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentRepository::GetAll Exception] " << ex.what() << std::endl;
            return results;
        }
    }
} // namespace omnisphere::repositories
