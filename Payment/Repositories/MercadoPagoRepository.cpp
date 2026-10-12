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

    bool MercadoPagoRepository::SaveSettings(const omnisphere::models::MercadoPagoSettings& settings) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::MercadoPagoSettings>({});
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string selectSql = "SELECT " + qp.SelectClause + " FROM \"MercadoPagoSettings\" LIMIT 1";
            std::vector<omnisphere::types::SQLParam> selectParams;
            auto existingDt = conn->FetchPrepared(selectSql, selectParams);

            auto isPlain = [](const std::string& val) -> bool {
                if (val.empty()) return false;
                if (val.rfind("TEST-", 0) == 0 || val.rfind("APP_USR-", 0) == 0) return true;
                return false;
            };

            auto ensureEncrypted = [&](const std::string& val) -> std::string {
                if (val.empty()) return "";
                if (isPlain(val)) {
                    return omnisphere::utils::Base64::Encode(val);
                }
                return val;
            };

            std::string encPublicKey = ensureEncrypted(settings.publicKey);
            std::string encAccessToken = ensureEncrypted(settings.accessToken);
            std::string encWebhookSecretKey = settings.webhookSecretKey.empty() ? "" : omnisphere::utils::Base64::Encode(settings.webhookSecretKey);

            if (!existingDt.IsEmpty())
            {
                const auto& existingRow = existingDt[0];
                auto getExisting = [&](const std::string& colName) -> std::string {
                    try {
                        return existingRow[colName].GetOptional<std::string>().value_or("");
                    } catch (...) {}
                    return "";
                };

                std::string finalName = settings.name.empty() ? getExisting("Name") : settings.name;
                std::string finalPublicKey = settings.publicKey.empty() ? getExisting("PublicKey") : encPublicKey;
                std::string finalAccessToken = settings.accessToken.empty() ? getExisting("AccessToken") : encAccessToken;
                std::string finalWebhookSecret = settings.webhookSecretKey.empty() ? getExisting("WebhookSecretKey") : encWebhookSecretKey;
                std::string finalApiBaseUrl = settings.apiBaseUrl.empty() ? getExisting("ApiBaseUrl") : settings.apiBaseUrl;
                std::string finalWebhookPath = settings.webhookPath.empty() ? getExisting("WebhookPath") : settings.webhookPath;
                if (finalWebhookPath.empty()) finalWebhookPath = "/api/v1/mercadopago/webhook";
                std::string finalCurrency = settings.currency.empty() ? getExisting("Currency") : settings.currency;
                if (finalCurrency.empty()) finalCurrency = "mxn";

                bool finalIsActive = settings.isActive;
                if (finalPublicKey.empty() || finalAccessToken.empty())
                {
                    finalIsActive = false;
                }

                std::vector<omnisphere::types::ColumnValue> updateCols = {
                    {"\"Name\"", omnisphere::types::MakeSQLParam(finalName)},
                    {"\"PublicKey\"", omnisphere::types::MakeSQLParam(finalPublicKey)},
                    {"\"AccessToken\"", omnisphere::types::MakeSQLParam(finalAccessToken)},
                    {"\"WebhookSecretKey\"", omnisphere::types::MakeSQLParam(finalWebhookSecret)},
                    {"\"ApiBaseUrl\"", omnisphere::types::MakeSQLParam(finalApiBaseUrl)},
                    {"\"WebhookPath\"", omnisphere::types::MakeSQLParam(finalWebhookPath)},
                    {"\"Currency\"", omnisphere::types::MakeSQLParam(finalCurrency)},
                    {"\"IsTestMode\"", omnisphere::types::MakeSQLParam(settings.isTestMode)},
                    {"\"IsActive\"", omnisphere::types::MakeSQLParam(finalIsActive)},
                    {"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(settings.createdBy)}
                };

                auto updateQuery = omnisphere::types::BuildUpdateQuery("\"MercadoPagoSettings\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(std::string("DEFAULT")));
                return conn->RunPrepared(updateQuery.Query, updateQuery.Parameters);
            }
            else
            {
                std::string finalApiBaseUrl = settings.apiBaseUrl.empty() ? "https://api.mercadopago.com" : settings.apiBaseUrl;
                std::string finalWebhookPath = settings.webhookPath.empty() ? "/api/v1/mercadopago/webhook" : settings.webhookPath;
                std::string finalCurrency = settings.currency.empty() ? "mxn" : settings.currency;

                bool finalIsActive = settings.isActive;
                if (encPublicKey.empty() || encAccessToken.empty())
                {
                    finalIsActive = false;
                }

                std::vector<std::string> cols = {
                    "\"Code\"", "\"Name\"", "\"PublicKey\"", "\"AccessToken\"",
                    "\"WebhookSecretKey\"", "\"ApiBaseUrl\"", "\"WebhookPath\"", "\"Currency\"",
                    "\"IsTestMode\"", "\"IsActive\"", "\"CreatedBy\""
                };
                std::string sql = omnisphere::types::BuildInsertQuery("\"MercadoPagoSettings\"", cols);
                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(std::string("DEFAULT")),
                    omnisphere::types::MakeSQLParam(settings.name.empty() ? std::string("Mercado Pago Settings") : settings.name),
                    omnisphere::types::MakeSQLParam(encPublicKey),
                    omnisphere::types::MakeSQLParam(encAccessToken),
                    omnisphere::types::MakeSQLParam(encWebhookSecretKey),
                    omnisphere::types::MakeSQLParam(finalApiBaseUrl),
                    omnisphere::types::MakeSQLParam(finalWebhookPath),
                    omnisphere::types::MakeSQLParam(finalCurrency),
                    omnisphere::types::MakeSQLParam(settings.isTestMode),
                    omnisphere::types::MakeSQLParam(finalIsActive),
                    omnisphere::types::MakeSQLParam(settings.createdBy)
                };
                return conn->RunPrepared(sql, params);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[MercadoPagoRepository::SaveSettings Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool MercadoPagoRepository::SaveTransaction(const omnisphere::models::MercadoPagoTransaction& tx) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            omnisphere::repositories::IdentityRepository identityRepo(m_dbPool);
            std::string codeVal = tx.code.empty() ? identityRepo.GetNextCode(conn, "MercadoPagoTransaction", "MPTX") : tx.code;
            if (codeVal.empty()) codeVal = "MPTX1";

            std::vector<std::string> cols = {
                "\"Code\"", "\"ExternalReference\"", "\"PaymentId\"", "\"PreferenceId\"",
                "\"Amount\"", "\"Currency\"", "\"Status\"", "\"StatusDetail\"",
                "\"PaymentMethodId\"", "\"PaymentTypeId\"", "\"Installments\"",
                "\"TicketUrl\"", "\"Clabe\"", "\"CardBrand\"", "\"CardLast4\"",
                "\"ClientIp\"", "\"IsActive\"", "\"CreatedBy\""
            };

            std::string sql = omnisphere::types::BuildInsertQuery("\"MercadoPagoTransactions\"", cols);
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(codeVal),
                omnisphere::types::MakeSQLParam(tx.externalReference),
                omnisphere::types::MakeSQLParam(tx.paymentId),
                omnisphere::types::MakeSQLParam(tx.preferenceId.value_or("")),
                omnisphere::types::MakeSQLParam(tx.amount),
                omnisphere::types::MakeSQLParam(tx.currency.empty() ? std::string("mxn") : tx.currency),
                omnisphere::types::MakeSQLParam(tx.status.empty() ? std::string("pending") : tx.status),
                omnisphere::types::MakeSQLParam(tx.statusDetail.value_or("")),
                omnisphere::types::MakeSQLParam(tx.paymentMethodId.value_or("")),
                omnisphere::types::MakeSQLParam(tx.paymentTypeId.value_or("")),
                omnisphere::types::MakeSQLParam(tx.installments),
                omnisphere::types::MakeSQLParam(tx.ticketUrl.value_or("")),
                omnisphere::types::MakeSQLParam(tx.clabe.value_or("")),
                omnisphere::types::MakeSQLParam(tx.cardBrand.value_or("")),
                omnisphere::types::MakeSQLParam(tx.cardLast4.value_or("")),
                omnisphere::types::MakeSQLParam(tx.clientIp.value_or("")),
                omnisphere::types::MakeSQLParam(tx.isActive),
                omnisphere::types::MakeSQLParam(tx.createdBy.empty() ? std::string("system") : tx.createdBy)
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
                return omnisphere::types::FromDataRow<omnisphere::models::MercadoPagoTransaction>(dt[0]);
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
                return omnisphere::types::FromDataRow<omnisphere::models::MercadoPagoTransaction>(dt[0]);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[MercadoPagoRepository::GetTransactionByReference Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }
} // namespace omnisphere::repositories
