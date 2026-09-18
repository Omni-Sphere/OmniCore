#pragma once
#include "Payment/Models/PaymentMethod.hpp"
#include "Payment/DTOs/CreatePaymentMethod.hpp"
#include "Payment/DTOs/UpdatePaymentMethod.hpp"
#include <OmniData/DatabasePool.hpp>
#include <memory>
#include <vector>
#include <string>

namespace omnisphere::repositories
{
    class PaymentMethodRepository
    {
    public:
        explicit PaymentMethodRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);

        bool Create(const omnisphere::dtos::CreatePaymentMethodInput& input) const;
        bool Update(const omnisphere::dtos::UpdatePaymentMethodInput& input) const;
        bool Delete(int entry) const;

        omnisphere::types::DataTable ReadAll(const std::vector<std::string>& fields = {}) const;
        omnisphere::types::DataTable GetByEntry(int entry, const std::vector<std::string>& fields = {}) const;
        omnisphere::types::DataTable GetByCode(const std::string& code, const std::vector<std::string>& fields = {}) const;
        omnisphere::types::DataTable GetActiveMethods(const std::vector<std::string>& fields = {}) const;

        std::optional<omnisphere::models::PaymentMethodDetail> GetDetailByCode(const std::string& code) const;
        bool SaveDetail(const std::string& code, const omnisphere::dtos::PaymentMethodDetailInput& detailInput, int userId) const;
        bool DeactivateDetail(const std::string& code, int userId) const;

    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
    };
} // namespace omnisphere::repositories
