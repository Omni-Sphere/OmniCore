#pragma once
#include <memory>
#include <string>
#include <vector>
#include <OmniData/DatabasePool.hpp>
#include <OmniData/DataTable.hpp>
#include "Integration/Models/TicketmasterSettings.hpp"

namespace omnisphere::repositories
{
    class TicketmasterRepository
    {
    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;

    public:
        explicit TicketmasterRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~TicketmasterRepository() = default;

        omnisphere::types::DataTable GetSettings(const std::vector<std::string>& fields = {}) const;
        bool Create(const omnisphere::models::TicketmasterSettings& settings, const std::vector<std::string>& mutationFields = {}) const;
        bool Update(const omnisphere::models::TicketmasterSettings& settings, const std::vector<std::string>& mutationFields = {}) const;
    };
}
