#include "Payment/Transaction/Repositories/PaymentRepository.hpp"
#include "Identity/Repositories/IdentityRepository.hpp"
#include <OmniUtils/Logger.hpp>
#include <iostream>
#include <algorithm>

namespace omnisphere::repositories
{
    PaymentRepository::PaymentRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    omnisphere::models::Payment PaymentRepository::MapTransferRow(omnisphere::types::DataTable::Row& row)
    {
        omnisphere::models::Payment p;
        p.entry = static_cast<int>(row["Entry"]);
        p.code = (std::string)row["Code"];
        p.paymentCode = (std::string)row["PaymentCode"];
        p.type = "TRANSFER";
        p.entityType = row.HasColumn("EntityType") ? (std::string)row["EntityType"] : "ROUTE_RESERVATION";
        p.entityCode = (std::string)row["EntityCode"];
        p.amount = row.HasColumn("Amount") ? (double)row["Amount"] : 0.0;
        p.currency = row.HasColumn("Currency") ? (std::string)row["Currency"] : "mxn";
        p.status = row.HasColumn("Status") ? (std::string)row["Status"] : "PENDING";

        if (row.HasColumn("BankName") && !row["BankName"].IsNull()) p.bankName = (std::string)row["BankName"];
        if (row.HasColumn("Clabe") && !row["Clabe"].IsNull()) p.clabe = (std::string)row["Clabe"];
        if (row.HasColumn("AccountHolder") && !row["AccountHolder"].IsNull()) p.accountHolder = (std::string)row["AccountHolder"];
        if (row.HasColumn("PaymentReference") && !row["PaymentReference"].IsNull()) p.paymentReference = (std::string)row["PaymentReference"];
        if (row.HasColumn("ReceiptUrl") && !row["ReceiptUrl"].IsNull()) p.receiptUrl = (std::string)row["ReceiptUrl"];

        if (row.HasColumn("ExpiresAt") && !row["ExpiresAt"].IsNull()) p.expiresAt = (std::string)row["ExpiresAt"];
        p.isActive = row.HasColumn("IsActive") ? (bool)row["IsActive"] : true;
        p.createdBy = row.HasColumn("CreatedBy") ? (std::string)row["CreatedBy"] : "system";
        if (row.HasColumn("CreateDate") && !row["CreateDate"].IsNull()) p.createDate = (std::string)row["CreateDate"];
        if (row.HasColumn("LastUpdatedBy") && !row["LastUpdatedBy"].IsNull()) p.lastUpdatedBy = (std::string)row["LastUpdatedBy"];
        if (row.HasColumn("UpdateDate") && !row["UpdateDate"].IsNull()) p.updateDate = (std::string)row["UpdateDate"];

        return p;
    }

    omnisphere::models::Payment PaymentRepository::MapCashRow(omnisphere::types::DataTable::Row& row)
    {
        omnisphere::models::Payment p;
        p.entry = static_cast<int>(row["Entry"]);
        p.code = (std::string)row["Code"];
        p.paymentCode = (std::string)row["PaymentCode"];
        p.type = "CASH";
        p.entityType = row.HasColumn("EntityType") ? (std::string)row["EntityType"] : "ROUTE_RESERVATION";
        p.entityCode = (std::string)row["EntityCode"];
        p.amount = row.HasColumn("Amount") ? (double)row["Amount"] : 0.0;
        p.currency = row.HasColumn("Currency") ? (std::string)row["Currency"] : "mxn";
        p.status = row.HasColumn("Status") ? (std::string)row["Status"] : "PENDING";

        if (row.HasColumn("CashReceived") && !row["CashReceived"].IsNull()) p.cashReceived = (double)row["CashReceived"];
        if (row.HasColumn("CashChange") && !row["CashChange"].IsNull()) p.cashChange = (double)row["CashChange"];
        if (row.HasColumn("ReceivedBy") && !row["ReceivedBy"].IsNull()) p.receivedBy = (std::string)row["ReceivedBy"];
        if (row.HasColumn("Location") && !row["Location"].IsNull()) p.location = (std::string)row["Location"];
        if (row.HasColumn("ReceiptNumber") && !row["ReceiptNumber"].IsNull()) p.receiptNumber = (std::string)row["ReceiptNumber"];
        if (row.HasColumn("Notes") && !row["Notes"].IsNull()) p.notes = (std::string)row["Notes"];

        if (row.HasColumn("ExpiresAt") && !row["ExpiresAt"].IsNull()) p.expiresAt = (std::string)row["ExpiresAt"];
        p.isActive = row.HasColumn("IsActive") ? (bool)row["IsActive"] : true;
        p.createdBy = row.HasColumn("CreatedBy") ? (std::string)row["CreatedBy"] : "system";
        if (row.HasColumn("CreateDate") && !row["CreateDate"].IsNull()) p.createDate = (std::string)row["CreateDate"];
        if (row.HasColumn("LastUpdatedBy") && !row["LastUpdatedBy"].IsNull()) p.lastUpdatedBy = (std::string)row["LastUpdatedBy"];
        if (row.HasColumn("UpdateDate") && !row["UpdateDate"].IsNull()) p.updateDate = (std::string)row["UpdateDate"];

        return p;
    }

