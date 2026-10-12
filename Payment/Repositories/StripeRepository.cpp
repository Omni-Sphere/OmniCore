#include "Payment/Repositories/StripeRepository.hpp"
#include "Identity/Repositories/IdentityRepository.hpp"
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <OmniUtils/Base64.hpp>
#include <iostream>
#include <cctype>

namespace omnisphere::repositories
{
    StripeRepository::StripeRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    omnisphere::types::DataTable StripeRepository::GetSettings(const std::vector<std::string>& requestedFields) const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::StripeSettings>(requestedFields);
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"StripeSettings\" LIMIT 1";
            std::vector<omnisphere::types::SQLParam> params;
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::GetSettings Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    bool StripeRepository::Create(const omnisphere::models::StripeSettings& settings, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        std::string queryStr;
        try
        {
            auto conn = m_dbPool->Acquire();
            auto insertResult = omnisphere::types::BuildInsertQuery("\"StripeSettings\"", 0, settings, mutationFields);
            queryStr = insertResult.Query;
            return conn->RunPrepared(insertResult.Query, insertResult.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::Create Exception] " << ex.what()
                      << "\n[SQL Query]: " << queryStr << std::endl;
            return false;
        }
    }

    bool StripeRepository::Update(const omnisphere::models::StripeSettings& settings, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        std::string queryStr;
        try
        {
            auto conn = m_dbPool->Acquire();
            auto cols = omnisphere::types::ExtractUpdateColumns(settings, mutationFields);
            if (cols.empty())
            {
                return true;
            }
            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"StripeSettings\"",
                cols,
                "\"Code\"",
                omnisphere::types::MakeSQLParam(settings.code.empty() ? "DEFAULT" : settings.code)
            );
            queryStr = updateResult.Query;
            return conn->RunPrepared(updateResult.Query, updateResult.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::Update Exception] " << ex.what()
                      << "\n[SQL Query]: " << queryStr << std::endl;
            return false;
        }
    }

    bool StripeRepository::SaveSession(omnisphere::models::StripeSession& session, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            omnisphere::repositories::IdentityRepository identityRepo(m_dbPool);
            if (session.code.empty())
            {
                session.code = identityRepo.GetNextCode(conn, "StripeSession", "STS");
                if (session.code.empty()) session.code = "STS1";
            }

            std::vector<std::string> fieldsToInsert = mutationFields;
            if (!fieldsToInsert.empty())
            {
                fieldsToInsert.push_back("Code");
            }

            auto insertData = omnisphere::types::BuildInsertQuery("\"StripeSessions\"", 0, session, fieldsToInsert);
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
            std::cerr << "[StripeRepository::SaveSession Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool StripeRepository::UpdateSessionStatus(const std::string& stripeSessionId, const std::string& newStatus, const std::string& paymentIntentId) const
    {
        if (!m_dbPool || stripeSessionId.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<omnisphere::types::ColumnValue> updateCols = {
                {"\"Status\"", omnisphere::types::MakeSQLParam(newStatus)}
            };
            if (!paymentIntentId.empty())
            {
                updateCols.push_back({"\"PaymentIntentId\"", omnisphere::types::MakeSQLParam(paymentIntentId)});
            }

            auto updateQuery = omnisphere::types::BuildUpdateQuery("\"StripeSessions\"", updateCols, "\"StripeSessionId\"", omnisphere::types::MakeSQLParam(stripeSessionId));
            return conn->RunPrepared(updateQuery.Query, updateQuery.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::UpdateSessionStatus Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    omnisphere::types::DataTable StripeRepository::GetSessionsByReservation(const std::string& reservationCode) const
    {
        if (!m_dbPool || reservationCode.empty()) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::StripeSession>({});
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"StripeSessions\" WHERE \"ReservationCode\" = $1 ORDER BY \"Entry\" DESC";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(reservationCode) };
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::GetSessionsByReservation Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    std::optional<omnisphere::models::StripeSession> StripeRepository::GetSessionByStripeId(const std::string& stripeSessionId) const
    {
        if (!m_dbPool || stripeSessionId.empty()) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::StripeSession>({});
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"StripeSessions\" WHERE \"StripeSessionId\" = $1 LIMIT 1";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(stripeSessionId) };
            auto dt = conn->FetchPrepared(sql, params);
            if (!dt.IsEmpty())
            {
                return omnisphere::types::FromDataRow<omnisphere::models::StripeSession>(dt[0]);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::GetSessionByStripeId Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }

    bool StripeRepository::SaveTransaction(omnisphere::models::StripeTransaction& tx, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            omnisphere::repositories::IdentityRepository identityRepo(m_dbPool);
            if (tx.code.empty())
            {
                tx.code = identityRepo.GetNextCode(conn, "StripeTransaction", "STX");
                if (tx.code.empty()) tx.code = "STX1";
            }

            std::vector<std::string> fieldsToInsert = mutationFields;
            if (!fieldsToInsert.empty())
            {
                fieldsToInsert.push_back("Code");
            }

            omnisphere::models::StripeTransaction toSave = tx;
            if (toSave.clabe)
            {
                toSave.clabe = omnisphere::utils::Base64::Encode(*toSave.clabe);
            }
            if (toSave.authorizationCode)
            {
                toSave.authorizationCode = omnisphere::utils::Base64::Encode(*toSave.authorizationCode);
            }
            if (toSave.cardFingerprint)
            {
                toSave.cardFingerprint = omnisphere::utils::Base64::Encode(*toSave.cardFingerprint);
            }
            if (toSave.clientIp)
            {
                toSave.clientIp = omnisphere::utils::Base64::Encode(*toSave.clientIp);
            }

            auto insertData = omnisphere::types::BuildInsertQuery("\"StripeTransactions\"", 0, toSave, fieldsToInsert);
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
            std::cerr << "[StripeRepository::SaveTransaction Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool StripeRepository::UpdateTransactionStatus(const std::string& paymentIntentId, const std::string& newStatus, const std::string& receiptUrl) const
    {
        if (!m_dbPool || paymentIntentId.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<omnisphere::types::ColumnValue> updateCols = {
                {"\"Status\"", omnisphere::types::MakeSQLParam(newStatus)}
            };
            if (!receiptUrl.empty())
            {
                updateCols.push_back({"\"ReceiptUrl\"", omnisphere::types::MakeSQLParam(receiptUrl)});
            }

            auto updateQuery = omnisphere::types::BuildUpdateQuery("\"StripeTransactions\"", updateCols, "\"PaymentIntentId\"", omnisphere::types::MakeSQLParam(paymentIntentId));
            return conn->RunPrepared(updateQuery.Query, updateQuery.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::UpdateTransactionStatus Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    omnisphere::types::DataTable StripeRepository::GetTransactionsByReservation(const std::string& reservationCode) const
    {
        if (!m_dbPool || reservationCode.empty()) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::StripeTransaction>({});
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"StripeTransactions\" WHERE \"ReservationCode\" = $1 ORDER BY \"Entry\" DESC";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(reservationCode) };
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::GetTransactionsByReservation Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    std::optional<omnisphere::models::StripeTransaction> StripeRepository::GetTransactionByPaymentIntent(const std::string& paymentIntentId) const
    {
        if (!m_dbPool || paymentIntentId.empty()) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::StripeTransaction>({});
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"StripeTransactions\" WHERE \"PaymentIntentId\" = $1 LIMIT 1";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(paymentIntentId) };
            auto dt = conn->FetchPrepared(sql, params);
            if (!dt.IsEmpty())
            {
                auto model = omnisphere::types::FromDataRow<omnisphere::models::StripeTransaction>(dt[0]);
                auto safeDecode = [](std::optional<std::string>& opt) {
                    if (opt.has_value() && !opt->empty()) {
                        try { *opt = omnisphere::utils::Base64::Decode(*opt); } catch (...) {}
                    }
                };
                safeDecode(model.clabe);
                safeDecode(model.authorizationCode);
                safeDecode(model.cardFingerprint);
                safeDecode(model.clientIp);
                return model;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::GetTransactionByPaymentIntent Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }
}
