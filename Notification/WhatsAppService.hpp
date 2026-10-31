#pragma once
#include "Notification/DTOs/WhatsAppConfig.hpp"
#include "Notification/DTOs/CreateMetaTemplateInput.hpp"
#include "Notification/DTOs/UpdateWhatsAppTemplateInput.hpp"
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
    struct WhatsAppMediaHeader
    {
        std::string type;     // "IMAGE", "VIDEO", "DOCUMENT", "LOCATION"
        std::string mediaUrl; // URL segura generada internamente
        std::string filename; // Nombre del archivo (ej. "Boleto_RSV123.pdf")
        double latitude = 0.0;
        double longitude = 0.0;
        std::string locationName;
        std::string locationAddress;
    };

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

        bool SendNotification(
            const std::string& phoneNumber,
            const std::string& templateName,
            const std::vector<std::string>& params,
            const std::optional<WhatsAppMediaHeader>& mediaHeader
        );

        bool SendNamedNotification(
            const std::string& phoneNumber,
            const std::string& templateName,
            const std::map<std::string, std::string>& params,
            const std::string& interpolatedContent = "",
            const std::optional<WhatsAppMediaHeader>& mediaHeader = std::nullopt
        );

        bool SendMetaTemplate(
            const std::string& phoneNumber,
            const std::string& templateCodeOrName,
            const std::map<std::string, std::string>& availableValues,
            const std::optional<WhatsAppMediaHeader>& mediaHeader = std::nullopt
        );

        bool SendMetaTemplate(
            const std::string& phoneNumber,
            const omnisphere::models::WhatsAppTemplate& templateModel,
            const std::map<std::string, std::string>& availableValues,
            const std::optional<WhatsAppMediaHeader>& mediaHeader = std::nullopt
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

        bool SetConversationContext(const std::string& phoneNumber, const std::string& reservationCode);
        std::optional<std::string> GetConversationContext(const std::string& phoneNumber) const;
        bool ClearConversationContext(const std::string& phoneNumber);

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

        bool UpdateWhatsAppTemplate(
            const omnisphere::dtos::UpdateWhatsAppTemplateInput& input,
            const std::vector<std::string>& mutationFields = {}
        );

        bool DeleteMetaTemplate(const std::string& templateCode);

        bool SyncTemplatesFromMeta();

        std::string DownloadMedia(
            const std::string& mediaId,
            const std::string& mimeType = "",
            const std::string& suggestedFilename = ""
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
        void CheckLicense() const;

        bool SendRequest(
            const std::string& phoneNumber,
            const std::string& messageType,
            const std::string& templateName,
            const std::string& content,
            const std::string& jsonString
        );
    };
}
