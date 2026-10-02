#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <OmniData/DatabasePool.hpp>
#include <OmniData/DataTable.hpp>
#include <OmniData/QueryBuilder.hpp>
#include "Payment/Transaction/Models/Payment.hpp"
#include "Payment/Transaction/DTOs/CreatePaymentInput.hpp"
#include "Payment/Transaction/DTOs/UpdatePaymentInput.hpp"

namespace omnisphere::repositories
{
    class PaymentRepository
    {
    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;

        static omnisphere::models::Payment MapTransferRow(omnisphere::types::DataTable::Row& row);
        static omnisphere::models::Payment MapCashRow(omnisphere::types::DataTable::Row& row);
        static omnisphere::models::Payment MapCardRow(omnisphere::types::DataTable::Row& row);

    public:
        explicit PaymentRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        ~PaymentRepository() = default;

        std::shared_ptr<omnisphere::data::DatabasePool> GetDatabasePool() const { return m_dbPool; }

        std::optional<omnisphere::models::Payment> Create(const omnisphere::dtos::CreatePaymentInput& input) const;
        bool Update(const omnisphere::dtos::UpdatePaymentInput& input, const std::string& type) const;
        bool Delete(const std::string& code, const std::string& type) const;

        std::optional<omnisphere::models::Payment> GetByCode(const std::string& code, const std::vector<std::string>& requestedFields = {}) const;
        std::optional<omnisphere::models::Payment> GetByEntity(const std::string& entityType, const std::string& entityCode, const std::vector<std::string>& requestedFields = {}) const;
        std::vector<omnisphere::models::Payment> GetAll(const std::optional<std::string>& entityType = std::nullopt, const std::optional<std::string>& entityCode = std::nullopt, const std::vector<std::string>& requestedFields = {}) const;
    };
} // namespace omnisphere::repositories
