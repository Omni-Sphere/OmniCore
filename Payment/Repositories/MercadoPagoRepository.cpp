#include "Payment/Repositories/MercadoPagoRepository.hpp"
#include "Identity/Repositories/IdentityRepository.hpp"
#include <OmniData/DataMapper.hpp>
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <OmniUtils/Base64.hpp>
#include <iostream>

namespace omnisphere::repositories
{
    MercadoPagoRepository::MercadoPagoRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    omnisphere::types::DataTable MercadoPagoRepository::GetSettings(const std::vector<std::string>& requestedFields) const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::MercadoPagoSettings>(requestedFields);
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"MercadoPagoSettings\" LIMIT 1";
            std::vector<omnisphere::types::SQLParam> params;
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[MercadoPagoRepository::GetSettings Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    bool MercadoPagoRepository::Create(const omnisphere::models::MercadoPagoSettings& settings, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            auto insertResult = omnisphere::types::BuildInsertQuery("\"MercadoPagoSettings\"", 0, settings, mutationFields);
            return conn->RunPrepared(insertResult.Query, insertResult.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[MercadoPagoRepository::Create Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool MercadoPagoRepository::Update(const omnisphere::models::MercadoPagoSettings& settings, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            auto cols = omnisphere::types::ExtractUpdateColumns(settings, mutationFields);
            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"MercadoPagoSettings\"",
                cols,
                "\"Code\"",
                omnisphere::types::MakeSQLParam(settings.code.empty() ? "DEFAULT" : settings.code)
            );
            return conn->RunPrepared(updateResult.Query, updateResult.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[MercadoPagoRepository::Update Exception] " << ex.what() << std::endl;
            return false;
        }
    }


    bool MercadoPagoRepository::SaveTransaction(omnisphere::models::MercadoPagoTransaction& tx, const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            omnisphere::repositories::IdentityRepository identityRepo(m_dbPool);
            if (tx.code.empty())
            {
                tx.code = identityRepo.GetNextCode(conn, "MercadoPagoTransaction", "MPTX");
                if (tx.code.empty()) tx.code = "MPTX1";
            }

            std::vector<std::string> fieldsToInsert = mutationFields;
            if (!fieldsToInsert.empty())
            {
                fieldsToInsert.push_back("Code");
            }

            omnisphere::models::MercadoPagoTransaction toSave = tx;
            if (toSave.clabe.has_value() && !toSave.clabe->empty())
            {
                toSave.clabe = omnisphere::utils::Base64::Encode(*toSave.clabe);
            }
            if (toSave.preferenceId.has_value() && !toSave.preferenceId->empty())
            {
                toSave.preferenceId = omnisphere::utils::Base64::Encode(*toSave.preferenceId);
            }
            if (toSave.clientIp.has_value() && !toSave.clientIp->empty())
            {
                toSave.clientIp = omnisphere::utils::Base64::Encode(*toSave.clientIp);
            }

            auto insertData = omnisphere::types::BuildInsertQuery("\"MercadoPagoTransactions\"", 0, toSave, fieldsToInsert);
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
            std::cerr << "[MercadoPagoRepository::SaveTransaction Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool MercadoPagoRepository::UpdateTransactionStatus(
        const std::string& paymentId,
        const std::string& newStatus,
        const std::string& statusDetail,
        const std::string& ticketUrl
    ) const
    {
        if (!m_dbPool || paymentId.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<omnisphere::types::ColumnValue> updateCols = {
                {"\"Status\"", omnisphere::types::MakeSQLParam(newStatus)}
            };
            if (!statusDetail.empty())
            {
                updateCols.push_back({"\"StatusDetail\"", omnisphere::types::MakeSQLParam(statusDetail)});
            }
            if (!ticketUrl.empty())
            {
                updateCols.push_back({"\"TicketUrl\"", omnisphere::types::MakeSQLParam(ticketUrl)});
            }

            auto updateQuery = omnisphere::types::BuildUpdateQuery("\"MercadoPagoTransactions\"", updateCols, "\"PaymentId\"", omnisphere::types::MakeSQLParam(paymentId));
            return conn->RunPrepared(updateQuery.Query, updateQuery.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[MercadoPagoRepository::UpdateTransactionStatus Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    std::optional<omnisphere::models::MercadoPagoTransaction> MercadoPagoRepository::GetTransactionByPaymentId(const std::string& paymentId) const
    {
        if (!m_dbPool || paymentId.empty()) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"PaymentId\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts({}, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"MercadoPagoTransactions\" WHERE " + qp.WhereClause + " LIMIT 1";
            auto dt = conn->FetchPrepared(sql, { omnisphere::types::MakeSQLParam(paymentId) });
            if (!dt.IsEmpty())
            {
                auto model = omnisphere::types::FromDataRow<omnisphere::models::MercadoPagoTransaction>(dt[0]);
                auto safeDecode = [](std::optional<std::string>& opt) {
                    if (opt.has_value() && !opt->empty()) {
                        try { *opt = omnisphere::utils::Base64::Decode(*opt); } catch (...) {}
                    }
                };
                safeDecode(model.clabe);
                safeDecode(model.preferenceId);
                safeDecode(model.clientIp);
                return model;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[MercadoPagoRepository::GetTransactionByPaymentId Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }

    std::optional<omnisphere::models::MercadoPagoTransaction> MercadoPagoRepository::GetTransactionByReference(const std::string& referenceCode) const
    {
        if (!m_dbPool || referenceCode.empty()) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"ExternalReference\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts({}, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"MercadoPagoTransactions\" WHERE " + qp.WhereClause + " ORDER BY \"Entry\" DESC LIMIT 1";
            auto dt = conn->FetchPrepared(sql, { omnisphere::types::MakeSQLParam(referenceCode) });
            if (!dt.IsEmpty())
            {
                auto model = omnisphere::types::FromDataRow<omnisphere::models::MercadoPagoTransaction>(dt[0]);
                auto safeDecode = [](std::optional<std::string>& opt) {
                    if (opt.has_value() && !opt->empty()) {
                        try { *opt = omnisphere::utils::Base64::Decode(*opt); } catch (...) {}
                    }
                };
                safeDecode(model.clabe);
                safeDecode(model.preferenceId);
                safeDecode(model.clientIp);
                return model;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[MercadoPagoRepository::GetTransactionByReference Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }
} // namespace omnisphere::repositories