    omnisphere::models::Payment PaymentRepository::MapCardRow(omnisphere::types::DataTable::Row& row)
    {
        omnisphere::models::Payment p;
        p.entry = static_cast<int>(row["Entry"]);
        p.code = (std::string)row["Code"];
        p.paymentCode = (std::string)row["PaymentCode"];
        p.type = "CARD";
        p.entityType = row.HasColumn("EntityType") ? (std::string)row["EntityType"] : "ROUTE_RESERVATION";
        p.entityCode = (std::string)row["EntityCode"];
        p.amount = row.HasColumn("Amount") ? (double)row["Amount"] : 0.0;
        p.currency = row.HasColumn("Currency") ? (std::string)row["Currency"] : "mxn";
        p.status = row.HasColumn("Status") ? (std::string)row["Status"] : "PENDING";

        if (row.HasColumn("Provider") && !row["Provider"].IsNull()) p.provider = (std::string)row["Provider"];
        if (row.HasColumn("PaymentIntentId") && !row["PaymentIntentId"].IsNull()) p.paymentIntentId = (std::string)row["PaymentIntentId"];
        if (row.HasColumn("SessionId") && !row["SessionId"].IsNull()) p.sessionId = (std::string)row["SessionId"];
        if (row.HasColumn("CardLast4") && !row["CardLast4"].IsNull()) p.cardLast4 = (std::string)row["CardLast4"];
        if (row.HasColumn("CardBrand") && !row["CardBrand"].IsNull()) p.cardBrand = (std::string)row["CardBrand"];
        if (row.HasColumn("CardHolderName") && !row["CardHolderName"].IsNull()) p.cardHolderName = (std::string)row["CardHolderName"];
        if (row.HasColumn("AuthorizationCode") && !row["AuthorizationCode"].IsNull()) p.authorizationCode = (std::string)row["AuthorizationCode"];
        if (row.HasColumn("ReceiptUrl") && !row["ReceiptUrl"].IsNull()) p.receiptUrl = (std::string)row["ReceiptUrl"];
        if (row.HasColumn("HostedUrl") && !row["HostedUrl"].IsNull()) p.hostedUrl = (std::string)row["HostedUrl"];
        if (row.HasColumn("RawPayload") && !row["RawPayload"].IsNull()) p.rawPayload = (std::string)row["RawPayload"];

        if (row.HasColumn("ExpiresAt") && !row["ExpiresAt"].IsNull()) p.expiresAt = (std::string)row["ExpiresAt"];
        p.isActive = row.HasColumn("IsActive") ? (bool)row["IsActive"] : true;
        p.createdBy = row.HasColumn("CreatedBy") ? (std::string)row["CreatedBy"] : "system";
        if (row.HasColumn("CreateDate") && !row["CreateDate"].IsNull()) p.createDate = (std::string)row["CreateDate"];
        if (row.HasColumn("LastUpdatedBy") && !row["LastUpdatedBy"].IsNull()) p.lastUpdatedBy = (std::string)row["LastUpdatedBy"];
        if (row.HasColumn("UpdateDate") && !row["UpdateDate"].IsNull()) p.updateDate = (std::string)row["UpdateDate"];

        return p;
    }

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
            std::string prefix;
            if (upperType == "TRANSFER" || upperType == "SPEI")
            {
                targetTable = "\"TransferTransactions\"";
                prefix = "TRF";
            }
            else if (upperType == "CASH")
            {
                targetTable = "\"CashTransactions\"";
                prefix = "CSH";
            }
            else
            {
                targetTable = "\"CardTransactions\"";
                prefix = "CRD";
            }

