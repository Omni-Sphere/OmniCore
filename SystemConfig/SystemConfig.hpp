#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "SystemConfig/Repositories/SystemConfig.hpp"
#include "SystemConfig/DTOs/SystemConfig.hpp"
#include "SystemConfig/Models/SystemConfig.hpp"
#include "Authorization/Models/SecurityContext.hpp"

namespace omnisphere::services
{
    class SystemConfig
    {
    private:
        std::shared_ptr<omnisphere::repositories::SystemConfig> m_repository;

    public:
        explicit SystemConfig(std::shared_ptr<omnisphere::repositories::SystemConfig> repository);
        explicit SystemConfig(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~SystemConfig() = default;

        omnisphere::types::DataTable GetActiveConfig(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& fields = {}) const;
        bool Update(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::UpdateSystemConfigInput& input, const std::vector<std::string>& mutationFields = {}) const;
        bool EnsureDefaultExists(const omnisphere::models::SecurityContext& ctx) const;
        std::optional<omnisphere::models::SystemConfig> GetModel(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& fields = {}) const;
        double CalculateAuthorizedTotal(const omnisphere::models::SecurityContext& ctx, double baseAmount, int paymentMethodEntry) const;
    };
} // namespace omnisphere::services
