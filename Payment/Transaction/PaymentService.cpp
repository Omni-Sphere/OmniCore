#include "Payment/Transaction/PaymentService.hpp"
#include <OmniUtils/Logger.hpp>
#include <stdexcept>

namespace omnisphere::services
{
    PaymentService::PaymentService(
        std::shared_ptr<omnisphere::repositories::PaymentRepository> repository,
        std::shared_ptr<omnisphere::services::Authorization> authService
    )
        : m_repository(std::move(repository)),
          m_authService(std::move(authService))
    {
    }

    PaymentService::PaymentService(
        std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
        std::shared_ptr<omnisphere::services::Authorization> authService
    )
        : m_repository(std::make_shared<omnisphere::repositories::PaymentRepository>(std::move(dbPool))),
          m_authService(std::move(authService))
    {
    }

    std::optional<omnisphere::models::Payment> PaymentService::Create(
        const omnisphere::models::SecurityContext& ctx,
        const omnisphere::dtos::CreatePaymentInput& input
    ) const
    {
        if (!m_repository) return std::nullopt;
        auto mutableInput = input;
        if (!ctx.userCode.empty() && (mutableInput.CreatedBy.empty() || mutableInput.CreatedBy == "system"))
        {
            mutableInput.CreatedBy = ctx.userCode;
        }
        return m_repository->Create(mutableInput);
    }

    bool PaymentService::Update(
        const omnisphere::models::SecurityContext& /*ctx*/,
        const omnisphere::dtos::UpdatePaymentInput& input,
        const std::string& type
    ) const
    {
        if (!m_repository) return false;
        return m_repository->Update(input, type);
    }

    bool PaymentService::Delete(
        const omnisphere::models::SecurityContext& /*ctx*/,
        const std::string& code,
        const std::string& type
    ) const
    {
        if (!m_repository) return false;
        return m_repository->Delete(code, type);
    }

    std::optional<omnisphere::models::Payment> PaymentService::GetByCode(
        const omnisphere::models::SecurityContext& /*ctx*/,
        const std::string& code,
        const std::vector<std::string>& requestedFields
    ) const
    {
        if (!m_repository) return std::nullopt;
        return m_repository->GetByCode(code, requestedFields);
    }

    std::optional<omnisphere::models::Payment> PaymentService::GetByEntity(
        const omnisphere::models::SecurityContext& /*ctx*/,
        const std::string& entityType,
        const std::string& entityCode,
        const std::optional<std::string>& paymentTypeHint,
        const std::vector<std::string>& requestedFields
    ) const
    {
        if (!m_repository) return std::nullopt;
        return m_repository->GetByEntity(entityType, entityCode, paymentTypeHint, requestedFields);
    }

    std::vector<omnisphere::models::Payment> PaymentService::GetAll(
        const omnisphere::models::SecurityContext& /*ctx*/,
        const std::optional<std::string>& entityType,
        const std::optional<std::string>& entityCode,
        const std::vector<std::string>& requestedFields
    ) const
    {
        if (!m_repository) return {};
        return m_repository->GetAll(entityType, entityCode, requestedFields);
    }
} // namespace omnisphere::services
