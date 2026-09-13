#include "Payment/PaymentMethodService.hpp"
#include <OmniData/DataMapper.hpp>

namespace omnisphere::services
{
    PaymentMethodService::PaymentMethodService(std::shared_ptr<omnisphere::repositories::PaymentMethodRepository> repository)
        : m_repository(std::move(repository)) {}

    PaymentMethodService::PaymentMethodService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_repository(std::make_shared<omnisphere::repositories::PaymentMethodRepository>(std::move(dbPool))) {}

    bool PaymentMethodService::Create(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::CreatePaymentMethodInput& input) const
    {
        if (!m_repository) return false;
        auto mutableInput = input;
        if (ctx.isAuthenticated() && !ctx.userCode.empty())
        {
            try { mutableInput.CreatedBy = std::stoi(ctx.userCode); } catch (...) { mutableInput.CreatedBy = 1; }
        }
        return m_repository->Create(mutableInput);
    }

    bool PaymentMethodService::Update(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::UpdatePaymentMethodInput& input) const
    {
        if (!m_repository) return false;
        auto mutableInput = input;
        if (ctx.isAuthenticated() && !ctx.userCode.empty())
        {
            try { mutableInput.LastUpdatedBy = std::stoi(ctx.userCode); } catch (...) { mutableInput.LastUpdatedBy = 1; }
        }
        return m_repository->Update(mutableInput);
    }

    bool PaymentMethodService::Delete(const omnisphere::models::SecurityContext& /*ctx*/, int entry) const
    {
        if (!m_repository) return false;
        return m_repository->Delete(entry);
    }

    omnisphere::types::DataTable PaymentMethodService::ReadAll(const omnisphere::models::SecurityContext& /*ctx*/, const std::vector<std::string>& fields) const
    {
        if (!m_repository) return {};
        return m_repository->ReadAll(fields);
    }

    omnisphere::types::DataTable PaymentMethodService::GetByEntry(const omnisphere::models::SecurityContext& /*ctx*/, int entry, const std::vector<std::string>& fields) const
    {
        if (!m_repository) return {};
        return m_repository->GetByEntry(entry, fields);
    }

    omnisphere::types::DataTable PaymentMethodService::GetByCode(const omnisphere::models::SecurityContext& /*ctx*/, const std::string& code, const std::vector<std::string>& fields) const
    {
        if (!m_repository) return {};
        return m_repository->GetByCode(code, fields);
    }

    omnisphere::types::DataTable PaymentMethodService::GetActiveMethods(const omnisphere::models::SecurityContext& /*ctx*/, const std::vector<std::string>& fields) const
    {
        if (!m_repository) return {};
        return m_repository->GetActiveMethods(fields);
    }

    std::vector<omnisphere::models::PaymentMethod> PaymentMethodService::GetModels(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& fields) const
    {
        auto dt = ReadAll(ctx, fields);
        return omnisphere::types::DataTableToModels<omnisphere::models::PaymentMethod>(dt);
    }
} // namespace omnisphere::services
