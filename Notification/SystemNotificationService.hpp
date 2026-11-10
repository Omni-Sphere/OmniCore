#pragma once
#include "Notification/Repositories/SystemNotificationRepository.hpp"
#include <OmniCore/Authorization/Models/SecurityContext.hpp>
#include <OmniCore/Authorization/Authorization.hpp>
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace omnisphere::services
{
    class SystemNotificationService
    {
    private:
        std::shared_ptr<omnisphere::repositories::SystemNotificationRepository> m_repo;
        std::shared_ptr<omnisphere::services::Authorization> m_authService;
        // Callback para disparar el deliver de GraphQL / SSE
        std::function<void(const omnisphere::models::AppNotification&)> m_onNotificationDispatched;

    public:
        explicit SystemNotificationService(
            std::shared_ptr<omnisphere::repositories::SystemNotificationRepository> repo,
            std::shared_ptr<omnisphere::services::Authorization> authService = nullptr
        );
        explicit SystemNotificationService(
            std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
            std::shared_ptr<omnisphere::services::Authorization> authService = nullptr
        );

        void SetDispatchCallback(std::function<void(const omnisphere::models::AppNotification&)> cb);

        // Disparo de eventos desde cualquier parte del sistema (ej: CreateReservation)
        bool EmitEvent(
            const std::string& typeCode,
            const std::string& title,
            const std::string& message,
            const std::optional<std::string>& referenceId = std::nullopt,
            const nlohmann::json& metadata = nlohmann::json::object(),
            const std::string& createdBy = "SYSTEM"
        );

        // Catálogo y configuración
        std::vector<omnisphere::models::NotificationTypeConfig> GetTypeConfigs(const omnisphere::models::SecurityContext& ctx) const;
        bool UpdateTypeConfig(
            const omnisphere::models::SecurityContext& ctx,
            const omnisphere::dtos::UpdateNotificationTypeConfigInput& input,
            const std::vector<std::string>& mutationFields
        ) const;

        // Consultas por usuario
        std::vector<omnisphere::models::AppNotification> GetMyNotifications(
            const omnisphere::models::SecurityContext& ctx,
            bool unreadOnly = false,
            int limit = 50
        ) const;

        int GetMyUnreadCount(const omnisphere::models::SecurityContext& ctx) const;
        bool MarkAsRead(const omnisphere::models::SecurityContext& ctx, const std::string& notificationCode) const;
        bool MarkAllAsRead(const omnisphere::models::SecurityContext& ctx) const;
    };
} // namespace omnisphere::services

