#include "SystemConfig/SystemConfig.hpp"
#include <OmniData/DataMapper.hpp>

namespace omnisphere::services
{
    SystemConfig::SystemConfig(std::shared_ptr<omnisphere::repositories::SystemConfig> repository)
        : m_repository(std::move(repository)) {}

    SystemConfig::SystemConfig(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_repository(std::make_shared<omnisphere::repositories::SystemConfig>(std::move(dbPool))) {}

    omnisphere::types::DataTable SystemConfig::GetActiveConfig(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& fields) const
    {
        if (!m_repository) return {};
        return m_repository->GetActiveConfig(fields);
    }

    bool SystemConfig::Update(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::UpdateSystemConfigInput& input, const std::vector<std::string>& mutationFields) const
    {
        if (!m_repository) return false;
        auto mutableInput = input;
        if (ctx.isAuthenticated() && !ctx.userCode.empty() && (mutableInput.LastUpdatedBy.empty() || mutableInput.LastUpdatedBy == "SYSTEM"))
        {
            mutableInput.LastUpdatedBy = ctx.userCode;
        }
        return m_repository->Update(mutableInput, mutationFields);
    }

    bool SystemConfig::EnsureDefaultExists(const omnisphere::models::SecurityContext& ctx) const
    {
        if (!m_repository) return false;
        return m_repository->EnsureDefaultExists();
    }

    std::optional<omnisphere::models::SystemConfig> SystemConfig::GetModel(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& fields) const
    {
        if (!m_repository) return std::nullopt;
        auto dt = m_repository->GetActiveConfig(fields);
        if (dt.RowsCount() == 0) return std::nullopt;

        auto models = omnisphere::types::DataTableToModels<omnisphere::models::SystemConfig>(dt);
        if (!models.empty())
        {
            return models[0];
        }
        return std::nullopt;
    }

    double SystemConfig::CalculateAuthorizedTotal(const omnisphere::models::SecurityContext& ctx, double baseAmount, int paymentMethodEntry) const
    {
        if (!m_repository) return baseAmount;
        return m_repository->CalculateAuthorizedTotal(baseAmount, paymentMethodEntry);
    }
} // namespace omnisphere::services
