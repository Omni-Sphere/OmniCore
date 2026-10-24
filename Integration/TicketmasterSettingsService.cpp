#include "Integration/TicketmasterSettingsService.hpp"
#include <OmniData/DataMapper.hpp>
#include <OmniUtils/Base64.hpp>
#include <iostream>

namespace omnisphere::services
{
    TicketmasterSettingsService::TicketmasterSettingsService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_repository(std::make_shared<omnisphere::repositories::TicketmasterRepository>(std::move(dbPool))) {}

    TicketmasterSettingsService::TicketmasterSettingsService(std::shared_ptr<omnisphere::repositories::TicketmasterRepository> repository)
        : m_repository(std::move(repository)) {}

    std::optional<omnisphere::models::TicketmasterSettings> TicketmasterSettingsService::GetSettings(bool decryptKeys) const
    {
        if (!m_repository) return std::nullopt;
        auto dt = m_repository->GetSettings();
        if (dt.IsEmpty())
        {
            omnisphere::models::TicketmasterSettings defaultSettings;
            return defaultSettings;
        }

        auto s = omnisphere::types::FromDataRow<omnisphere::models::TicketmasterSettings>(dt[0]);
        if (decryptKeys)
        {
            if (!s.apiKey.empty())
            {
                try { s.apiKey = omnisphere::utils::Base64::Decode(s.apiKey); } catch (...) {}
            }
            if (!s.apiSecret.empty())
            {
                try { s.apiSecret = omnisphere::utils::Base64::Decode(s.apiSecret); } catch (...) {}
            }
        }
        else
        {
            // Enmascaramiento para la vista de administración
            if (!s.apiKey.empty())
            {
                std::string decodedKey = s.apiKey;
                try { decodedKey = omnisphere::utils::Base64::Decode(s.apiKey); } catch (...) {}
                if (decodedKey.length() > 8)
                {
                    s.apiKey = decodedKey.substr(0, 4) + "••••••••" + decodedKey.substr(decodedKey.length() - 4);
                }
            }
            if (!s.apiSecret.empty())
            {
                s.apiSecret = "••••••••••••••••";
            }
        }
        return s;
    }

    bool TicketmasterSettingsService::SaveSettings(const omnisphere::models::TicketmasterSettings& settings, const std::vector<std::string>& mutationFields) const
    {
        if (!m_repository) return false;

        omnisphere::models::TicketmasterSettings toSave = settings;
        auto existingDt = m_repository->GetSettings({"Entry", "Code", "ApiKey", "ApiSecret"});
        omnisphere::models::TicketmasterSettings existingSettings{};
        if (!existingDt.IsEmpty())
        {
            existingSettings = omnisphere::types::FromDataRow<omnisphere::models::TicketmasterSettings>(existingDt[0]);
        }

        auto encodeIfNeeded = [](const std::string& input, const std::string& existingVal) -> std::string {
            if (input.empty()) return "";
            if (input.find("••••") != std::string::npos) return existingVal;
            if (!existingVal.empty() && input == existingVal) return existingVal;
            return omnisphere::utils::Base64::Encode(input);
        };

        toSave.apiKey = encodeIfNeeded(toSave.apiKey, existingSettings.apiKey);
        toSave.apiSecret = encodeIfNeeded(toSave.apiSecret, existingSettings.apiSecret);

        if (existingDt.IsEmpty())
        {
            if (toSave.code.empty()) toSave.code = "DEFAULT";
            return m_repository->Create(toSave, mutationFields);
        }
        else
        {
            toSave.code = existingSettings.code.empty() ? "DEFAULT" : existingSettings.code;
            return m_repository->Update(toSave, mutationFields);
        }
    }
}
