#pragma once
#include "Notification/DTOs/WhatsAppConfig.hpp"
#include "Notification/Repositories/WhatsAppRepository.hpp"
#include "Notification/Models/WhatsAppSettings.hpp"
#include <OmniData/DatabasePool.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace omnisphere::services
{
    class WhatsAppService
    {
    public:
        WhatsAppService() = default;
        explicit WhatsAppService(const omnisphere::dtos::WhatsAppConfig& config);
        explicit WhatsAppService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);

        bool InitializeFromDatabase(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        void SetConfig(const omnisphere::dtos::WhatsAppConfig& config);
        omnisphere::dtos::WhatsAppConfig GetConfig() const;

        omnisphere::models::WhatsAppSettings GetSettings(const std::vector<std::string>& requestedFields = {}) const;
        bool SaveSettings(const omnisphere::models::WhatsAppSettings& settings);

        bool SendNotification(
            const std::string& phoneNumber,
            const std::string& templateName,
            const std::vector<std::string>& params
        );

        bool SendNamedNotification(
            const std::string& phoneNumber,
            const std::string& templateName,
            const std::map<std::string, std::string>& params
        );

        bool SendMessage(
            const std::string& phoneNumber,
            const std::string& message
        );

        bool SendTicketConfirmation(
            const std::string& phoneNumber,
            const std::string& customerName,
            const std::string& ticketNumber,
            const std::string& tripType,
            const std::string& eventName,
            const std::string& eventDate,
            const std::string& eventTime,
            const std::string& departureName,
            const std::string& references,
            const std::string& toleranceTime
        );

    private:
        omnisphere::dtos::WhatsAppConfig m_config;
        std::shared_ptr<omnisphere::repositories::WhatsAppRepository> m_repository;

        bool SendRequest(
            const std::string& phoneNumber,
            const std::string& messageType,
            const std::string& templateName,
            const std::string& content,
            const std::string& jsonString
        );
    };
}
