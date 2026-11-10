#include "Notification/Repositories/SystemNotificationRepository.hpp"
#include <OmniData/DataMapper.hpp>
#include <OmniData/QueryBuilder.hpp>
#include <iostream>
#include <chrono>

namespace omnisphere::repositories
{
    SystemNotificationRepository::SystemNotificationRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_dbPool(std::move(dbPool))
    {
    }

    std::vector<omnisphere::models::NotificationTypeConfig> SystemNotificationRepository::GetTypeConfigs() const
    {
        if (!m_dbPool) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = R"(
                SELECT "Entry", "Code", "Name", "Description", "ModuleCode", "RequiredPermission", 
                       "IsEnabled", "RolesAllowed", "CreatedBy", "CreateDate", "LastUpdatedBy", "UpdateDate"
                FROM "NotificationTypeConfigs"
                ORDER BY "Entry" ASC
            )";
            auto dt = conn->Run(sql);
            return omnisphere::types::DataTableToModels<omnisphere::models::NotificationTypeConfig>(dt);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemNotificationRepository::GetTypeConfigs Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    std::optional<omnisphere::models::NotificationTypeConfig> SystemNotificationRepository::GetTypeConfigByCode(const std::string& code) const
    {
        if (!m_dbPool || code.empty()) return std::nullopt;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = R"(
                SELECT "Entry", "Code", "Name", "Description", "ModuleCode", "RequiredPermission", 
                       "IsEnabled", "RolesAllowed", "CreatedBy", "CreateDate", "LastUpdatedBy", "UpdateDate"
                FROM "NotificationTypeConfigs"
                WHERE "Code" = ?
                LIMIT 1
            )";
            auto dt = conn->RunPrepared(sql, { omnisphere::types::MakeSQLParam(code) });
            if (!dt.IsEmpty())
            {
                return omnisphere::types::FromDataRow<omnisphere::models::NotificationTypeConfig>(dt[0]);
            }
            return std::nullopt;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemNotificationRepository::GetTypeConfigByCode Exception] " << ex.what() << std::endl;
            return std::nullopt;
        }
    }

    bool SystemNotificationRepository::UpdateTypeConfig(
        const omnisphere::dtos::UpdateNotificationTypeConfigInput& input, 
        const std::vector<std::string>& mutationFields) const
    {
        if (!m_dbPool || input.code.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::vector<omnisphere::types::ColumnValue> updateCols;

            if (input.name.has_value()) updateCols.push_back({"\"Name\"", omnisphere::types::MakeSQLParam(*input.name)});
            if (input.description.has_value()) updateCols.push_back({"\"Description\"", omnisphere::types::MakeSQLParam(*input.description)});
            if (input.requiredPermission.has_value()) updateCols.push_back({"\"RequiredPermission\"", omnisphere::types::MakeSQLParam(*input.requiredPermission)});
            if (input.isEnabled.has_value()) updateCols.push_back({"\"IsEnabled\"", omnisphere::types::MakeSQLParam(*input.isEnabled)});
            if (input.rolesAllowed.has_value()) updateCols.push_back({"\"RolesAllowed\"", omnisphere::types::MakeSQLParam(*input.rolesAllowed)});
            if (input.lastUpdatedBy.has_value()) updateCols.push_back({"\"LastUpdatedBy\"", omnisphere::types::MakeSQLParam(*input.lastUpdatedBy)});

            if (updateCols.empty()) return true;

            auto updateResult = omnisphere::types::BuildUpdateQuery(
                "\"NotificationTypeConfigs\"", updateCols, "\"Code\"", omnisphere::types::MakeSQLParam(input.code)
            );

            return conn->RunPrepared(updateResult.Query, updateResult.Parameters);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemNotificationRepository::UpdateTypeConfig Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    std::string SystemNotificationRepository::CreateNotification(
        const std::string& typeCode,
        const std::string& title,
        const std::string& message,
        const std::optional<std::string>& referenceId,
        const std::optional<std::string>& metadataJson,
        const std::string& createdBy) const
    {
        if (!m_dbPool || typeCode.empty()) return "";
        try
        {
            auto conn = m_dbPool->Acquire();
            // Generar código autoincrementable o timestamp hash
            std::string code = "NTF_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

            std::string sql = R"(
                INSERT INTO "Notifications" ("Code", "TypeCode", "Title", "Message", "ReferenceId", "MetadataJson", "CreatedBy")
                VALUES (?, ?, ?, ?, ?, ?, ?)
            )";

            bool ok = conn->RunPrepared(sql, {
                omnisphere::types::MakeSQLParam(code),
                omnisphere::types::MakeSQLParam(typeCode),
                omnisphere::types::MakeSQLParam(title),
                omnisphere::types::MakeSQLParam(message),
                omnisphere::types::MakeSQLParam(referenceId.value_or("")),
                omnisphere::types::MakeSQLParam(metadataJson.value_or("{}")),
                omnisphere::types::MakeSQLParam(createdBy.empty() ? "SYSTEM" : createdBy)
            });

            return ok ? code : "";
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemNotificationRepository::CreateNotification Exception] " << ex.what() << std::endl;
            return "";
        }
    }

    bool SystemNotificationRepository::AddRecipients(const std::string& notificationCode, const std::vector<std::string>& userCodes) const
    {
        if (!m_dbPool || notificationCode.empty() || userCodes.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            for (const auto& uCode : userCodes)
            {
                if (uCode.empty()) continue;
                std::string sql = R"(
                    INSERT INTO "NotificationRecipients" ("NotificationCode", "UserCode", "IsRead")
                    VALUES (?, ?, false)
                    ON CONFLICT ("NotificationCode", "UserCode") DO NOTHING
                )";
                conn->RunPrepared(sql, {
                    omnisphere::types::MakeSQLParam(notificationCode),
                    omnisphere::types::MakeSQLParam(uCode)
                });
            }
            return true;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemNotificationRepository::AddRecipients Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    std::vector<std::string> SystemNotificationRepository::GetUsersWithPermission(const std::string& permissionCode) const
    {
        if (!m_dbPool || permissionCode.empty()) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            // Obtener usuarios con rol ADMIN, o usuarios que tengan el permiso explícito en su rol
            std::string sql = R"(
                SELECT DISTINCT u."Code"
                FROM "Users" u
                JOIN "UserRoles" ur ON u."Code" = ur."UserCode" AND ur."IsActive" = true
                LEFT JOIN "RolePermissions" rp ON ur."RoleCode" = rp."RoleCode" AND rp."IsActive" = true
                WHERE u."IsActive" = true AND (ur."RoleCode" = 'ADMIN' OR rp."PermissionCode" = ?)
            )";

            auto dt = conn->RunPrepared(sql, { omnisphere::types::MakeSQLParam(permissionCode) });
            std::vector<std::string> users;
            for (const auto& row : dt)
            {
                auto codeOpt = row["Code"].GetOptional<std::string>();
                if (codeOpt.has_value() && !codeOpt->empty())
                {
                    users.push_back(*codeOpt);
                }
            }
            return users;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemNotificationRepository::GetUsersWithPermission Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    std::vector<omnisphere::models::AppNotification> SystemNotificationRepository::GetUserNotifications(
        const std::string& userCode, bool unreadOnly, int limit) const
    {
        if (!m_dbPool || userCode.empty()) return {};
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = R"(
                SELECT n."Entry", n."Code", n."TypeCode", n."Title", n."Message", 
                       n."ReferenceId", n."MetadataJson", n."CreatedBy", n."CreateDate",
                       nr."IsRead", nr."ReadDate"
                FROM "Notifications" n
                JOIN "NotificationRecipients" nr ON n."Code" = nr."NotificationCode"
                WHERE nr."UserCode" = ?
            )";

            if (unreadOnly)
            {
                sql += " AND nr.\"IsRead\" = false ";
            }

            sql += " ORDER BY n.\"CreateDate\" DESC LIMIT ? ";

            auto dt = conn->RunPrepared(sql, {
                omnisphere::types::MakeSQLParam(userCode),
                omnisphere::types::MakeSQLParam(limit <= 0 ? 50 : limit)
            });

            return omnisphere::types::DataTableToModels<omnisphere::models::AppNotification>(dt);
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemNotificationRepository::GetUserNotifications Exception] " << ex.what() << std::endl;
            return {};
        }
    }

    int SystemNotificationRepository::GetUserUnreadCount(const std::string& userCode) const
    {
        if (!m_dbPool || userCode.empty()) return 0;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = R"(
                SELECT COUNT(*) as "Count"
                FROM "NotificationRecipients"
                WHERE "UserCode" = ? AND "IsRead" = false
            )";

            auto dt = conn->RunPrepared(sql, { omnisphere::types::MakeSQLParam(userCode) });
            if (!dt.IsEmpty())
            {
                return dt[0]["Count"].GetOptional<int>().value_or(0);
            }
            return 0;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemNotificationRepository::GetUserUnreadCount Exception] " << ex.what() << std::endl;
            return 0;
        }
    }

    bool SystemNotificationRepository::MarkAsRead(const std::string& notificationCode, const std::string& userCode) const
    {
        if (!m_dbPool || notificationCode.empty() || userCode.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = R"(
                UPDATE "NotificationRecipients"
                SET "IsRead" = true, "ReadDate" = CURRENT_TIMESTAMP
                WHERE "NotificationCode" = ? AND "UserCode" = ?
            )";

            return conn->RunPrepared(sql, {
                omnisphere::types::MakeSQLParam(notificationCode),
                omnisphere::types::MakeSQLParam(userCode)
            });
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemNotificationRepository::MarkAsRead Exception] " << ex.what() << std::endl;
            return false;
        }
    }

    bool SystemNotificationRepository::MarkAllAsRead(const std::string& userCode) const
    {
        if (!m_dbPool || userCode.empty()) return false;
        try
        {
            auto conn = m_dbPool->Acquire();
            std::string sql = R"(
                UPDATE "NotificationRecipients"
                SET "IsRead" = true, "ReadDate" = CURRENT_TIMESTAMP
                WHERE "UserCode" = ? AND "IsRead" = false
            )";

            return conn->RunPrepared(sql, { omnisphere::types::MakeSQLParam(userCode) });
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[SystemNotificationRepository::MarkAllAsRead Exception] " << ex.what() << std::endl;
            return false;
        }
    }
} // namespace omnisphere::repositories

