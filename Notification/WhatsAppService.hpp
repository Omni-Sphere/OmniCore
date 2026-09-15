#pragma once
#include "Notification/DTOs/WhatsAppConfig.hpp"
#include "Notification/DTOs/CreateMetaTemplateInput.hpp"
#include "Notification/Repositories/WhatsAppRepository.hpp"
#include "Notification/Models/WhatsAppSettings.hpp"
#include "Notification/Models/CustomMessage.hpp"
#include "License/Services/LicenseService.hpp"
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

        bool MarkAsRead(const std::string& wamid) const;

        bool SendInteractiveButtons(
            const std::string& phoneNumber,
            const std::string& bodyText,
            const std::vector<std::pair<std::string, std::string>>& buttons
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

        // Obtiene los datos necesarios desde la DB (JOIN único) y envía la plantilla ticket_confirmation.
        // Debe llamarse desde un thread separado ya que hace I/O de red.
        bool SendTicketConfirmationByReservation(
            const std::string& reservationCode,
            std::shared_ptr<omnisphere::data::DatabasePool> dbPool
        );

        bool SendCustomMessage(
            const std::string& phoneNumber,
            const std::string& messageCode,
            const std::map<std::string, std::string>& placeholders = {}
        );

        std::optional<omnisphere::models::CustomMessage> GetCustomMessage(
            const std::string& messageCode
        ) const;

        omnisphere::dtos::CreateMetaTemplateResult CreateMetaTemplate(
            const omnisphere::dtos::CreateMetaTemplateInput& input
        );

        bool HasRecentWelcomeCard(const std::string& phoneNumber, int minutesWindow = 30) const;

        void SetLicenseService(std::shared_ptr<omnisphere::services::LicenseService> licenseService);
        std::string GetLastErrorMessage() const { return m_lastError; }

    private:
        omnisphere::dtos::WhatsAppConfig m_config;
        std::shared_ptr<omnisphere::repositories::WhatsAppRepository> m_repository;
        std::shared_ptr<omnisphere::services::LicenseService> m_licenseService;
        mutable std::string m_lastError;

        static std::string ParseMetaErrorMessage(const std::string& rawPayload);

        bool SendRequest(
            const std::string& phoneNumber,
            const std::string& messageType,
            const std::string& templateName,
            const std::string& content,
            const std::string& jsonString
        );
    };
}
