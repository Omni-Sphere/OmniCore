#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <OmniData/DatabasePool.hpp>
#include "Integration/Models/TicketmasterSettings.hpp"
#include "Integration/Repositories/TicketmasterRepository.hpp"

namespace omnisphere::services
{
    class TicketmasterSettingsService
    {
    private:
        std::shared_ptr<omnisphere::repositories::TicketmasterRepository> m_repository;

    public:
        explicit TicketmasterSettingsService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        explicit TicketmasterSettingsService(std::shared_ptr<omnisphere::repositories::TicketmasterRepository> repository);
        ~TicketmasterSettingsService() = default;

        std::optional<omnisphere::models::TicketmasterSettings> GetSettings(bool decryptKeys = false) const;
        bool SaveSettings(const omnisphere::models::TicketmasterSettings& settings, const std::vector<std::string>& mutationFields = {}) const;
    };
}
