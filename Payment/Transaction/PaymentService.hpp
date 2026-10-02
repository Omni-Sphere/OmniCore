#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "Payment/Transaction/Repositories/PaymentRepository.hpp"
#include "Payment/Transaction/DTOs/CreatePaymentInput.hpp"
#include "Payment/Transaction/DTOs/UpdatePaymentInput.hpp"
#include "Payment/Transaction/Models/Payment.hpp"
#include "Authorization/Models/SecurityContext.hpp"
#include "Authorization/Authorization.hpp"

namespace omnisphere::services
{
    class PaymentService
    {
    private:
        std::shared_ptr<omnisphere::repositories::PaymentRepository> m_repository;
        std::shared_ptr<omnisphere::services::Authorization> m_authService;

    public:
        explicit PaymentService(
            std::shared_ptr<omnisphere::repositories::PaymentRepository> repository,
            std::shared_ptr<omnisphere::services::Authorization> authService = nullptr
        );
        explicit PaymentService(
            std::shared_ptr<omnisphere::data::DatabasePool> dbPool,
            std::shared_ptr<omnisphere::services::Authorization> authService = nullptr
        );
        ~PaymentService() = default;

        std::optional<omnisphere::models::Payment> Create(
            const omnisphere::models::SecurityContext& ctx,
            const omnisphere::dtos::CreatePaymentInput& input
        ) const;

        bool Update(
            const omnisphere::models::SecurityContext& ctx,
            const omnisphere::dtos::UpdatePaymentInput& input,
            const std::string& type
        ) const;

        bool Delete(
            const omnisphere::models::SecurityContext& ctx,
            const std::string& code,
            const std::string& type
        ) const;

        std::optional<omnisphere::models::Payment> GetByCode(
            const omnisphere::models::SecurityContext& ctx,
            const std::string& code
        ) const;

        std::optional<omnisphere::models::Payment> GetByEntity(
            const omnisphere::models::SecurityContext& ctx,
            const std::string& entityType,
            const std::string& entityCode
        ) const;

        std::vector<omnisphere::models::Payment> GetAll(
            const omnisphere::models::SecurityContext& ctx,
            const std::optional<std::string>& entityType = std::nullopt,
            const std::optional<std::string>& entityCode = std::nullopt
        ) const;
    };
} // namespace omnisphere::services
