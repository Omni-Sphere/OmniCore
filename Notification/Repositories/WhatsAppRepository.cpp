#include "Notification/Repositories/WhatsAppRepository.hpp"
#include <OmniData/Database.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <OmniUtils/Base64.hpp>
#include <iostream>

namespace omnisphere::repositories
{
    WhatsAppRepository::WhatsAppRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool)) {}

    omnisphere::types::DataTable WhatsAppRepository::GetSettings(const std::vector<std::string>& requestedFields) const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppSettings>(requestedFields);
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"WhatsAppSettings\" LIMIT 1";
            std::vector<omnisphere::types::SQLParam> params;
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::GetSettings Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    bool WhatsAppRepository::SaveSettings(const omnisphere::models::WhatsAppSettings& settings) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            auto existingDt = GetSettings();

            auto ensureEncrypted = [](const std::string& val) -> std::string {
                if (val.empty()) return "";
                try {
                    std::string decoded = omnisphere::utils::Base64::Decode(val);
                    if (omnisphere::utils::Base64::Encode(decoded) == val) {
                        return val;
                    }
                } catch (...) {}
                return omnisphere::utils::Base64::Encode(val);
            };

            std::string encPhoneId = ensureEncrypted(settings.phoneId);
            std::string encApiToken = ensureEncrypted(settings.apiToken);
            std::string encBusinessAccountId = ensureEncrypted(settings.businessAccountId);
            std::string encWebhookVerifyToken = ensureEncrypted(settings.webhookVerifyToken);

            if (existingDt.RowsCount() > 0)
            {
                auto getExisting = [&](const std::string& colName) -> std::string {
                    try {
                        if (existingDt.RowsCount() > 0)
                        {
                            std::string val = (std::string)existingDt[0][colName];
                            return val;
                        }
                    } catch (...) {}
                    return "";
                };

                std::string finalName = settings.name.empty() ? getExisting("Name") : settings.name;
                std::string finalPhoneId = settings.phoneId.empty() ? getExisting("PhoneId") : encPhoneId;
                std::string finalApiToken = settings.apiToken.empty() ? getExisting("ApiToken") : encApiToken;
                std::string finalBusinessAccountId = settings.businessAccountId.empty() ? getExisting("BusinessAccountId") : encBusinessAccountId;
                std::string finalWebhookVerifyToken = settings.webhookVerifyToken.empty() ? getExisting("WebhookVerifyToken") : encWebhookVerifyToken;
                std::string finalApiVersion = settings.apiVersion.empty() ? getExisting("ApiVersion") : settings.apiVersion;
                if (finalApiVersion.empty()) finalApiVersion = "v24.0";

                std::vector<omnisphere::types::ColumnValue> updateCols = {
                    {"\"Name\"", omnisphere::types::MakeSQLParam(finalName)},
                    {"\"PhoneId\"", omnisphere::types::MakeSQLParam(finalPhoneId)},
                    {"\"ApiToken\"", omnisphere::types::MakeSQLParam(finalApiToken)},
                    {"\"BusinessAccountId\"", omnisphere::types::MakeSQLParam(finalBusinessAccountId)},
                    {"\"WebhookVerifyToken\"", omnisphere::types::MakeSQLParam(finalWebhookVerifyToken)},
                    {"\"ApiVersion\"", omnisphere::types::MakeSQLParam(finalApiVersion)},
                    {"\"IsActive\"", omnisphere::types::MakeSQLParam(settings.isActive)},
                    {"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(settings.createdBy)}
                };

                auto updateQuery = omnisphere::types::BuildUpdateQuery("\"WhatsAppSettings\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam("DEFAULT"));
                return conn->RunPrepared(updateQuery.Query, updateQuery.Parameters);
            }
            else
            {
                std::vector<std::string> cols = {
                    "\"Code\"", "\"Name\"", "\"PhoneId\"", "\"ApiToken\"",
                    "\"BusinessAccountId\"", "\"WebhookVerifyToken\"", "\"ApiVersion\"",
                    "\"IsActive\"", "\"CreatedBy\""
                };
                std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppSettings\"", cols);
                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(settings.code.empty() ? "DEFAULT" : settings.code),
                    omnisphere::types::MakeSQLParam(settings.name),
                    omnisphere::types::MakeSQLParam(encPhoneId),
                    omnisphere::types::MakeSQLParam(encApiToken),
                    omnisphere::types::MakeSQLParam(encBusinessAccountId),
                    omnisphere::types::MakeSQLParam(encWebhookVerifyToken),
                    omnisphere::types::MakeSQLParam(settings.apiVersion.empty() ? "v24.0" : settings.apiVersion),
                    omnisphere::types::MakeSQLParam(settings.isActive),
                    omnisphere::types::MakeSQLParam(settings.createdBy)
                };
                return conn->RunPrepared(sql, params);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::SaveSettings Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    int WhatsAppRepository::GetOrCreateConversation(const std::string& customerPhone, const std::string& customerName) const
    {
        if (!m_dbPool || customerPhone.empty()) return 0;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<std::string> selectFields = {"\"Entry\""};
            std::vector<omnisphere::types::Condition> conditions = {{"", "\"CustomerPhone\"", "=", "?"}};
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string selectSql = "SELECT " + qp.SelectClause + " FROM \"WhatsAppConversations\" WHERE " + qp.WhereClause + " LIMIT 1";
            std::vector<omnisphere::types::SQLParam> selectParams = { omnisphere::types::MakeSQLParam(customerPhone) };
            auto dt = conn->FetchPrepared(selectSql, selectParams);
            if (dt.RowsCount() > 0)
            {
                return dt[0]["Entry"];
            }

            std::string code = "CONV-" + customerPhone;
            std::vector<std::string> insertCols = {"\"Code\"", "\"CustomerPhone\"", "\"CustomerName\"", "\"Status\"", "\"IsActive\"", "\"CreatedBy\""};
            std::string insertSql = omnisphere::types::BuildInsertQuery("\"WhatsAppConversations\"", insertCols) + " RETURNING \"Entry\"";
            std::vector<omnisphere::types::SQLParam> insertParams = {
                omnisphere::types::MakeSQLParam(code),
                omnisphere::types::MakeSQLParam(customerPhone),
                omnisphere::types::MakeSQLParam(customerName),
                omnisphere::types::MakeSQLParam("OPEN"),
                omnisphere::types::MakeSQLParam(true),
                omnisphere::types::MakeSQLParam(1)
            };
            auto insertDt = conn->FetchPrepared(insertSql, insertParams);
            if (insertDt.RowsCount() > 0)
            {
                return insertDt[0]["Entry"];
            }
            return 0;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::GetOrCreateConversation Exception] " << ex.what() << std::endl;
            return 0;
        }
    }

    omnisphere::types::DataTable WhatsAppRepository::ReadConversations(int limit) const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppConversation>({});
            std::vector<omnisphere::types::Condition> conditions = {{"", "\"IsActive\"", "=", "?"}};
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"WhatsAppConversations\" WHERE " + qp.WhereClause + " ORDER BY \"LastMessageDate\" DESC LIMIT ?";
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(true),
                omnisphere::types::MakeSQLParam(limit)
            };
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::ReadConversations Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    omnisphere::types::DataTable WhatsAppRepository::ReadMessages(int conversationEntry, int limit) const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppMessage>({});
            std::vector<omnisphere::types::Condition> conditions = {{"", "\"ConversationEntry\"", "=", "?"}};
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"WhatsAppMessages\" WHERE " + qp.WhereClause + " ORDER BY \"CreateDate\" ASC LIMIT ?";
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(conversationEntry),
                omnisphere::types::MakeSQLParam(limit)
            };
            return conn->FetchPrepared(sql, params);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::ReadMessages Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    bool WhatsAppRepository::LogMessage(const omnisphere::models::WhatsAppMessage& msg) const
    {
        if (!m_dbPool) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<std::string> cols = {
                "\"Code\"", "\"ConversationEntry\"", "\"SenderType\"", "\"MessageType\"",
                "\"TemplateName\"", "\"Content\"", "\"MediaUrl\"", "\"Status\"",
                "\"ResponsePayload\"", "\"ErrorMessage\"", "\"SentBy\""
            };
            std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppMessages\"", cols);

            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(msg.code),
                omnisphere::types::MakeSQLParam(msg.conversationEntry),
                omnisphere::types::MakeSQLParam(msg.senderType),
                omnisphere::types::MakeSQLParam(msg.messageType),
                omnisphere::types::MakeSQLParam(msg.templateName),
                omnisphere::types::MakeSQLParam(msg.content),
                omnisphere::types::MakeSQLParam(msg.mediaUrl),
                omnisphere::types::MakeSQLParam(msg.status),
                omnisphere::types::MakeSQLParam(msg.responsePayload),
                omnisphere::types::MakeSQLParam(msg.errorMessage),
                omnisphere::types::MakeSQLParam(msg.sentBy)
            };

            bool ok = conn->RunPrepared(sql, params);
            if (ok && msg.content.has_value() && !msg.content.value().empty())
            {
                std::vector<omnisphere::types::ColumnValue> updateCols = {
                    {"\"LastMessageText\"", omnisphere::types::MakeSQLParam(msg.content.value())}
                };
                auto updateQuery = omnisphere::types::BuildUpdateQuery("\"WhatsAppConversations\"", updateCols, "\"Entry\"", omnisphere::types::MakeSQLParam(msg.conversationEntry));
                conn->RunPrepared(updateQuery.Query, updateQuery.Parameters);
            }
            return ok;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::LogMessage Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool WhatsAppRepository::UpdateMessageStatus(const std::string& wamidCode, const std::string& newStatus, const std::string& responsePayload) const
    {
        if (!m_dbPool || wamidCode.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<omnisphere::types::ColumnValue> updateCols = {
                {"\"Status\"", omnisphere::types::MakeSQLParam(newStatus)},
                {"\"ResponsePayload\"", omnisphere::types::MakeSQLParam(responsePayload)}
            };
            auto updateQuery = omnisphere::types::BuildUpdateQuery("\"WhatsAppMessages\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(wamidCode));
            return conn->RunPrepared(updateQuery.Query, updateQuery.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::UpdateMessageStatus Exception] " << ex.what() << std::endl;
            return false;
        }
    }
} // namespace omnisphere::repositories
