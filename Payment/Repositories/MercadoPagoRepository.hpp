#pragma once
#include "Payment/Models/MercadoPagoSettings.hpp"
#include "Payment/Models/MercadoPagoTransaction.hpp"
#include <OmniData/DatabasePool.hpp>
#include <OmniData/DataTable.hpp>
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace omnisphere::repositories
{
    class MercadoPagoRepository
    {
    public:
        explicit MercadoPagoRepository(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);

        omnisphere::types::DataTable GetSettings(const std::vector<std::string>& requestedFields = {}) const;
        bool Create(const omnisphere::models::MercadoPagoSettings& settings, const std::vector<std::string>& mutationFields = {}) const;
        bool Update(const omnisphere::models::MercadoPagoSettings& settings, const std::vector<std::string>& mutationFields = {}) const;

        bool SaveTransaction(omnisphere::models::MercadoPagoTransaction& tx, const std::vector<std::string>& mutationFields = {}) const;
        bool UpdateTransactionStatus(
            const std::string& paymentId,
            const std::string& newStatus,
            const std::string& statusDetail = "",
            const std::string& ticketUrl = ""
        ) const;
        std::optional<omnisphere::models::MercadoPagoTransaction> GetTransactionByPaymentId(const std::string& paymentId) const;
        std::optional<omnisphere::models::MercadoPagoTransaction> GetTransactionByReference(const std::string& referenceCode) const;

    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
    };
} // namespace omnisphere::repositories
