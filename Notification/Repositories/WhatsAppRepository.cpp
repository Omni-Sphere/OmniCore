#include "Notification/Repositories/WhatsAppRepository.hpp"
#include "Identity/Repositories/IdentityRepository.hpp"
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
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppSettings>({});
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string selectSql = "SELECT " + qp.SelectClause + " FROM \"WhatsAppSettings\" LIMIT 1";
            std::vector<omnisphere::types::SQLParam> selectParams;
            auto existingDt = conn->FetchPrepared(selectSql, selectParams);

            auto isPlain = [](const std::string& val) -> bool {
                if (val.empty()) return false;
                if (val.rfind("omni_", 0) == 0) return true;
                if (val.rfind("EAA", 0) == 0) return true;
                bool allDigits = true;
                for (char c : val) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) { allDigits = false; break; }
                }
                if (allDigits && val.length() > 5) return true;
                return false;
            };

            auto ensureEncrypted = [&](const std::string& val) -> std::string {
                if (val.empty()) return "";
                if (isPlain(val)) {
                    return omnisphere::utils::Base64::Encode(val);
                }
                return val;
            };

            std::string encPhoneId = ensureEncrypted(settings.phoneId);
            std::string encApiToken = ensureEncrypted(settings.apiToken);
            std::string encBusinessAccountId = ensureEncrypted(settings.businessAccountId);
            std::string encWebhookVerifyToken = ensureEncrypted(settings.webhookVerifyToken);

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

                auto updateQuery = omnisphere::types::BuildUpdateQuery("\"WhatsAppSettings\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(std::string("DEFAULT")));
                if (!conn->RunPrepared(updateQuery.Query, updateQuery.Parameters))
                {
                    conn->RollbackTransaction();
                    return false;
                }
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
                if (!conn->RunPrepared(sql, params))
                {
                    conn->RollbackTransaction();
                    return false;
                }
            }
            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[WhatsAppRepository::SaveSettings Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    int WhatsAppRepository::GetOrCreateConversation(const std::string& customerPhone, const std::string& customerName) const
    {
        if (!m_dbPool || customerPhone.empty()) return 0;
        auto conn = m_dbPool->Acquire();
        try
        {
            std::vector<std::string> selectFields = {"\"Entry\""};
            std::vector<omnisphere::types::Condition> conditions = {{"", "\"CustomerPhone\"", "=", "?"}};
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string selectSql = "SELECT " + qp.SelectClause + " FROM \"WhatsAppConversations\" WHERE " + qp.WhereClause + " LIMIT 1";
            std::vector<omnisphere::types::SQLParam> selectParams = { omnisphere::types::MakeSQLParam(customerPhone) };
            auto dt = conn->FetchPrepared(selectSql, selectParams);
            if (!dt.IsEmpty())
            {
                const auto& row = dt[0];
                return row["Entry"].GetOptional<int>().value_or(0);
            }

            conn->BeginTransaction();
            omnisphere::repositories::IdentityRepository identityRepo(m_dbPool);
            std::string code = identityRepo.GetNextCode(conn, "WhatsAppConversation", "WAC");
            if (code.empty()) code = "WAC1";

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
            if (!insertDt.IsEmpty())
            {
                conn->CommitTransaction();
                const auto& insertRow = insertDt[0];
                return insertRow["Entry"].GetOptional<int>().value_or(0);
            }
            conn->RollbackTransaction();
            return 0;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[WhatsAppRepository::GetOrCreateConversation Exception] " << ex.what() << std::endl;
            return 0;
        }
    }

    bool WhatsAppRepository::SetConversationContext(const std::string& customerPhone, const std::string& reservationCode) const
    {
        if (!m_dbPool || customerPhone.empty()) return false;
        try
        {
            int convEntry = GetOrCreateConversation(customerPhone);
            if (convEntry <= 0) return false;

            auto conn = m_dbPool->Acquire();
            std::string sql = "UPDATE \"WhatsAppConversations\" SET \"ContextReservationCode\" = ?, \"UpdateDate\" = CURRENT_TIMESTAMP WHERE \"Entry\" = ?";
            return conn->RunPrepared(sql, {
                omnisphere::types::MakeSQLParam(reservationCode),
                omnisphere::types::MakeSQLParam(convEntry)
            });
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::SetConversationContext Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    std::optional<std::string> WhatsAppRepository::GetConversationContext(const std::string& customerPhone) const
    {
        if (!m_dbPool || customerPhone.empty()) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string digits;
            for (char c : customerPhone) {
                if (std::isdigit(static_cast<unsigned char>(c))) digits += c;
            }
            std::string phoneSuffix = (digits.length() > 10) ? digits.substr(digits.length() - 10) : digits;

            std::string sql =
                "SELECT \"ContextReservationCode\" FROM \"WhatsAppConversations\" "
                "WHERE (\"CustomerPhone\" = ? OR RIGHT(REGEXP_REPLACE(\"CustomerPhone\", '[^0-9]', '', 'g'), 10) = ?) "
                "  AND (\"Status\" IS NULL OR \"Status\" != 'CLOSED') "
                "  AND \"IsActive\" = true "
                "ORDER BY \"LastMessageDate\" DESC LIMIT 1";

            auto dt = conn->FetchPrepared(sql, {
                omnisphere::types::MakeSQLParam(customerPhone),
                omnisphere::types::MakeSQLParam(phoneSuffix)
            });

            if (dt.IsEmpty()) return std::nullopt;
            auto ctxOpt = dt[0]["ContextReservationCode"].GetOptional<std::string>();
            if (ctxOpt.has_value() && !ctxOpt->empty()) return ctxOpt;
            return std::nullopt;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::GetConversationContext Exception] " << ex.what() << std::endl;
            return std::nullopt;
        }
    }

    bool WhatsAppRepository::ClearConversationContext(const std::string& customerPhone) const
    {
        if (!m_dbPool || customerPhone.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string digits;
            for (char c : customerPhone) {
                if (std::isdigit(static_cast<unsigned char>(c))) digits += c;
            }
            std::string phoneSuffix = (digits.length() > 10) ? digits.substr(digits.length() - 10) : digits;

            std::string sql =
                "UPDATE \"WhatsAppConversations\" SET \"ContextReservationCode\" = NULL, \"UpdateDate\" = CURRENT_TIMESTAMP "
                "WHERE \"CustomerPhone\" = ? OR RIGHT(REGEXP_REPLACE(\"CustomerPhone\", '[^0-9]', '', 'g'), 10) = ?";

            return conn->RunPrepared(sql, {
                omnisphere::types::MakeSQLParam(customerPhone),
                omnisphere::types::MakeSQLParam(phoneSuffix)
            });
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::ClearConversationContext Exception] " << ex.what() << std::endl;
            return false;
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
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            omnisphere::repositories::IdentityRepository identityRepo(m_dbPool);
            std::string msgCode = (msg.code.has_value() && !msg.code.value().empty() && msg.code.value().rfind("WAM", 0) == 0)
                ? msg.code.value()
                : identityRepo.GetNextCode(conn, "WhatsAppMessage", "WAM");
            if (msgCode.empty()) msgCode = "WAM1";

            std::string waId = msg.whatsAppId.value_or("");
            if (waId.empty() && msg.code.has_value() && msg.code.value().rfind("WAM", 0) != 0)
            {
                waId = msg.code.value();
            }

            std::vector<std::string> cols = {
                "\"Code\"", "\"WhatsAppId\"", "\"ConversationEntry\"", "\"SenderType\"", "\"MessageType\"",
                "\"TemplateName\"", "\"Content\"", "\"MediaUrl\"", "\"MediaId\"", "\"MediaMimeType\"", "\"MediaFileName\"",
                "\"ContextWhatsAppId\"", "\"MessageTimestamp\"", "\"Status\"",
                "\"ResponsePayload\"", "\"ErrorMessage\"", "\"SentBy\""
            };
            std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppMessages\"", cols);

            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(msgCode),
                omnisphere::types::MakeSQLParam(waId),
                omnisphere::types::MakeSQLParam(msg.conversationEntry),
                omnisphere::types::MakeSQLParam(msg.senderType),
                omnisphere::types::MakeSQLParam(msg.messageType),
                omnisphere::types::MakeSQLParam(msg.templateName),
                omnisphere::types::MakeSQLParam(msg.content),
                omnisphere::types::MakeSQLParam(msg.mediaUrl),
                omnisphere::types::MakeSQLParam(msg.mediaId),
                omnisphere::types::MakeSQLParam(msg.mediaMimeType),
                omnisphere::types::MakeSQLParam(msg.mediaFileName),
                omnisphere::types::MakeSQLParam(msg.contextWhatsAppId),
                omnisphere::types::MakeSQLParam(msg.messageTimestamp),
                omnisphere::types::MakeSQLParam(msg.status),
                omnisphere::types::MakeSQLParam(msg.responsePayload),
                omnisphere::types::MakeSQLParam(msg.errorMessage),
                omnisphere::types::MakeSQLParam(msg.sentBy)
            };

            bool ok = conn->RunPrepared(sql, params);
            if (!ok)
            {
                conn->RollbackTransaction();
                return false;
            }

            if (msg.content.has_value() && !msg.content.value().empty())
            {
                std::string convSql = "UPDATE \"WhatsAppConversations\" SET \"LastMessageText\" = ?, \"LastMessageDate\" = CURRENT_TIMESTAMP, \"UnreadCount\" = \"UnreadCount\" + ? WHERE \"Entry\" = ?";
                int unreadInc = (msg.senderType == "INBOUND") ? 1 : 0;
                std::vector<omnisphere::types::SQLParam> convParams = {
                    omnisphere::types::MakeSQLParam(msg.content.value()),
                    omnisphere::types::MakeSQLParam(unreadInc),
                    omnisphere::types::MakeSQLParam(msg.conversationEntry)
                };
                if (!conn->RunPrepared(convSql, convParams))
                {
                    conn->RollbackTransaction();
                    return false;
                }
            }

            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[WhatsAppRepository::LogMessage Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool WhatsAppRepository::UpdateMessageStatus(const std::string& wamidCode, const std::string& newStatus, const std::string& responsePayload) const
    {
        if (!m_dbPool || wamidCode.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            std::string upperStatus = newStatus;
            std::transform(upperStatus.begin(), upperStatus.end(), upperStatus.begin(), ::toupper);

            std::string sql;
            if (upperStatus == "DELIVERED")
            {
                sql = "UPDATE \"WhatsAppMessages\" SET \"Status\" = ?, \"DeliveredDate\" = CURRENT_TIMESTAMP, \"ResponsePayload\" = ? WHERE \"WhatsAppId\" = ? OR \"Code\" = ?";
            }
            else if (upperStatus == "READ")
            {
                sql = "UPDATE \"WhatsAppMessages\" SET \"Status\" = ?, \"ReadDate\" = CURRENT_TIMESTAMP, \"ResponsePayload\" = ? WHERE \"WhatsAppId\" = ? OR \"Code\" = ?";
            }
            else
            {
                sql = "UPDATE \"WhatsAppMessages\" SET \"Status\" = ?, \"ResponsePayload\" = ? WHERE \"WhatsAppId\" = ? OR \"Code\" = ?";
            }

            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(upperStatus),
                omnisphere::types::MakeSQLParam(responsePayload),
                omnisphere::types::MakeSQLParam(wamidCode),
                omnisphere::types::MakeSQLParam(wamidCode)
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
            std::cerr << "[WhatsAppRepository::UpdateMessageStatus Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool WhatsAppRepository::IsMessageProcessed(const std::string& wamidCode) const
    {
        if (!m_dbPool || wamidCode.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = "SELECT \"Entry\" FROM \"WhatsAppMessages\" WHERE \"WhatsAppId\" = ? OR \"Code\" = ? LIMIT 1";
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(wamidCode),
                omnisphere::types::MakeSQLParam(wamidCode)
            };
            auto dt = conn->FetchPrepared(sql, params);
            return !dt.IsEmpty();
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::IsMessageProcessed Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    std::vector<omnisphere::models::CustomButton> WhatsAppRepository::GetButtonsForMessage(int messageEntry) const
    {
        if (!m_dbPool || messageEntry <= 0) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = "SELECT \"Entry\", \"MessageEntry\", \"ButtonId\", \"Title\", \"ActionType\", \"ActionPayload\", \"SortOrder\" FROM \"CustomButtons\" WHERE \"MessageEntry\" = ? ORDER BY \"SortOrder\" ASC";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(messageEntry) };
            auto dt = conn->FetchPrepared(sql, params);
            return omnisphere::types::DataTableToModels<omnisphere::models::CustomButton>(dt);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::GetButtonsForMessage Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    std::vector<omnisphere::models::CustomButton> WhatsAppRepository::GetButtonsForMessageCode(const std::string& messageCode) const
    {
        if (!m_dbPool || messageCode.empty()) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto dtMsg = conn->FetchPrepared("SELECT \"Entry\" FROM \"CustomMessages\" WHERE \"Code\" = ? LIMIT 1", { omnisphere::types::MakeSQLParam(messageCode) });
            if (dtMsg.IsEmpty()) return {};

            const auto& msgRow = dtMsg[0];
            int msgEntry = msgRow["Entry"].GetOptional<int>().value_or(0);
            auto btnFields = omnisphere::types::FilterModelFields<omnisphere::models::CustomButton>({});
            auto qp = omnisphere::types::BuildQueryParts(btnFields, { {"", "\"MessageEntry\"", "=", "?"} });
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"CustomButtons\" WHERE " + qp.WhereClause + " ORDER BY \"SortOrder\" ASC";
            auto dt = conn->FetchPrepared(sql, { omnisphere::types::MakeSQLParam(msgEntry) });
            return omnisphere::types::DataTableToModels<omnisphere::models::CustomButton>(dt);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::GetButtonsForMessageCode Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    std::vector<omnisphere::models::CustomMessageParameter> WhatsAppRepository::GetParametersForMessage(const std::string& messageCode) const
    {
        if (!m_dbPool || messageCode.empty()) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto paramFields = omnisphere::types::FilterModelFields<omnisphere::models::CustomMessageParameter>({});
            auto qp = omnisphere::types::BuildQueryParts(paramFields, { {"", "\"MessageCode\"", "=", "?"} });
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"CustomMessageParameters\" WHERE " + qp.WhereClause + " ORDER BY \"SortOrder\" ASC";
            std::vector<omnisphere::types::SQLParam> params = { omnisphere::types::MakeSQLParam(messageCode) };
            auto dt = conn->FetchPrepared(sql, params);
            return omnisphere::types::DataTableToModels<omnisphere::models::CustomMessageParameter>(dt);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::GetParametersForMessage Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    std::optional<omnisphere::models::CustomMessage> WhatsAppRepository::GetCustomMessageByCode(const std::string& code) const
    {
        if (!m_dbPool || code.empty()) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::CustomMessage>({});
            std::vector<omnisphere::types::Condition> conditions = {
                {"", "\"Code\"", "=", "?"},
                {"", "\"IsActive\"", "=", "?"}
            };
            auto qp = omnisphere::types::BuildQueryParts(selectFields, conditions);
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"CustomMessages\" WHERE " + qp.WhereClause + " LIMIT 1";
            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(code),
                omnisphere::types::MakeSQLParam(true)
            };
            auto dt = conn->FetchPrepared(sql, params);
            if (dt.IsEmpty()) return std::nullopt;

            auto msg = omnisphere::types::FromDataRow<omnisphere::models::CustomMessage>(dt[0]);
            
            // Try fetching buttons by MessageCode first, then fallback to MessageEntry
            msg.buttons = GetButtonsForMessageCode(msg.code);
            if (msg.buttons.empty())
            {
                msg.buttons = GetButtonsForMessage(msg.entry);
            }
            msg.parameters = GetParametersForMessage(msg.code);

            // Self-healing: Ensure TPL_WELCOME_WITH_RESERVATION is configured as INTERACTIVE_BUTTON with BTN_DETAILS_{folio}
            if (msg.code == "TPL_WELCOME_WITH_RESERVATION" && (msg.messageType != "INTERACTIVE_BUTTON" || msg.buttons.empty()))
            {
                try
                {
                    conn->BeginTransaction();
                    conn->RunPrepared("UPDATE \"CustomMessages\" SET \"MessageType\" = 'INTERACTIVE_BUTTON' WHERE \"Code\" = 'TPL_WELCOME_WITH_RESERVATION'", {});
                    conn->RunPrepared("DELETE FROM \"CustomButtons\" WHERE \"MessageCode\" = 'TPL_WELCOME_WITH_RESERVATION'", {});
                    conn->RunPrepared("INSERT INTO \"CustomButtons\" (\"MessageCode\", \"ButtonId\", \"Title\", \"SortOrder\", \"CreatedBy\") VALUES ('TPL_WELCOME_WITH_RESERVATION', 'BTN_DETAILS_{folio}', 'Ver Detalles', 1, 'SYSTEM')", {});
                    conn->CommitTransaction();
                    
                    msg.messageType = "INTERACTIVE_BUTTON";
                    msg.buttons = GetButtonsForMessageCode(msg.code);
                    if (msg.buttons.empty())
                    {
                        msg.buttons = GetButtonsForMessage(msg.entry);
                    }
                }
                catch (const std::exception& ex)
                {
                    conn->RollbackTransaction();
                    std::cerr << "[Self-healing Exception] " << ex.what() << std::endl;
                }
            }

            return msg;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::GetCustomMessageByCode Exception] " << ex.what() << std::endl;
            return std::nullopt;
        }
    }

    std::vector<omnisphere::models::CustomMessage> WhatsAppRepository::GetAllCustomMessages() const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::CustomMessage>({});
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"CustomMessages\" ORDER BY \"Entry\" ASC";
            std::vector<omnisphere::types::SQLParam> params;
            auto dt = conn->FetchPrepared(sql, params);

            auto messages = omnisphere::types::DataTableToModels<omnisphere::models::CustomMessage>(dt);
            for (auto& msg : messages)
            {
                msg.buttons = GetButtonsForMessage(msg.entry);
                msg.parameters = GetParametersForMessage(msg.code);
            }
            return messages;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::GetAllCustomMessages Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    bool WhatsAppRepository::SaveCustomMessage(const omnisphere::models::CustomMessage& msg) const
    {
        if (!m_dbPool || msg.code.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            auto existing = GetCustomMessageByCode(msg.code);
            bool ok = false;
            int insertedEntry = 0;

            if (!existing.has_value())
            {
                std::vector<std::string> cols = {
                    "\"Code\"", "\"Title\"", "\"MessageType\"", "\"HeaderType\"",
                    "\"HeaderContent\"", "\"BodyTemplate\"", "\"FooterText\"",
                    "\"MetaTemplateId\"", "\"MetaStatus\"", "\"MetaCategory\"", "\"MetaRejectReason\"",
                    "\"IsActive\"", "\"CreatedBy\""
                };
                std::string sql = omnisphere::types::BuildInsertQuery("\"CustomMessages\"", cols);
                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(msg.code),
                    omnisphere::types::MakeSQLParam(msg.title),
                    omnisphere::types::MakeSQLParam(msg.messageType),
                    omnisphere::types::MakeSQLParam(msg.headerType),
                    omnisphere::types::MakeSQLParam(msg.headerContent.value_or("")),
                    omnisphere::types::MakeSQLParam(msg.bodyTemplate),
                    omnisphere::types::MakeSQLParam(msg.footerText.value_or("")),
                    omnisphere::types::MakeSQLParam(msg.metaTemplateId.value_or("")),
                    omnisphere::types::MakeSQLParam(msg.metaStatus.value_or("NONE")),
                    omnisphere::types::MakeSQLParam(msg.metaCategory.value_or("UTILITY")),
                    omnisphere::types::MakeSQLParam(msg.metaRejectReason.value_or("")),
                    omnisphere::types::MakeSQLParam(msg.isActive),
                    omnisphere::types::MakeSQLParam(msg.createdBy)
                };
                ok = conn->RunPrepared(sql, params);
                if (ok)
                {
                    auto saved = GetCustomMessageByCode(msg.code);
                    if (saved.has_value()) insertedEntry = saved->entry;
                }
            }
            else
            {
                std::vector<omnisphere::types::ColumnValue> updateCols = {
                    {"\"Title\"", omnisphere::types::MakeSQLParam(msg.title)},
                    {"\"MessageType\"", omnisphere::types::MakeSQLParam(msg.messageType)},
                    {"\"HeaderType\"", omnisphere::types::MakeSQLParam(msg.headerType)},
                    {"\"HeaderContent\"", omnisphere::types::MakeSQLParam(msg.headerContent.value_or(""))},
                    {"\"BodyTemplate\"", omnisphere::types::MakeSQLParam(msg.bodyTemplate)},
                    {"\"FooterText\"", omnisphere::types::MakeSQLParam(msg.footerText.value_or(""))},
                    {"\"MetaTemplateId\"", omnisphere::types::MakeSQLParam(msg.metaTemplateId.value_or(""))},
                    {"\"MetaStatus\"", omnisphere::types::MakeSQLParam(msg.metaStatus.value_or("NONE"))},
                    {"\"MetaCategory\"", omnisphere::types::MakeSQLParam(msg.metaCategory.value_or("UTILITY"))},
                    {"\"MetaRejectReason\"", omnisphere::types::MakeSQLParam(msg.metaRejectReason.value_or(""))},
                    {"\"IsActive\"", omnisphere::types::MakeSQLParam(msg.isActive)}
                };
                auto updateQuery = omnisphere::types::BuildUpdateQuery("\"CustomMessages\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(msg.code));
                ok = conn->RunPrepared(updateQuery.Query, updateQuery.Parameters);
                insertedEntry = existing->entry;
            }

            if (!ok)
            {
                conn->RollbackTransaction();
                return false;
            }

            if (!msg.buttons.empty())
            {
                if (!SaveButtonsForMessage(insertedEntry, msg.code, msg.buttons))
                {
                    conn->RollbackTransaction();
                    return false;
                }
            }

            if (!msg.parameters.empty())
            {
                if (!SaveParametersForMessage(msg.code, msg.parameters))
                {
                    conn->RollbackTransaction();
                    return false;
                }
            }

            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[WhatsAppRepository::SaveCustomMessage Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool WhatsAppRepository::UpdateMetaTemplateStatus(
        const std::string& metaTemplateId,
        const std::string& templateName,
        const std::string& metaStatus,
        const std::string& rejectReason
    ) const
    {
        if (!m_dbPool) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            std::string sql =
                "UPDATE \"WhatsAppTemplates\" SET "
                "  \"Status\" = ?, "
                "  \"RejectReason\" = ?, "
                "  \"MetaTemplateId\" = COALESCE(NULLIF(?, ''), \"MetaTemplateId\"), "
                "  \"UpdateDate\" = CURRENT_TIMESTAMP "
                "WHERE (\"MetaTemplateId\" IS NOT NULL AND \"MetaTemplateId\" = ?) "
                "   OR \"Code\" = ? "
                "   OR LOWER(\"Code\") = LOWER(?)";

            std::vector<omnisphere::types::SQLParam> params = {
                omnisphere::types::MakeSQLParam(metaStatus),
                omnisphere::types::MakeSQLParam(rejectReason),
                omnisphere::types::MakeSQLParam(metaTemplateId),
                omnisphere::types::MakeSQLParam(metaTemplateId),
                omnisphere::types::MakeSQLParam(templateName),
                omnisphere::types::MakeSQLParam(templateName)
            };
            conn->RunPrepared(sql, params);

            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[WhatsAppRepository::UpdateMetaTemplateStatus Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool WhatsAppRepository::SaveButtonsForMessage(
        int messageEntry,
        const std::string& messageCode,
        const std::vector<omnisphere::models::CustomButton>& buttons
    ) const
    {
        if (!m_dbPool || (messageEntry <= 0 && messageCode.empty())) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            if (messageEntry > 0)
            {
                if (!conn->RunPrepared("DELETE FROM \"CustomButtons\" WHERE \"MessageEntry\" = ?",
                    { omnisphere::types::MakeSQLParam(messageEntry) }))
                {
                    conn->RollbackTransaction();
                    return false;
                }
            }
            if (!messageCode.empty())
            {
                if (!conn->RunPrepared("DELETE FROM \"CustomButtons\" WHERE \"MessageCode\" = ?",
                    { omnisphere::types::MakeSQLParam(messageCode) }))
                {
                    conn->RollbackTransaction();
                    return false;
                }
            }

            int sortOrder = 1;
            for (const auto& btn : buttons)
            {
                std::vector<std::string> cols = {
                    "\"MessageEntry\"", "\"ButtonId\"", "\"Title\"",
                    "\"ActionType\"", "\"ActionPayload\"", "\"SortOrder\"", "\"CreatedBy\""
                };
                std::string sql = omnisphere::types::BuildInsertQuery("\"CustomButtons\"", cols);
                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(messageEntry),
                    omnisphere::types::MakeSQLParam(btn.buttonId),
                    omnisphere::types::MakeSQLParam(btn.title),
                    omnisphere::types::MakeSQLParam(btn.actionType),
                    omnisphere::types::MakeSQLParam(btn.actionPayload.value_or("")),
                    omnisphere::types::MakeSQLParam(btn.sortOrder > 0 ? btn.sortOrder : sortOrder++),
                    omnisphere::types::MakeSQLParam(!btn.createdBy.empty() ? btn.createdBy : std::string("SYSTEM"))
                };
                if (!conn->RunPrepared(sql, params))
                {
                    conn->RollbackTransaction();
                    return false;
                }
            }
            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[WhatsAppRepository::SaveButtonsForMessage Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool WhatsAppRepository::SaveParametersForMessage(
        const std::string& messageCode,
        const std::vector<omnisphere::models::CustomMessageParameter>& parameters
    ) const
    {
        if (!m_dbPool || messageCode.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            if (!conn->RunPrepared("DELETE FROM \"CustomMessageParameters\" WHERE \"MessageCode\" = ?",
                { omnisphere::types::MakeSQLParam(messageCode) }))
            {
                conn->RollbackTransaction();
                return false;
            }

            int sortOrder = 1;
            for (const auto& param : parameters)
            {
                std::vector<std::string> cols = {
                    "\"MessageCode\"", "\"ParamKey\"", "\"ParamName\"", "\"DataType\"",
                    "\"DefaultValue\"", "\"IsRequired\"", "\"Description\"", "\"SortOrder\"", "\"CreatedBy\""
                };
                std::string sql = omnisphere::types::BuildInsertQuery("\"CustomMessageParameters\"", cols);
                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(messageCode),
                    omnisphere::types::MakeSQLParam(param.paramKey),
                    omnisphere::types::MakeSQLParam(param.paramName.empty() ? param.paramKey : param.paramName),
                    omnisphere::types::MakeSQLParam(param.dataType.empty() ? "STRING" : param.dataType),
                    omnisphere::types::MakeSQLParam(param.defaultValue.value_or("")),
                    omnisphere::types::MakeSQLParam(param.isRequired),
                    omnisphere::types::MakeSQLParam(param.description.value_or("")),
                    omnisphere::types::MakeSQLParam(param.sortOrder > 0 ? param.sortOrder : sortOrder++),
                    omnisphere::types::MakeSQLParam(!param.createdBy.empty() ? param.createdBy : std::string("SYSTEM"))
                };
                if (!conn->RunPrepared(sql, params))
                {
                    conn->RollbackTransaction();
                    return false;
                }
            }
            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[WhatsAppRepository::SaveParametersForMessage Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool WhatsAppRepository::HasRecentWelcomeCard(const std::string& phone, int minutesWindow) const
    {
        if (!m_dbPool || phone.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string digits;
            for (char c : phone)
            {
                if (std::isdigit(static_cast<unsigned char>(c))) digits += c;
            }
            std::string suffix = (digits.length() > 10) ? digits.substr(digits.length() - 10) : digits;

            // 1. Consultar la conversación por teléfono del cliente (sin JOINs)
            auto dtConv = conn->FetchPrepared(
                "SELECT \"Entry\" FROM \"WhatsAppConversations\" "
                "WHERE (\"CustomerPhone\" ILIKE ? OR RIGHT(REGEXP_REPLACE(\"CustomerPhone\", '[^0-9]', '', 'g'), 10) = ?) "
                "ORDER BY \"Entry\" DESC LIMIT 1",
                { omnisphere::types::MakeSQLParam("%" + suffix + "%"), omnisphere::types::MakeSQLParam(suffix) }
            );

            if (dtConv.IsEmpty()) return false;

            const auto& convRow = dtConv[0];
            int convEntry = convRow["Entry"].GetOptional<int>().value_or(0);

            // 2. Consultar mensajes recientes para esa conversación
            auto dtMsg = conn->FetchPrepared(
                "SELECT \"Entry\" FROM \"WhatsAppMessages\" "
                "WHERE \"ConversationEntry\" = ? "
                "  AND \"SenderType\" = 'OUTBOUND' "
                "  AND \"MessageType\" = 'interactive' "
                "  AND (\"Content\" ILIKE '%Ver Detalles%' OR \"Content\" ILIKE '%BTN_DETAILS%' OR \"Content\" ILIKE '%TPL_WELCOME_WITH_RESERVATION%') "
                "  AND \"CreateDate\" >= (NOW() - (INTERVAL '1 minute' * ?)) "
                "LIMIT 1",
                { omnisphere::types::MakeSQLParam(convEntry), omnisphere::types::MakeSQLParam(minutesWindow) }
            );

            return !dtMsg.IsEmpty();
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::HasRecentWelcomeCard Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    std::optional<omnisphere::models::WhatsAppTemplate> WhatsAppRepository::GetWhatsAppTemplateByCode(const std::string& code, const std::vector<std::string>& requestedFields) const
    {
        if (!m_dbPool || code.empty()) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplate>({});
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"WhatsAppTemplates\" WHERE \"Code\" = ? OR \"TemplateName\" = ? LIMIT 1";
            auto dt = conn->FetchPrepared(sql, { omnisphere::types::MakeSQLParam(code), omnisphere::types::MakeSQLParam(code) });
            if (dt.IsEmpty()) return std::nullopt;

            auto tmpl = omnisphere::types::FromDataRow<omnisphere::models::WhatsAppTemplate>(dt[0]);

            // Header
            auto headerFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateHeader>({});
            auto qpH = omnisphere::types::BuildQueryParts(headerFields, { {"", "\"TemplateEntry\"", "=", "?"} });
            std::vector<omnisphere::types::SQLParam> hParams = { omnisphere::types::MakeSQLParam(tmpl.entry) };
            auto dtH = conn->FetchPrepared("SELECT " + qpH.SelectClause + " FROM \"WhatsAppTemplateHeaders\" WHERE " + qpH.WhereClause + " LIMIT 1", hParams);
            if (!dtH.IsEmpty())
            {
                auto header = omnisphere::types::FromDataRow<omnisphere::models::WhatsAppTemplateHeader>(dtH[0]);
                auto hvFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateHeaderVariable>({});
                auto qpHV = omnisphere::types::BuildQueryParts(hvFields, { {"", "\"HeaderEntry\"", "=", "?"} });
                std::vector<omnisphere::types::SQLParam> hvParams = { omnisphere::types::MakeSQLParam(header.entry) };
                auto dtHV = conn->FetchPrepared("SELECT " + qpHV.SelectClause + " FROM \"WhatsAppTemplateHeaderVariables\" WHERE " + qpHV.WhereClause + " ORDER BY \"VarIndex\" ASC", hvParams);
                header.variables = omnisphere::types::DataTableToModels<omnisphere::models::WhatsAppTemplateHeaderVariable>(dtHV);
                tmpl.header = header;
            }

            // Body
            auto bodyFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateBody>({});
            auto qpB = omnisphere::types::BuildQueryParts(bodyFields, { {"", "\"TemplateEntry\"", "=", "?"} });
            std::vector<omnisphere::types::SQLParam> bParams = { omnisphere::types::MakeSQLParam(tmpl.entry) };
            auto dtB = conn->FetchPrepared("SELECT " + qpB.SelectClause + " FROM \"WhatsAppTemplateBodies\" WHERE " + qpB.WhereClause + " LIMIT 1", bParams);
            if (!dtB.IsEmpty())
            {
                auto body = omnisphere::types::FromDataRow<omnisphere::models::WhatsAppTemplateBody>(dtB[0]);
                auto bvFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateBodyVariable>({});
                auto qpBV = omnisphere::types::BuildQueryParts(bvFields, { {"", "\"BodyEntry\"", "=", "?"} });
                std::vector<omnisphere::types::SQLParam> bvParams = { omnisphere::types::MakeSQLParam(body.entry) };
                auto dtBV = conn->FetchPrepared("SELECT " + qpBV.SelectClause + " FROM \"WhatsAppTemplateBodyVariables\" WHERE " + qpBV.WhereClause + " ORDER BY \"VarIndex\" ASC", bvParams);
                body.variables = omnisphere::types::DataTableToModels<omnisphere::models::WhatsAppTemplateBodyVariable>(dtBV);
                tmpl.body = body;
            }

            // Footer
            auto footerFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateFooter>({});
            auto qpF = omnisphere::types::BuildQueryParts(footerFields, { {"", "\"TemplateEntry\"", "=", "?"} });
            std::vector<omnisphere::types::SQLParam> fParams = { omnisphere::types::MakeSQLParam(tmpl.entry) };
            auto dtF = conn->FetchPrepared("SELECT " + qpF.SelectClause + " FROM \"WhatsAppTemplateFooters\" WHERE " + qpF.WhereClause + " LIMIT 1", fParams);
            if (!dtF.IsEmpty())
            {
                auto footer = omnisphere::types::FromDataRow<omnisphere::models::WhatsAppTemplateFooter>(dtF[0]);
                auto fvFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateFooterVariable>({});
                auto qpFV = omnisphere::types::BuildQueryParts(fvFields, { {"", "\"FooterEntry\"", "=", "?"} });
                std::vector<omnisphere::types::SQLParam> fvParams = { omnisphere::types::MakeSQLParam(footer.entry) };
                auto dtFV = conn->FetchPrepared("SELECT " + qpFV.SelectClause + " FROM \"WhatsAppTemplateFooterVariables\" WHERE " + qpFV.WhereClause + " ORDER BY \"VarIndex\" ASC", fvParams);
                footer.variables = omnisphere::types::DataTableToModels<omnisphere::models::WhatsAppTemplateFooterVariable>(dtFV);
                tmpl.footer = footer;
            }

            // Buttons
            auto btnFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateButton>({});
            auto qpBtn = omnisphere::types::BuildQueryParts(btnFields, { {"", "\"TemplateEntry\"", "=", "?"} });
            std::vector<omnisphere::types::SQLParam> btnParams = { omnisphere::types::MakeSQLParam(tmpl.entry) };
            auto dtBtn = conn->FetchPrepared("SELECT " + qpBtn.SelectClause + " FROM \"WhatsAppTemplateButtons\" WHERE " + qpBtn.WhereClause + " ORDER BY \"SortOrder\" ASC", btnParams);
            tmpl.buttons = omnisphere::types::DataTableToModels<omnisphere::models::WhatsAppTemplateButton>(dtBtn);

            return tmpl;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::GetWhatsAppTemplateByCode Exception] " << ex.what() << std::endl;
            return std::nullopt;
        }
    }

    std::vector<omnisphere::models::WhatsAppTemplate> WhatsAppRepository::GetAllWhatsAppTemplates(const std::vector<std::string>& requestedFields) const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            auto selectFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplate>({});
            auto qp = omnisphere::types::BuildQueryParts(selectFields, {});
            std::string sql = "SELECT " + qp.SelectClause + " FROM \"WhatsAppTemplates\" ORDER BY \"Entry\" DESC";
            std::vector<omnisphere::types::SQLParam> emptyParams;
            auto dt = conn->FetchPrepared(sql, emptyParams);
            auto list = omnisphere::types::DataTableToModels<omnisphere::models::WhatsAppTemplate>(dt);

            auto headerFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateHeader>({});
            auto hvFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateHeaderVariable>({});
            auto bodyFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateBody>({});
            auto bvFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateBodyVariable>({});
            auto footerFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateFooter>({});
            auto fvFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateFooterVariable>({});
            auto btnFields = omnisphere::types::FilterModelFields<omnisphere::models::WhatsAppTemplateButton>({});

            for (auto& tmpl : list)
            {
                // Header
                auto qpH = omnisphere::types::BuildQueryParts(headerFields, { {"", "\"TemplateEntry\"", "=", "?"} });
                std::vector<omnisphere::types::SQLParam> hParams = { omnisphere::types::MakeSQLParam(tmpl.entry) };
                auto dtH = conn->FetchPrepared("SELECT " + qpH.SelectClause + " FROM \"WhatsAppTemplateHeaders\" WHERE " + qpH.WhereClause + " LIMIT 1", hParams);
                if (!dtH.IsEmpty())
                {
                    auto header = omnisphere::types::FromDataRow<omnisphere::models::WhatsAppTemplateHeader>(dtH[0]);
                    auto qpHV = omnisphere::types::BuildQueryParts(hvFields, { {"", "\"HeaderEntry\"", "=", "?"} });
                    std::vector<omnisphere::types::SQLParam> hvParams = { omnisphere::types::MakeSQLParam(header.entry) };
                    auto dtHV = conn->FetchPrepared("SELECT " + qpHV.SelectClause + " FROM \"WhatsAppTemplateHeaderVariables\" WHERE " + qpHV.WhereClause + " ORDER BY \"VarIndex\" ASC", hvParams);
                    header.variables = omnisphere::types::DataTableToModels<omnisphere::models::WhatsAppTemplateHeaderVariable>(dtHV);
                    tmpl.header = header;
                }

                // Body
                auto qpB = omnisphere::types::BuildQueryParts(bodyFields, { {"", "\"TemplateEntry\"", "=", "?"} });
                std::vector<omnisphere::types::SQLParam> bParams = { omnisphere::types::MakeSQLParam(tmpl.entry) };
                auto dtB = conn->FetchPrepared("SELECT " + qpB.SelectClause + " FROM \"WhatsAppTemplateBodies\" WHERE " + qpB.WhereClause + " LIMIT 1", bParams);
                if (!dtB.IsEmpty())
                {
                    auto body = omnisphere::types::FromDataRow<omnisphere::models::WhatsAppTemplateBody>(dtB[0]);
                    auto qpBV = omnisphere::types::BuildQueryParts(bvFields, { {"", "\"BodyEntry\"", "=", "?"} });
                    std::vector<omnisphere::types::SQLParam> bvParams = { omnisphere::types::MakeSQLParam(body.entry) };
                    auto dtBV = conn->FetchPrepared("SELECT " + qpBV.SelectClause + " FROM \"WhatsAppTemplateBodyVariables\" WHERE " + qpBV.WhereClause + " ORDER BY \"VarIndex\" ASC", bvParams);
                    body.variables = omnisphere::types::DataTableToModels<omnisphere::models::WhatsAppTemplateBodyVariable>(dtBV);
                    tmpl.body = body;
                }

                // Footer
                auto qpF = omnisphere::types::BuildQueryParts(footerFields, { {"", "\"TemplateEntry\"", "=", "?"} });
                std::vector<omnisphere::types::SQLParam> fParams = { omnisphere::types::MakeSQLParam(tmpl.entry) };
                auto dtF = conn->FetchPrepared("SELECT " + qpF.SelectClause + " FROM \"WhatsAppTemplateFooters\" WHERE " + qpF.WhereClause + " LIMIT 1", fParams);
                if (!dtF.IsEmpty())
                {
                    auto footer = omnisphere::types::FromDataRow<omnisphere::models::WhatsAppTemplateFooter>(dtF[0]);
                    auto qpFV = omnisphere::types::BuildQueryParts(fvFields, { {"", "\"FooterEntry\"", "=", "?"} });
                    std::vector<omnisphere::types::SQLParam> fvParams = { omnisphere::types::MakeSQLParam(footer.entry) };
                    auto dtFV = conn->FetchPrepared("SELECT " + qpFV.SelectClause + " FROM \"WhatsAppTemplateFooterVariables\" WHERE " + qpFV.WhereClause + " ORDER BY \"VarIndex\" ASC", fvParams);
                    footer.variables = omnisphere::types::DataTableToModels<omnisphere::models::WhatsAppTemplateFooterVariable>(dtFV);
                    tmpl.footer = footer;
                }

                // Buttons
                auto qpBtn = omnisphere::types::BuildQueryParts(btnFields, { {"", "\"TemplateEntry\"", "=", "?"} });
                std::vector<omnisphere::types::SQLParam> btnParams = { omnisphere::types::MakeSQLParam(tmpl.entry) };
                auto dtBtn = conn->FetchPrepared("SELECT " + qpBtn.SelectClause + " FROM \"WhatsAppTemplateButtons\" WHERE " + qpBtn.WhereClause + " ORDER BY \"SortOrder\" ASC", btnParams);
                tmpl.buttons = omnisphere::types::DataTableToModels<omnisphere::models::WhatsAppTemplateButton>(dtBtn);
            }
            return list;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[WhatsAppRepository::GetAllWhatsAppTemplates Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    bool WhatsAppRepository::SaveWhatsAppTemplate(const omnisphere::models::WhatsAppTemplate& tmpl) const
    {
        if (!m_dbPool || tmpl.code.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();
            int templateEntry = 0;

            auto existingDt = conn->FetchPrepared("SELECT \"Entry\", \"TemplateName\" FROM \"WhatsAppTemplates\" WHERE \"Code\" = ? LIMIT 1",
                { omnisphere::types::MakeSQLParam(tmpl.code) });

            if (existingDt.IsEmpty())
            {
                std::vector<std::string> cols = {
                    "\"Code\"", "\"TemplateName\"", "\"Language\"", "\"Category\"",
                    "\"MetaTemplateId\"", "\"Status\"", "\"RejectReason\"", "\"IsActive\"", "\"CreatedBy\""
                };
                std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplates\"", cols);
                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(tmpl.code),
                    omnisphere::types::MakeSQLParam(tmpl.templateName),
                    omnisphere::types::MakeSQLParam(tmpl.language.empty() ? "es_MX" : tmpl.language),
                    omnisphere::types::MakeSQLParam(tmpl.category.empty() ? "UTILITY" : tmpl.category),
                    omnisphere::types::MakeSQLParam(tmpl.metaTemplateId.value_or("")),
                    omnisphere::types::MakeSQLParam(tmpl.status.empty() ? "PENDING" : tmpl.status),
                    omnisphere::types::MakeSQLParam(tmpl.rejectReason.value_or("")),
                    omnisphere::types::MakeSQLParam(tmpl.isActive),
                    omnisphere::types::MakeSQLParam(!tmpl.createdBy.empty() ? tmpl.createdBy : std::string("system"))
                };
                if (!conn->RunPrepared(sql, params))
                {
                    conn->RollbackTransaction();
                    return false;
                }
                auto insertedDt = conn->FetchPrepared("SELECT \"Entry\" FROM \"WhatsAppTemplates\" WHERE \"Code\" = ? LIMIT 1",
                    { omnisphere::types::MakeSQLParam(tmpl.code) });
                if (insertedDt.IsEmpty()) { conn->RollbackTransaction(); return false; }
                templateEntry = insertedDt[0]["Entry"].GetOptional<int>().value_or(0);
            }
            else
            {
                templateEntry = existingDt[0]["Entry"].GetOptional<int>().value_or(0);
                std::string existingTitle = existingDt[0]["TemplateName"].GetOptional<std::string>().value_or("");
                std::string finalTitle = tmpl.templateName;
                // Si la sincronización de Meta trae como título solo el Code técnico, pero en la BD ya tenemos un título descriptivo personalizado, conservamos el título descriptivo
                if ((finalTitle.empty() || finalTitle == tmpl.code) && !existingTitle.empty() && existingTitle != tmpl.code)
                {
                    finalTitle = existingTitle;
                }

                std::vector<omnisphere::types::ColumnValue> updateCols = {
                    {"\"TemplateName\"", omnisphere::types::MakeSQLParam(finalTitle)},
                    {"\"Language\"", omnisphere::types::MakeSQLParam(tmpl.language)},
                    {"\"Category\"", omnisphere::types::MakeSQLParam(tmpl.category)},
                    {"\"MetaTemplateId\"", omnisphere::types::MakeSQLParam(tmpl.metaTemplateId.value_or(""))},
                    {"\"Status\"", omnisphere::types::MakeSQLParam(tmpl.status)},
                    {"\"RejectReason\"", omnisphere::types::MakeSQLParam(tmpl.rejectReason.value_or(""))},
                    {"\"IsActive\"", omnisphere::types::MakeSQLParam(tmpl.isActive)}
                };
                auto updateQuery = omnisphere::types::BuildUpdateQuery("\"WhatsAppTemplates\"", updateCols, "\"Entry\"", omnisphere::types::MakeSQLParam(templateEntry));
                if (!conn->RunPrepared(updateQuery.Query, updateQuery.Parameters))
                {
                    conn->RollbackTransaction();
                    return false;
                }
            }

            // Clean previous child records to update cleanly
            conn->RunPrepared("DELETE FROM \"WhatsAppTemplateHeaders\" WHERE \"TemplateEntry\" = ?", { omnisphere::types::MakeSQLParam(templateEntry) });
            conn->RunPrepared("DELETE FROM \"WhatsAppTemplateBodies\" WHERE \"TemplateEntry\" = ?", { omnisphere::types::MakeSQLParam(templateEntry) });
            conn->RunPrepared("DELETE FROM \"WhatsAppTemplateFooters\" WHERE \"TemplateEntry\" = ?", { omnisphere::types::MakeSQLParam(templateEntry) });
            conn->RunPrepared("DELETE FROM \"WhatsAppTemplateButtons\" WHERE \"TemplateEntry\" = ?", { omnisphere::types::MakeSQLParam(templateEntry) });

            // 1. Save Header
            if (tmpl.header.has_value())
            {
                const auto& h = *tmpl.header;
                std::vector<std::string> cols = { "\"TemplateEntry\"", "\"HeaderType\"", "\"TextContent\"", "\"MediaUrl\"", "\"CreatedBy\"" };
                std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateHeaders\"", cols);
                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(templateEntry),
                    omnisphere::types::MakeSQLParam(h.headerType),
                    omnisphere::types::MakeSQLParam(h.textContent.value_or("")),
                    omnisphere::types::MakeSQLParam(h.mediaUrl.value_or("")),
                    omnisphere::types::MakeSQLParam(!h.createdBy.empty() ? h.createdBy : std::string("system"))
                };
                conn->RunPrepared(sql, params);

                auto hDt = conn->FetchPrepared("SELECT \"Entry\" FROM \"WhatsAppTemplateHeaders\" WHERE \"TemplateEntry\" = ? LIMIT 1", { omnisphere::types::MakeSQLParam(templateEntry) });
                if (!hDt.IsEmpty())
                {
                    int hEntry = hDt[0]["Entry"].GetOptional<int>().value_or(0);
                    for (const auto& hv : h.variables)
                    {
                        std::vector<std::string> vCols = { "\"HeaderEntry\"", "\"VarName\"", "\"VarIndex\"", "\"ExampleValue\"", "\"CreatedBy\"" };
                        std::string vSql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateHeaderVariables\"", vCols);
                        std::vector<omnisphere::types::SQLParam> vParams = {
                            omnisphere::types::MakeSQLParam(hEntry),
                            omnisphere::types::MakeSQLParam(hv.varName),
                            omnisphere::types::MakeSQLParam(hv.varIndex),
                            omnisphere::types::MakeSQLParam(hv.exampleValue.value_or("")),
                            omnisphere::types::MakeSQLParam(!hv.createdBy.empty() ? hv.createdBy : std::string("system"))
                        };
                        conn->RunPrepared(vSql, vParams);
                    }
                }
            }

            // 2. Save Body
            if (tmpl.body.has_value())
            {
                const auto& b = *tmpl.body;
                std::vector<std::string> cols = { "\"TemplateEntry\"", "\"BodyText\"", "\"CreatedBy\"" };
                std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateBodies\"", cols);
                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(templateEntry),
                    omnisphere::types::MakeSQLParam(b.bodyText),
                    omnisphere::types::MakeSQLParam(!b.createdBy.empty() ? b.createdBy : std::string("system"))
                };
                conn->RunPrepared(sql, params);

                auto bDt = conn->FetchPrepared("SELECT \"Entry\" FROM \"WhatsAppTemplateBodies\" WHERE \"TemplateEntry\" = ? LIMIT 1", { omnisphere::types::MakeSQLParam(templateEntry) });
                if (!bDt.IsEmpty())
                {
                    int bEntry = bDt[0]["Entry"].GetOptional<int>().value_or(0);
                    for (const auto& bv : b.variables)
                    {
                        std::vector<std::string> vCols = { "\"BodyEntry\"", "\"VarName\"", "\"VarIndex\"", "\"ExampleValue\"", "\"CreatedBy\"" };
                        std::string vSql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateBodyVariables\"", vCols);
                        std::vector<omnisphere::types::SQLParam> vParams = {
                            omnisphere::types::MakeSQLParam(bEntry),
                            omnisphere::types::MakeSQLParam(bv.varName),
                            omnisphere::types::MakeSQLParam(bv.varIndex),
                            omnisphere::types::MakeSQLParam(bv.exampleValue.value_or("")),
                            omnisphere::types::MakeSQLParam(!bv.createdBy.empty() ? bv.createdBy : std::string("system"))
                        };
                        conn->RunPrepared(vSql, vParams);
                    }
                }
            }

            // 3. Save Footer
            if (tmpl.footer.has_value())
            {
                const auto& f = *tmpl.footer;
                std::vector<std::string> cols = { "\"TemplateEntry\"", "\"FooterText\"", "\"CreatedBy\"" };
                std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateFooters\"", cols);
                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(templateEntry),
                    omnisphere::types::MakeSQLParam(f.footerText.value_or("")),
                    omnisphere::types::MakeSQLParam(!f.createdBy.empty() ? f.createdBy : std::string("system"))
                };
                conn->RunPrepared(sql, params);

                auto fDt = conn->FetchPrepared("SELECT \"Entry\" FROM \"WhatsAppTemplateFooters\" WHERE \"TemplateEntry\" = ? LIMIT 1", { omnisphere::types::MakeSQLParam(templateEntry) });
                if (!fDt.IsEmpty())
                {
                    int fEntry = fDt[0]["Entry"].GetOptional<int>().value_or(0);
                    for (const auto& fv : f.variables)
                    {
                        std::vector<std::string> vCols = { "\"FooterEntry\"", "\"VarName\"", "\"VarIndex\"", "\"ExampleValue\"", "\"CreatedBy\"" };
                        std::string vSql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateFooterVariables\"", vCols);
                        std::vector<omnisphere::types::SQLParam> vParams = {
                            omnisphere::types::MakeSQLParam(fEntry),
                            omnisphere::types::MakeSQLParam(fv.varName),
                            omnisphere::types::MakeSQLParam(fv.varIndex),
                            omnisphere::types::MakeSQLParam(fv.exampleValue.value_or("")),
                            omnisphere::types::MakeSQLParam(!fv.createdBy.empty() ? fv.createdBy : std::string("system"))
                        };
                        conn->RunPrepared(vSql, vParams);
                    }
                }
            }

            // 4. Save Buttons
            int btnIdx = 1;
            for (const auto& btn : tmpl.buttons)
            {
                std::vector<std::string> cols = { "\"TemplateEntry\"", "\"ButtonType\"", "\"Title\"", "\"Payload\"", "\"SortOrder\"", "\"CreatedBy\"" };
                std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateButtons\"", cols);
                std::vector<omnisphere::types::SQLParam> params = {
                    omnisphere::types::MakeSQLParam(templateEntry),
                    omnisphere::types::MakeSQLParam(btn.buttonType),
                    omnisphere::types::MakeSQLParam(btn.title),
                    omnisphere::types::MakeSQLParam(btn.payload.value_or("")),
                    omnisphere::types::MakeSQLParam(btn.sortOrder > 0 ? btn.sortOrder : btnIdx++),
                    omnisphere::types::MakeSQLParam(!btn.createdBy.empty() ? btn.createdBy : std::string("system"))
                };
                conn->RunPrepared(sql, params);
            }

            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[WhatsAppRepository::SaveWhatsAppTemplate Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool WhatsAppRepository::UpdateWhatsAppTemplate(
        const omnisphere::dtos::UpdateWhatsAppTemplateInput& input,
        const std::vector<std::string>& mutationFields
    ) const
    {
        if (!m_dbPool || input.Code.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();

            // 1. Extraer columnas a actualizar en la tabla principal WhatsAppTemplates
            std::vector<omnisphere::types::ColumnValue> updateCols =
                omnisphere::types::ExtractUpdateColumns(input, mutationFields);

            bool hasLastUpdatedBy = false;
            bool hasUpdateDate = false;
            for (const auto& c : updateCols)
            {
                if (c.Column == "\"LastUpdatedBy\"") hasLastUpdatedBy = true;
                if (c.Column == "\"UpdateDate\"") hasUpdateDate = true;
            }

            if (!hasLastUpdatedBy)
            {
                updateCols.push_back({"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(input.LastUpdatedBy.empty() ? "system" : input.LastUpdatedBy)});
            }
            if (!hasUpdateDate)
            {
                auto now = std::chrono::system_clock::now();
                auto in_time_t = std::chrono::system_clock::to_time_t(now);
                char buf[32];
                std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::gmtime(&in_time_t));
                updateCols.push_back({"\"UpdateDate\"", omnisphere::types::MakeSQLParam(input.UpdateDate.value_or(std::string(buf)))});
            }

            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"WhatsAppTemplates\"",
                updateCols,
                "\"Code\"",
                omnisphere::types::MakeSQLParam(input.Code)
            );

            // Ejecutar actualización principal devolviendo Entry
            auto updateDt = conn->FetchPrepared(
                updateResult.Query + " RETURNING \"Entry\"",
                updateResult.Parameters
            );

            if (updateDt.IsEmpty())
            {
                conn->RollbackTransaction();
                return false;
            }

            int templateEntry = updateDt[0]["Entry"].GetOptional<int>().value_or(0);

            // 2. Header y variables
            if (input.HeaderType.has_value() || input.HeaderText.has_value() || input.HeaderVariables.has_value())
            {
                conn->RunPrepared("DELETE FROM \"WhatsAppTemplateHeaders\" WHERE \"TemplateEntry\" = ?", { omnisphere::types::MakeSQLParam(templateEntry) });

                std::string hType = input.HeaderType.value_or("NONE");
                if (hType != "NONE")
                {
                    std::vector<std::string> cols = { "\"TemplateEntry\"", "\"HeaderType\"", "\"TextContent\"", "\"CreatedBy\"" };
                    std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateHeaders\"", cols);
                    std::vector<omnisphere::types::SQLParam> params = {
                        omnisphere::types::MakeSQLParam(templateEntry),
                        omnisphere::types::MakeSQLParam(hType),
                        omnisphere::types::MakeSQLParam(input.HeaderText.value_or("")),
                        omnisphere::types::MakeSQLParam(input.LastUpdatedBy.empty() ? "system" : input.LastUpdatedBy)
                    };
                    auto hDt = conn->FetchPrepared(sql + " RETURNING \"Entry\"", params);

                    if (!hDt.IsEmpty() && input.HeaderVariables.has_value())
                    {
                        int hEntry = hDt[0]["Entry"].GetOptional<int>().value_or(0);
                        int hvIdx = 1;
                        for (const auto& hv : *input.HeaderVariables)
                        {
                            std::vector<std::string> vCols = { "\"HeaderEntry\"", "\"VarName\"", "\"VarIndex\"", "\"ExampleValue\"", "\"CreatedBy\"" };
                            std::string vSql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateHeaderVariables\"", vCols);
                            std::vector<omnisphere::types::SQLParam> vParams = {
                                omnisphere::types::MakeSQLParam(hEntry),
                                omnisphere::types::MakeSQLParam(hv.key),
                                omnisphere::types::MakeSQLParam(hvIdx++),
                                omnisphere::types::MakeSQLParam(hv.example),
                                omnisphere::types::MakeSQLParam(input.LastUpdatedBy.empty() ? "system" : input.LastUpdatedBy)
                            };
                            conn->RunPrepared(vSql, vParams);
                        }
                    }
                }
            }

            // 3. Body y variables
            if (input.BodyText.has_value() || input.BodyVariables.has_value())
            {
                conn->RunPrepared("DELETE FROM \"WhatsAppTemplateBodies\" WHERE \"TemplateEntry\" = ?", { omnisphere::types::MakeSQLParam(templateEntry) });

                if (input.BodyText.has_value() && !input.BodyText->empty())
                {
                    std::vector<std::string> cols = { "\"TemplateEntry\"", "\"BodyText\"", "\"CreatedBy\"" };
                    std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateBodies\"", cols);
                    std::vector<omnisphere::types::SQLParam> params = {
                        omnisphere::types::MakeSQLParam(templateEntry),
                        omnisphere::types::MakeSQLParam(*input.BodyText),
                        omnisphere::types::MakeSQLParam(input.LastUpdatedBy.empty() ? "system" : input.LastUpdatedBy)
                    };
                    auto bDt = conn->FetchPrepared(sql + " RETURNING \"Entry\"", params);

                    if (!bDt.IsEmpty() && input.BodyVariables.has_value())
                    {
                        int bEntry = bDt[0]["Entry"].GetOptional<int>().value_or(0);
                        int bvIdx = 1;
                        for (const auto& bv : *input.BodyVariables)
                        {
                            std::vector<std::string> vCols = { "\"BodyEntry\"", "\"VarName\"", "\"VarIndex\"", "\"ExampleValue\"", "\"CreatedBy\"" };
                            std::string vSql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateBodyVariables\"", vCols);
                            std::vector<omnisphere::types::SQLParam> vParams = {
                                omnisphere::types::MakeSQLParam(bEntry),
                                omnisphere::types::MakeSQLParam(bv.key),
                                omnisphere::types::MakeSQLParam(bvIdx++),
                                omnisphere::types::MakeSQLParam(bv.example),
                                omnisphere::types::MakeSQLParam(input.LastUpdatedBy.empty() ? "system" : input.LastUpdatedBy)
                            };
                            conn->RunPrepared(vSql, vParams);
                        }
                    }
                }
            }

            // 4. Footer y variables
            if (input.FooterText.has_value() || input.FooterVariables.has_value())
            {
                conn->RunPrepared("DELETE FROM \"WhatsAppTemplateFooters\" WHERE \"TemplateEntry\" = ?", { omnisphere::types::MakeSQLParam(templateEntry) });

                if (input.FooterText.has_value() && !input.FooterText->empty())
                {
                    std::vector<std::string> cols = { "\"TemplateEntry\"", "\"FooterText\"", "\"CreatedBy\"" };
                    std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateFooters\"", cols);
                    std::vector<omnisphere::types::SQLParam> params = {
                        omnisphere::types::MakeSQLParam(templateEntry),
                        omnisphere::types::MakeSQLParam(*input.FooterText),
                        omnisphere::types::MakeSQLParam(input.LastUpdatedBy.empty() ? "system" : input.LastUpdatedBy)
                    };
                    auto fDt = conn->FetchPrepared(sql + " RETURNING \"Entry\"", params);

                    if (!fDt.IsEmpty() && input.FooterVariables.has_value())
                    {
                        int fEntry = fDt[0]["Entry"].GetOptional<int>().value_or(0);
                        int fvIdx = 1;
                        for (const auto& fv : *input.FooterVariables)
                        {
                            std::vector<std::string> vCols = { "\"FooterEntry\"", "\"VarName\"", "\"VarIndex\"", "\"ExampleValue\"", "\"CreatedBy\"" };
                            std::string vSql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateFooterVariables\"", vCols);
                            std::vector<omnisphere::types::SQLParam> vParams = {
                                omnisphere::types::MakeSQLParam(fEntry),
                                omnisphere::types::MakeSQLParam(fv.key),
                                omnisphere::types::MakeSQLParam(fvIdx++),
                                omnisphere::types::MakeSQLParam(fv.example),
                                omnisphere::types::MakeSQLParam(input.LastUpdatedBy.empty() ? "system" : input.LastUpdatedBy)
                            };
                            conn->RunPrepared(vSql, vParams);
                        }
                    }
                }
            }

            // 5. Botones
            if (input.Buttons.has_value())
            {
                conn->RunPrepared("DELETE FROM \"WhatsAppTemplateButtons\" WHERE \"TemplateEntry\" = ?", { omnisphere::types::MakeSQLParam(templateEntry) });

                int btnIdx = 1;
                for (const auto& btn : *input.Buttons)
                {
                    std::vector<std::string> cols = { "\"TemplateEntry\"", "\"ButtonType\"", "\"Title\"", "\"Payload\"", "\"SortOrder\"", "\"CreatedBy\"" };
                    std::string sql = omnisphere::types::BuildInsertQuery("\"WhatsAppTemplateButtons\"", cols);
                    std::vector<omnisphere::types::SQLParam> params = {
                        omnisphere::types::MakeSQLParam(templateEntry),
                        omnisphere::types::MakeSQLParam(btn.type),
                        omnisphere::types::MakeSQLParam(btn.text),
                        omnisphere::types::MakeSQLParam(btn.url.has_value() ? *btn.url : (btn.phoneNumber.has_value() ? *btn.phoneNumber : "")),
                        omnisphere::types::MakeSQLParam(btnIdx++),
                        omnisphere::types::MakeSQLParam(input.LastUpdatedBy.empty() ? "system" : input.LastUpdatedBy)
                    };
                    conn->RunPrepared(sql, params);
                }
            }

            conn->CommitTransaction();
            return true;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[WhatsAppRepository::UpdateWhatsAppTemplate Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool WhatsAppRepository::DeleteWhatsAppTemplate(const std::string& code) const
    {
        if (!m_dbPool || code.empty()) return false;
        auto conn = m_dbPool->Acquire();
        try
        {
            conn->BeginTransaction();

            // Eliminar de WhatsAppTemplates (en cascada elimina Headers, Bodies, Footers, Buttons y Variables)
            bool ok = conn->RunPrepared("DELETE FROM \"WhatsAppTemplates\" WHERE \"Code\" = ? OR LOWER(\"Code\") = LOWER(?)",
                { omnisphere::types::MakeSQLParam(code), omnisphere::types::MakeSQLParam(code) });

            if (ok)
            {
                conn->CommitTransaction();
                return true;
            }
            conn->RollbackTransaction();
            return false;
        }
        catch (const std::exception& ex)
        {
            conn->RollbackTransaction();
            std::cerr << "[WhatsAppRepository::DeleteWhatsAppTemplate Exception] " << ex.what() << std::endl;
            return false;
        }
    }
} // namespace omnisphere::repositories