            std::string code = input.Code ? *input.Code : identityRepo.GetNextCode(conn, targetTable, prefix);

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
            auto fields = omnisphere::types::FilterModelFields<omnisphere::models::Payment>(requestedFields);
            std::vector<omnisphere::types::Condition> conds = {
                {"", "\"Code\"", "=", "?"},
                {"", "\"IsActive\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(fields, conds);
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(code),
                omnisphere::types::MakeSQLParam(true)
            };

            auto dt = conn->FetchPrepared("SELECT " + qp.SelectClause + " FROM \"TransferTransactions\" WHERE " + qp.WhereClause + " LIMIT 1", params);
            if (!dt.IsEmpty()) return MapTransferRow(dt[0]);

            dt = conn->FetchPrepared("SELECT " + qp.SelectClause + " FROM \"CashTransactions\" WHERE " + qp.WhereClause + " LIMIT 1", params);
            if (!dt.IsEmpty()) return MapCashRow(dt[0]);

            dt = conn->FetchPrepared("SELECT " + qp.SelectClause + " FROM \"CardTransactions\" WHERE " + qp.WhereClause + " LIMIT 1", params);
            if (!dt.IsEmpty()) return MapCardRow(dt[0]);

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
            auto rawFields = omnisphere::types::FilterModelFields<omnisphere::models::Payment>(requestedFields);
            std::vector<std::string> fields;
            for (const auto& f : rawFields) {
                if (f != "Type" && f != "\"Type\"" && f != "PaymentCode" && f != "\"PaymentCode\"") {
                    fields.push_back(f);
                }
            }

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

            std::vector<omnisphere::types::Condition> dummyConds;
            auto qp = omnisphere::types::BuildQueryParts(fields, dummyConds);

            std::string normType = paymentTypeHint.value_or("");
            std::transform(normType.begin(), normType.end(), normType.begin(), ::toupper);

            bool checkTransfer = normType.empty() || normType == "TRANSFER" || normType == "SPEI";
            bool checkCash     = normType.empty() || normType == "CASH";
            bool checkCard     = normType.empty() || normType == "CARD" || normType == "STRIPE";

            if (!checkTransfer && !checkCash && !checkCard)
            {
                checkTransfer = true;
                checkCash = true;
                checkCard = true;
            }

            if (checkTransfer)
            {
                auto dt = conn->FetchPrepared("SELECT " + qp.SelectClause + " FROM \"TransferTransactions\" WHERE " + whereClause + " ORDER BY \"Entry\" DESC LIMIT 1", params);
                if (!dt.IsEmpty()) return MapTransferRow(dt[0]);
            }

            if (checkCash)
            {
                auto dt = conn->FetchPrepared("SELECT " + qp.SelectClause + " FROM \"CashTransactions\" WHERE " + whereClause + " ORDER BY \"Entry\" DESC LIMIT 1", params);
                if (!dt.IsEmpty()) return MapCashRow(dt[0]);
            }

            if (checkCard)
            {
                auto dt = conn->FetchPrepared("SELECT " + qp.SelectClause + " FROM \"CardTransactions\" WHERE " + whereClause + " ORDER BY \"Entry\" DESC LIMIT 1", params);
                if (!dt.IsEmpty()) return MapCardRow(dt[0]);
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
            auto fields = omnisphere::types::FilterModelFields<omnisphere::models::Payment>(requestedFields);
            std::vector<omnisphere::types::Condition> dummyConds;
            auto qp = omnisphere::types::BuildQueryParts(fields, dummyConds);

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

            auto dt1 = conn->FetchPrepared("SELECT " + qp.SelectClause + " FROM \"TransferTransactions\"" + where + " ORDER BY \"Entry\" DESC", params);
            for (auto& row : dt1) results.push_back(MapTransferRow(row));

            auto dt2 = conn->FetchPrepared("SELECT " + qp.SelectClause + " FROM \"CashTransactions\"" + where + " ORDER BY \"Entry\" DESC", params);
            for (auto& row : dt2) results.push_back(MapCashRow(row));

            auto dt3 = conn->FetchPrepared("SELECT " + qp.SelectClause + " FROM \"CardTransactions\"" + where + " ORDER BY \"Entry\" DESC", params);
            for (auto& row : dt3) results.push_back(MapCardRow(row));

            return results;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[PaymentRepository::GetAll Exception] " << ex.what() << std::endl;
            return results;
        }
    }
} // namespace omnisphere::repositories
