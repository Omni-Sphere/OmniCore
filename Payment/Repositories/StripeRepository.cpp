#include "Payment/Repositories/StripeRepository.hpp"
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

    bool StripeRepository::SaveSettings(const omnisphere::models::StripeSettings& settings) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::StripeSettings>({});
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string selectSql = "SELECT " + qp.SelectClause + " FROM \"StripeSettings\" LIMIT 1";
            std::vector<omnisphere::types::SQLParam> selectParams;
            auto existingDt = conn->FetchPrepared(selectSql, selectParams);

            auto isPlain = [](const std::string& val) -> bool {
                if (val.empty()) return false;
                if (val.rfind("pk_", 0) == 0 || val.rfind("sk_", 0) == 0 || val.rfind("whsec_", 0) == 0) return true;
                return false;
            };

            auto ensureEncrypted = [&](const std::string& val) -> std::string {
                if (val.empty()) return "";
                if (isPlain(val)) {
                    return omnisphere::utils::Base64::Encode(val);
                }
                return val;
            };

            std::string encPublishableKey = ensureEncrypted(settings.publishableKey);
            std::string encSecretKey = ensureEncrypted(settings.secretKey);
            std::string encWebhookSecretKey = ensureEncrypted(settings.webhookSecretKey);

            if (existingDt.RowsCount() > 0)
            {
                auto getExisting = [&](const std::string& colName) -> std::string {
                    try {
                        if (existingDt.RowsCount() > 0 && existingDt[0].HasColumn(colName) && !existingDt[0][colName].IsNull())
                        {
                            return (std::string)existingDt[0][colName];
                        }
                    } catch (...) {}
                    return "";
                };

                std::string finalName = settings.name.empty() ? getExisting("Name") : settings.name;
                std::string finalPublishableKey = settings.publishableKey.empty() ? getExisting("PublishableKey") : encPublishableKey;
                std::string finalSecretKey = settings.secretKey.empty() ? getExisting("SecretKey") : encSecretKey;
                std::string finalWebhookSecretKey = settings.webhookSecretKey.empty() ? getExisting("WebhookSecretKey") : encWebhookSecretKey;
                std::string finalApiBaseUrl = settings.apiBaseUrl.empty() ? getExisting("ApiBaseUrl") : settings.apiBaseUrl;
                if (finalApiBaseUrl.empty()) finalApiBaseUrl = "https://api.stripe.com/v1";
                std::string finalCheckoutEndpoint = settings.checkoutEndpoint.empty() ? getExisting("CheckoutEndpoint") : settings.checkoutEndpoint;
                if (finalCheckoutEndpoint.empty()) finalCheckoutEndpoint = "/checkout/sessions";
                std::string finalWebhookPath = settings.webhookPath.empty() ? getExisting("WebhookPath") : settings.webhookPath;
                if (finalWebhookPath.empty()) finalWebhookPath = "/api/v1/stripe/webhook";
                std::string finalCurrency = settings.currency.empty() ? getExisting("Currency") : settings.currency;
                if (finalCurrency.empty()) finalCurrency = "mxn";

                std::vector<omnisphere::types::ColumnValue> updateCols = {
                    {"\"Name\"", omnisphere::types::MakeSQLParam(finalName)},
                    {"\"PublishableKey\"", omnisphere::types::MakeSQLParam(finalPublishableKey)},
                    {"\"SecretKey\"", omnisphere::types::MakeSQLParam(finalSecretKey)},
                    {"\"WebhookSecretKey\"", omnisphere::types::MakeSQLParam(finalWebhookSecretKey)},
                    {"\"ApiBaseUrl\"", omnisphere::types::MakeSQLParam(finalApiBaseUrl)},
                    {"\"CheckoutEndpoint\"", omnisphere::types::MakeSQLParam(finalCheckoutEndpoint)},
                    {"\"WebhookPath\"", omnisphere::types::MakeSQLParam(finalWebhookPath)},
                    {"\"Currency\"", omnisphere::types::MakeSQLParam(finalCurrency)},
                    {"\"IsTestMode\"", omnisphere::types::MakeSQLParam(settings.isTestMode)},
                    {"\"IsActive\"", omnisphere::types::MakeSQLParam(settings.isActive)},
                    {"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(settings.createdBy)}
                };

                auto updateQuery = omnisphere::types::BuildUpdateQuery("\"StripeSettings\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(std::string("DEFAULT")));
                return conn->RunPrepared(updateQuery.Query, updateQuery.Parameters);
            }
            else
            {
                // Inserción bajo demanda desde la mutación (cuando no existe el registro aún)
                std::string finalApiBaseUrl = settings.apiBaseUrl.empty() ? "https://api.stripe.com/v1" : settings.apiBaseUrl;
                std::string finalCheckoutEndpoint = settings.checkoutEndpoint.empty() ? "/checkout/sessions" : settings.checkoutEndpoint;
                std::string finalWebhookPath = settings.webhookPath.empty() ? "/api/v1/stripe/webhook" : settings.webhookPath;
                std::string finalCurrency = settings.currency.empty() ? "mxn" : settings.currency;

                std::vector<std::string> cols = {
                    "\"Code\"", "\"Name\"", "\"PublishableKey\"", "\"SecretKey\"",
                    "\"WebhookSecretKey\"", "\"ApiBaseUrl\"", "\"CheckoutEndpoint\"", "\"WebhookPath\"",
                    "\"Currency\"", "\"IsTestMode\"", "\"IsActive\"", "\"CreatedBy\""
                };
                std::string sql = omnisphere::types::BuildInsertQuery("\"StripeSettings\"", cols);
                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(std::string("DEFAULT")),
                    omnisphere::types::MakeSQLParam(settings.name.empty() ? std::string("Stripe Payment Settings") : settings.name),
                    omnisphere::types::MakeSQLParam(encPublishableKey),
                    omnisphere::types::MakeSQLParam(encSecretKey),
                    omnisphere::types::MakeSQLParam(encWebhookSecretKey),
                    omnisphere::types::MakeSQLParam(finalApiBaseUrl),
                    omnisphere::types::MakeSQLParam(finalCheckoutEndpoint),
                    omnisphere::types::MakeSQLParam(finalWebhookPath),
                    omnisphere::types::MakeSQLParam(finalCurrency),
                    omnisphere::types::MakeSQLParam(settings.isTestMode),
                    omnisphere::types::MakeSQLParam(settings.isActive),
                    omnisphere::types::MakeSQLParam(settings.createdBy)
                };

                return conn->RunPrepared(sql, params);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::SaveSettings Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool StripeRepository::SaveSession(const omnisphere::models::StripeSession& session) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string codeVal = session.code.empty() ? "STR-SESS-" + session.stripeSessionId.substr(0, 12) : session.code;
            std::vector<std::string> cols = {
                "\"Code\"", "\"ReservationCode\"", "\"StripeSessionId\"", "\"PaymentIntentId\"",
                "\"CheckoutUrl\"", "\"Amount\"", "\"Currency\"", "\"Status\"", "\"IsActive\"", "\"CreatedBy\""
            };
            std::string sql = omnisphere::types::BuildInsertQuery("\"StripeSessions\"", cols);
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(codeVal),
                omnisphere::types::MakeSQLParam(session.reservationCode),
                omnisphere::types::MakeSQLParam(session.stripeSessionId),
                omnisphere::types::MakeSQLParam(session.paymentIntentId.value_or("")),
                omnisphere::types::MakeSQLParam(session.checkoutUrl),
                omnisphere::types::MakeSQLParam(session.amount),
                omnisphere::types::MakeSQLParam(session.currency.empty() ? std::string("mxn") : session.currency),
                omnisphere::types::MakeSQLParam(session.status.empty() ? std::string("open") : session.status),
                omnisphere::types::MakeSQLParam(session.isActive),
                omnisphere::types::MakeSQLParam(session.createdBy)
            };

            return conn->RunPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
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
            if (dt.RowsCount() > 0)
            {
                omnisphere::models::StripeSession sess;
                sess.entry = (int)dt[0]["Entry"];
                sess.code = (std::string)dt[0]["Code"];
                sess.reservationCode = (std::string)dt[0]["ReservationCode"];
                sess.stripeSessionId = (std::string)dt[0]["StripeSessionId"];
                if (dt[0].HasColumn("PaymentIntentId") && !dt[0]["PaymentIntentId"].IsNull())
                    sess.paymentIntentId = (std::string)dt[0]["PaymentIntentId"];
                sess.checkoutUrl = (std::string)dt[0]["CheckoutUrl"];
                sess.amount = (double)dt[0]["Amount"];
                sess.currency = (std::string)dt[0]["Currency"];
                sess.status = (std::string)dt[0]["Status"];
                sess.isActive = (bool)dt[0]["IsActive"];
                return sess;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::GetSessionByStripeId Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }

    bool StripeRepository::SaveTransaction(const omnisphere::models::StripeTransaction& tx) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string codeVal = tx.code.empty() ? ("TX-" + tx.stripePaymentIntentId) : tx.code;

            std::vector<std::string> cols = {
                "\"Code\"", "\"ReservationCode\"", "\"PaymentIntentId\"", "\"ChargeId\"",
                "\"Amount\"", "\"Currency\"", "\"Status\"", "\"PaymentMethodType\"",
                "\"Clabe\"", "\"BankName\"", "\"CardBrand\"", "\"CardLast4\"", "\"CardType\"",
                "\"AuthorizationCode\"", "\"CardFingerprint\"", "\"ReceiptUrl\"", "\"HostedInstructionsUrl\"",
                "\"ClientIp\"", "\"IsActive\"", "\"CreatedBy\""
            };

            std::string sql = omnisphere::types::BuildInsertQuery("\"StripeTransactions\"", cols);
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(codeVal),
                omnisphere::types::MakeSQLParam(tx.reservationCode),
                omnisphere::types::MakeSQLParam(tx.stripePaymentIntentId),
                omnisphere::types::MakeSQLParam(tx.stripeChargeId.value_or("")),
                omnisphere::types::MakeSQLParam(tx.amount),
                omnisphere::types::MakeSQLParam(tx.currency.empty() ? std::string("mxn") : tx.currency),
                omnisphere::types::MakeSQLParam(tx.status.empty() ? std::string("succeeded") : tx.status),
                omnisphere::types::MakeSQLParam(tx.paymentMethodType.value_or("card")),
                omnisphere::types::MakeSQLParam(tx.clabe.value_or("")),
                omnisphere::types::MakeSQLParam(tx.bankName.value_or("")),
                omnisphere::types::MakeSQLParam(tx.cardBrand.value_or("")),
                omnisphere::types::MakeSQLParam(tx.cardLast4.value_or("")),
                omnisphere::types::MakeSQLParam(tx.cardFunding.value_or("")),
                omnisphere::types::MakeSQLParam(tx.authorizationCode.value_or("")),
                omnisphere::types::MakeSQLParam(tx.cardFingerprint.value_or("")),
                omnisphere::types::MakeSQLParam(tx.receiptUrl.value_or("")),
                omnisphere::types::MakeSQLParam(tx.hostedInstructionsUrl.value_or("")),
                omnisphere::types::MakeSQLParam(tx.clientIp.value_or("")),
                omnisphere::types::MakeSQLParam(tx.isActive),
                omnisphere::types::MakeSQLParam(tx.createdBy)
            };

            return conn->RunPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
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
            if (dt.RowsCount() > 0)
            {
                omnisphere::models::StripeTransaction tx;
                tx.entry = (int)dt[0]["Entry"];
                tx.code = (std::string)dt[0]["Code"];
                tx.reservationCode = (std::string)dt[0]["ReservationCode"];
                tx.stripePaymentIntentId = (std::string)dt[0]["PaymentIntentId"];
                if (dt[0].HasColumn("ChargeId") && !dt[0]["ChargeId"].IsNull())
                    tx.stripeChargeId = (std::string)dt[0]["ChargeId"];
                tx.amount = (double)dt[0]["Amount"];
                tx.currency = (std::string)dt[0]["Currency"];
                tx.status = (std::string)dt[0]["Status"];
                if (dt[0].HasColumn("PaymentMethodType") && !dt[0]["PaymentMethodType"].IsNull())
                    tx.paymentMethodType = (std::string)dt[0]["PaymentMethodType"];
                if (dt[0].HasColumn("Clabe") && !dt[0]["Clabe"].IsNull())
                    tx.clabe = (std::string)dt[0]["Clabe"];
                if (dt[0].HasColumn("BankName") && !dt[0]["BankName"].IsNull())
                    tx.bankName = (std::string)dt[0]["BankName"];
                if (dt[0].HasColumn("HostedInstructionsUrl") && !dt[0]["HostedInstructionsUrl"].IsNull())
                    tx.hostedInstructionsUrl = (std::string)dt[0]["HostedInstructionsUrl"];
                if (dt[0].HasColumn("CardBrand") && !dt[0]["CardBrand"].IsNull())
                    tx.cardBrand = (std::string)dt[0]["CardBrand"];
                if (dt[0].HasColumn("CardLast4") && !dt[0]["CardLast4"].IsNull())
                    tx.cardLast4 = (std::string)dt[0]["CardLast4"];
                if (dt[0].HasColumn("ReceiptUrl") && !dt[0]["ReceiptUrl"].IsNull())
                    tx.receiptUrl = (std::string)dt[0]["ReceiptUrl"];
                tx.isActive = (bool)dt[0]["IsActive"];
                return tx;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[StripeRepository::GetTransactionByPaymentIntent Exception] " << ex.what() << std::endl;
        }
        return std::nullopt;
    }
}
