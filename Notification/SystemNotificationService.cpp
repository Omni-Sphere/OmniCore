#include "Notification/SystemNotificationService.hpp"
#include <iostream>

namespace omnisphere::services
{
    SystemNotificationService::SystemNotificationService(std::shared_ptr<omnisphere::repositories::SystemNotificationRepository> repo)
        : m_repo(std::move(repo))
    {
    }

    SystemNotificationService::SystemNotificationService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_repo(std::make_shared<omnisphere::repositories::SystemNotificationRepository>(std::move(dbPool)))
    {
    }

    void SystemNotificationService::SetDispatchCallback(std::function<void(const omnisphere::models::AppNotification&)> cb)
    {
        m_onNotificationDispatched = std::move(cb);
    }

    bool SystemNotificationService::EmitEvent(
        const std::string& typeCode,
        const std::string& title,
        const std::string& message,
        const std::optional<std::string>& referenceId,
        const nlohmann::json& metadata,
        const std::string& createdBy)
    {
        if (!m_repo) return false;

        // 1. Validar si el tipo está configurado y habilitado
        auto configOpt = m_repo->GetTypeConfigByCode(typeCode);
        if (!configOpt.has_value())
        {
            std::cerr << "[SystemNotificationService] Tipo de notificación desconocido: " << typeCode << std::endl;
            return false;
        }

        if (!configOpt->isEnabled)
        {
            // Tipo deshabilitado administrativamente
            return true;
        }

        // 2. Crear el registro maestro de la notificación en BD
        std::string metaStr = metadata.is_null() ? "{}" : metadata.dump();
        std::string notifCode = m_repo->CreateNotification(typeCode, title, message, referenceId, metaStr, createdBy);
        if (notifCode.empty())
        {
            return false;
        }

        // 3. Obtener usuarios autorizados según el permiso configurado
        auto recipients = m_repo->GetUsersWithPermission(configOpt->requiredPermission);
        if (!recipients.empty())
        {
            m_repo->AddRecipients(notifCode, recipients);
        }

        // 4. Notificar a los clientes online conectados (vía SSE / deliver)
        if (m_onNotificationDispatched)
        {
            omnisphere::models::AppNotification item;
            item.code = notifCode;
            item.typeCode = typeCode;
            item.title = title;
            item.message = message;
            item.referenceId = referenceId;
            item.metadataJson = metaStr;
            item.createdBy = createdBy;
            item.isRead = false;

            try
            {
                m_onNotificationDispatched(item);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "[SystemNotificationService] Error al despachar SSE: " << ex.what() << std::endl;
            }
        }

        return true;
    }

    std::vector<omnisphere::models::NotificationTypeConfig> SystemNotificationService::GetTypeConfigs(
        const omnisphere::models::SecurityContext& ctx) const
    {
        AUTHORIZE(ctx, "MOD_INTEGRATIONS", "CORE_NOTIF_CONFIG");
        return m_repo ? m_repo->GetTypeConfigs() : std::vector<omnisphere::models::NotificationTypeConfig>{};
    }

    bool SystemNotificationService::UpdateTypeConfig(
        const omnisphere::models::SecurityContext& ctx,
        const omnisphere::dtos::UpdateNotificationTypeConfigInput& input,
        const std::vector<std::string>& mutationFields) const
    {
        AUTHORIZE(ctx, "MOD_INTEGRATIONS", "CORE_NOTIF_CONFIG");
        return m_repo ? m_repo->UpdateTypeConfig(input, mutationFields) : false;
    }

    std::vector<omnisphere::models::AppNotification> SystemNotificationService::GetMyNotifications(
        const omnisphere::models::SecurityContext& ctx,
        bool unreadOnly,
        int limit) const
    {
        if (!ctx.isAuthenticated() || ctx.userCode.empty() || !m_repo)
        {
            return {};
        }
        return m_repo->GetUserNotifications(ctx.userCode, unreadOnly, limit);
    }

    int SystemNotificationService::GetMyUnreadCount(const omnisphere::models::SecurityContext& ctx) const
    {
        if (!ctx.isAuthenticated() || ctx.userCode.empty() || !m_repo)
        {
            return 0;
        }
        return m_repo->GetUserUnreadCount(ctx.userCode);
    }

    bool SystemNotificationService::MarkAsRead(const omnisphere::models::SecurityContext& ctx, const std::string& notificationCode) const
    {
        if (!ctx.isAuthenticated() || ctx.userCode.empty() || !m_repo)
        {
            return false;
        }
        return m_repo->MarkAsRead(notificationCode, ctx.userCode);
    }

    bool SystemNotificationService::MarkAllAsRead(const omnisphere::models::SecurityContext& ctx) const
    {
        if (!ctx.isAuthenticated() || ctx.userCode.empty() || !m_repo)
        {
            return false;
        }
        return m_repo->MarkAllAsRead(ctx.userCode);
    }
} // namespace omnisphere::services

