#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "Integration/Models/IntegrationInfo.hpp"
#include "Authorization/Models/SecurityContext.hpp"
#include <OmniData/DatabasePool.hpp>

namespace omnisphere::services
{
    class IntegrationService
    {
    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;

    public:
        explicit IntegrationService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~IntegrationService() = default;

        std::vector<omnisphere::models::IntegrationInfo> GetIntegrations(
            const omnisphere::models::SecurityContext& ctx,
            const std::string& category = ""
        ) const;

        std::optional<omnisphere::models::IntegrationInfo> GetIntegrationByCode(
            const omnisphere::models::SecurityContext& ctx,
            const std::string& code
        ) const;
    };
} // namespace omnisphere::services
