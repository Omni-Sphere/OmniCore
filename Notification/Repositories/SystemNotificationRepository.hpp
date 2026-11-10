#pragma once
#include "Notification/Models/NotificationTypeConfig.hpp"
#include "Notification/Models/AppNotification.hpp"
#include "Notification/DTOs/UpdateNotificationTypeConfigInput.hpp"
#include <OmniData/DatabasePool.hpp>
#include <OmniData/DataTable.hpp>
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace omnisphere::repositories
{
    class SystemNotificationRepository
    {
    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;

    public:
        explicit SystemNotificationRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);

        // Catálogo de tipos de notificación
        std::vector<omnisphere::models::NotificationTypeConfig> GetTypeConfigs() const;
        std::optional<omnisphere::models::NotificationTypeConfig> GetTypeConfigByCode(const std::string& code) const;
        bool UpdateTypeConfig(const omnisphere::dtos::UpdateNotificationTypeConfigInput& input, const std::vector<std::string>& mutationFields) const;

        // Historial y persistencia de notificaciones
        std::string CreateNotification(
            const std::string& typeCode,
            const std::string& title,
            const std::string& message,
            const std::optional<std::string>& referenceId = std::nullopt,
            const std::optional<std::string>& metadataJson = std::nullopt,
            const std::string& createdBy = "SYSTEM"
        ) const;

        // Asignación de destinatarios (Notificaciones pendientes)
        bool AddRecipients(const std::string& notificationCode, const std::vector<std::string>& userCodes) const;
        std::vector<std::string> GetUsersWithPermission(const std::string& permissionCode) const;

        // Consultas por usuario
        std::vector<omnisphere::models::AppNotification> GetUserNotifications(const std::string& userCode, bool unreadOnly = false, int limit = 50) const;
        int GetUserUnreadCount(const std::string& userCode) const;
        bool MarkAsRead(const std::string& notificationCode, const std::string& userCode) const;
        bool MarkAllAsRead(const std::string& userCode) const;
    };
} // namespace omnisphere::repositories

