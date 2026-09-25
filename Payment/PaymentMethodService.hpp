#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "Payment/Repositories/PaymentMethodRepository.hpp"
#include "Payment/DTOs/CreatePaymentMethod.hpp"
#include "Payment/DTOs/UpdatePaymentMethod.hpp"
#include "Payment/Models/PaymentMethod.hpp"
#include "Authorization/Models/SecurityContext.hpp"

namespace omnisphere::services
{
    class PaymentMethodService
    {
    private:
        std::shared_ptr<omnisphere::repositories::PaymentMethodRepository> m_repository;

    public:
        explicit PaymentMethodService(std::shared_ptr<omnisphere::repositories::PaymentMethodRepository> repository);
        explicit PaymentMethodService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~PaymentMethodService() = default;

        bool Create(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::CreatePaymentMethodInput& input, const std::vector<std::string>& mutationFields = {}) const;
        bool Update(const omnisphere::models::SecurityContext& ctx, const omnisphere::dtos::UpdatePaymentMethodInput& input, const std::vector<std::string>& mutationFields = {}) const;
        bool Delete(const omnisphere::models::SecurityContext& ctx, int entry) const;

        omnisphere::types::DataTable ReadAll(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& fields = {}) const;
        omnisphere::types::DataTable GetByEntry(const omnisphere::models::SecurityContext& ctx, int entry, const std::vector<std::string>& fields = {}) const;
        omnisphere::types::DataTable GetByCode(const omnisphere::models::SecurityContext& ctx, const std::string& code, const std::vector<std::string>& fields = {}) const;
        omnisphere::types::DataTable GetActiveMethods(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& fields = {}) const;

        std::vector<omnisphere::models::PaymentMethod> GetModels(const omnisphere::models::SecurityContext& ctx, const std::vector<std::string>& fields = {}) const;
        std::optional<omnisphere::models::PaymentMethodDetail> GetDetailByCode(const std::string& code) const;
    };
} // namespace omnisphere::services
