#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <OmniData/DatabasePool.hpp>
#include <OmniData/DataTable.hpp>
#include "SystemConfig/DTOs/SystemConfig.hpp"
#include "SystemConfig/Models/SystemConfig.hpp"

namespace omnisphere::repositories
{
    class SystemConfig
    {
    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;

    public:
        explicit SystemConfig(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~SystemConfig() = default;

        omnisphere::types::DataTable GetActiveConfig(const std::vector<std::string>& fields = {}) const;
        bool Update(const omnisphere::dtos::UpdateSystemConfigInput& input) const;
        double CalculateAuthorizedTotal(double baseAmount, int paymentMethodEntry) const;
    };
} // namespace omnisphere::repositories
